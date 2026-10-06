#include "RandomCombatSimulator.h"
#include "CombatParameters.h"
#include "EquipmentSelector.h"
#include "MenuLaunchRequest.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <string>

namespace randomcombat
{
namespace
{
constexpr LPCSTR BUTTON_NAME = "Utils_RandomCombatSimulator_Start";
constexpr LPCSTR JSON_ROOT = "era.random_combat_simulator.";
constexpr DWORD SELECT_SCENARIO_START_GAME = 0x0058BFB0;
constexpr DWORD ADVENTURE_MANAGER_START_BATTLE = 0x0075ADD9;
constexpr DWORD WOG_OPTIONS_ARRAY = 0x02771920;
constexpr DWORD CREATURE_EXPERIENCE_LIMIT = 0x00727E20;
constexpr DWORD HERO_GIVE_EXPERIENCE = 0x004E3620;
constexpr DWORD SILENT_LEVEL_UP = 0x00698450;
constexpr int STACK_EXPERIENCE_OPTION = 900;
constexpr int SINGLE_SCENARIO = 100;
constexpr int DIALOG_OK = 30722;

struct HeroLevelSnapshot
{
    H3Hero *hero = nullptr;
    INT16 level = 0;
    INT32 experience = 0;

    void Apply(const H3Hero &combatHero)
    {
        hero = P_Game->GetHero(combatHero.id);
        level = hero->level;
        experience = hero->experience;
        hero->level = combatHero.level;
        hero->experience = combatHero.experience;
    }

    void Restore()
    {
        if (hero)
        {
            hero->level = level;
            hero->experience = experience;
            hero = nullptr;
        }
    }
};

struct Simulator
{
    Settings settings;
    std::string mapName = "QuickCombat.h3m";
    Generator random;
    H3Hero heroes[2];
    HeroLevelSnapshot heroLevels[2];
    WoG::NPC commanders[2];
    H3MapItem *tile = nullptr;
    INT8 originalLand = 0;
    BOOL originalQuickCombat = FALSE;
    Patch *skipAfterCombat = nullptr;
    bool buttonRegistered = false;
    bool mapLaunchRequested = false;
    MenuLaunchRequest menuLaunch;
    bool battlePending = false;
    bool inCombat = false;

    Simulator() : random(GetTickCount() ^ GetCurrentProcessId())
    {
    }
};

Simulator *simulator = nullptr;

void GiveExperience(H3Hero &hero, int experience)
{
    // The engine checks this DWORD when choosing skills and showing level-up
    // dialogs. Restore the previous value even if a hook throws during the call.
    struct SilentLevelUps
    {
        const DWORD previous = DwordAt(SILENT_LEVEL_UP);
        SilentLevelUps()
        {
            DwordAt(SILENT_LEVEL_UP) = TRUE;
        }
        ~SilentLevelUps()
        {
            DwordAt(SILENT_LEVEL_UP) = previous;
        }
    } silentLevelUps;
    // H3Hero::GiveExperience(amount, checkLevelUps, showCapWindow).

    THISCALL_4(int, HERO_GIVE_EXPERIENCE, &hero, experience, TRUE, FALSE);
}

LPCSTR Text(LPCSTR suffix, LPCSTR fallback)
{
    bool found = false;
    LPCSTR value = EraJS::read(std::string(JSON_ROOT) + "text." + suffix, found);
    return found && value && *value ? value : fallback;
}

int ReadInt(LPCSTR suffix, int fallback, int min, int max)
{
    bool found = false;
    const int value = EraJS::readInt(std::string(JSON_ROOT) + suffix, found);
    return (std::max)(min, (std::min)(max, found ? value : fallback));
}

void ReadRange(Range &range, LPCSTR name, int min, int max)
{
    const std::string key = std::string("battle.") + name;
    range.min = ReadInt((key + ".min").c_str(), range.min, min, max);
    range.max = ReadInt((key + ".max").c_str(), range.max, range.min, max);
}

LPCSTR FileNamePart(LPCSTR path)
{
    LPCSTR result = path ? path : h3_NullString;
    for (LPCSTR cursor = result; *cursor; ++cursor)
        if (*cursor == '\\' || *cursor == '/')
            result = cursor + 1;
    return result;
}

void ReadSettings()
{
    auto &s = *simulator;
    bool found = false;
    LPCSTR mapName = EraJS::read("era.random_combat_simulator.map_file", found);
    if (found && mapName && *FileNamePart(mapName))
        s.mapName = FileNamePart(mapName);
    auto &settings = s.settings;
    ReadRange(settings.stacks, "stacks", 1, 7);
    ReadRange(settings.creatures, "creatures", 0, static_cast<int>(H3CreatureCount::Get()) - 1);
    ReadRange(settings.count, "count", 1, 10000);
    ReadRange(settings.primary, "primary", 0, 99);
    ReadRange(settings.secondary, "secondary", 0, 8);
    ReadRange(settings.experience, "experience", 0, NH3Levels::LEVEL_50);
    ReadRange(settings.artifacts, "artifacts", 0, 14);
    ReadRange(settings.mana, "mana", 0, SHRT_MAX);
    ReadRange(settings.morale, "morale", -3, 3);
    ReadRange(settings.luck, "luck", -3, 3);
    settings.spellChance = ReadInt("battle.spell_chance", settings.spellChance, 0, 100);
    settings.spellbookChance = ReadInt("battle.spellbook_chance", settings.spellbookChance, 0, 100);
    settings.warMachineChance = ReadInt("battle.war_machine_chance", settings.warMachineChance, 0, 100);
    settings.randomTerrain = ReadInt("battle.random_terrain", 1, 0, 1) != 0;
}

// Extracted from Test_DifferentStuff::quickStart. Only the API button skips
// the New Game submenu; the regular New Game -> Single Scenario path remains.
void __stdcall ChooseSingleScenario(HiHook *hook, H3BaseDlg *dlg)
{
    if (simulator->mapLaunchRequested)
    {
        P_WindowManager->resultItemID = SINGLE_SCENARIO;
        return;
    }
    THISCALL_1(void, hook->GetDefaultFunc(), dlg);
}

void __stdcall StartSelectedMap(HiHook *hook, H3SelectScenarioDialog *dlg, int runMode)
{
    auto &s = *simulator;
    if (!dlg->isLoadingMaybe && !dlg->isCampaignMaybe)
    {
        s.mapLaunchRequested = false;
        // mapsList contains all enumerated scenarios, including those hidden
        // by a previously selected map-size filter. UpdateForSelectedScenario
        // takes an index into currentMapsList. Swap ownership to avoid H3API's
        // broken deep assignment of the nested hero bitsets in scenario records.
        for (UINT i = 0; i < dlg->mapsList.Size(); ++i)
        {
            if (libc::strcmpi(FileNamePart(dlg->mapsList[i].playersInfo.filename), s.mapName.c_str()) != 0)
                continue;
            const UINT oldIndex = dlg->selectedMapIndex;
            dlg->randomMapGeneration = FALSE;
            dlg->currentMapsList.swap(dlg->mapsList);
            dlg->selectedMapIndex = UINT_MAX;
            dlg->UpdateForSelectedScenario(i, FALSE);
            if (THISCALL_1(char, SELECT_SCENARIO_START_GAME, dlg))
            {
                P_WindowManager->resultItemID = DIALOG_OK;
                return;
            }
            dlg->currentMapsList.swap(dlg->mapsList);
            if (oldIndex < dlg->currentMapsList.Size())
            {
                dlg->selectedMapIndex = UINT_MAX;
                dlg->UpdateForSelectedScenario(oldIndex, FALSE);
            }
            H3Messagebox::Show(Text("map_invalid", "The simulator map could not be started."));
            return THISCALL_2(void, hook->GetDefaultFunc(), dlg, runMode);
        }
        const std::string message = std::string(Text("map_missing", "Simulator map not found:")) + "\n" + s.mapName;
        H3Messagebox::Show(message.c_str());
    }
    THISCALL_2(void, hook->GetDefaultFunc(), dlg, runMode);
}

int __fastcall StartButton(void *message)
{
    const auto msg = static_cast<H3Msg *>(message);
    if (msg && msg->IsLeftClick())
    {
        simulator->mapLaunchRequested = true;
        simulator->menuLaunch.Queue();
    }
    return TRUE;
}

int __stdcall MainMenuProc(HiHook *hook, H3Msg *msg)
{
    return simulator->menuLaunch.Process(
        *msg, [hook](H3Msg &message) { return FASTCALL_1(int, hook->GetDefaultFunc(), &message); },
        [](H3Msg &message) {
            message.command = eMsgCommand::ITEM_COMMAND;
            message.subtype = eMsgSubtype::LBUTTON_CLICK;
            message.itemId = Era::EGameMenuTarget::PAGE_NEW_GAME;
            message.flags = eMsgFlag::NONE;
            message.parameter = nullptr;
        });
}

std::vector<int> CreaturePool()
{
    std::vector<int> result;
    const auto &range = simulator->settings.creatures;
    auto info = H3CreatureInformation::Get();
    for (int id = range.min; id <= range.max; ++id)
    {
        // Siege weapons and commanders have separate combat slots and cannot
        // be used as ordinary army stacks. Ignore empty extension records too.
        if ((id >= 145 && id <= 149) || Era::IsCommanderId(id) || info[id].siegeWeapon || info[id].hitPoints <= 0 ||
            !info[id].defName || !*info[id].defName)
            continue;
        result.push_back(id);
    }
    return result;
}

void RandomizeEquipment(H3Hero &hero, int artifactCount)
{
    auto &s = *simulator;
    auto setups = H3ArtifactSetup::Get();
    const int count = static_cast<int>(H3ArtifactCount::Get());
    std::vector<int> pool;
    for (int id = 7; id < count; ++id)
        if (setups[id].name && *setups[id].name && setups[id].comboArtifactId == eCombinationArtifacts::NONE)
            pool.push_back(id);
    EquipUniqueArtifacts(
        s.random, artifactCount, std::move(pool),
        [&hero](int id, int slot) { return hero.CanPlaceArtifact(id, slot) != FALSE; },
        [&hero](int id, int slot) { hero.GiveArtifact(H3Artifact(id, -1), slot); });
    if (s.random.Chance(s.settings.spellbookChance))
        hero.GiveArtifact(H3Artifact(eArtifact::SPELLBOOK), NH3Artifacts::eArtifactSlots::SPELLBOOK);
    for (int id = eArtifact::BALLISTA; id <= eArtifact::FIRST_AID_TENT; ++id)
        if (s.random.Chance(s.settings.warMachineChance))
            hero.GiveArtifact(H3Artifact(id, -1), NH3Artifacts::eArtifactSlots::BALLISTA + id - eArtifact::BALLISTA);
}

void CopyHero(H3Hero &destination, const H3Hero &source)
{
    // H3Bitset's assignment cannot instantiate for visitedTowns (48 bits).
    // Copy plain fields around the members that own or construct game data,
    // using the same approach as Utils_CombatEmulator.
    auto target = reinterpret_cast<unsigned char *>(&destination);
    auto original = reinterpret_cast<const unsigned char *>(&source);
    const size_t position = offsetof(H3Hero, mixedPosition);
    const size_t army = offsetof(H3Hero, army);
    const size_t biography = offsetof(H3Hero, biography);
    const size_t biographyEnd = biography + sizeof(destination.biography);
    std::memcpy(target, original, position);
    destination.mixedPosition = source.mixedPosition;
    std::memcpy(target + position + sizeof(destination.mixedPosition),
                original + position + sizeof(source.mixedPosition),
                army - position - sizeof(destination.mixedPosition));
    destination.army = source.army;
    std::memcpy(target + army + sizeof(destination.army), original + army + sizeof(source.army),
                biography - army - sizeof(destination.army));
    destination.biography.Erase();
    if (!source.biography.Empty())
        destination.biography = source.biography;
    std::memcpy(target + biographyEnd, original + biographyEnd, sizeof(H3Hero) - biographyEnd);
}

void PrepareHero(H3Hero &hero, const HeroParameters &parameters, int owner)
{
    CopyHero(hero, *P_Game->GetHero(parameters.id));
    hero.id = parameters.id;
    hero.owner = static_cast<INT8>(owner);
    hero.x = hero.y = hero.z = 0;
    hero.mixedPosition = H3Position::Pack(0, 0, 0);
    hero.flags = 0;
    hero.objectBelow = FALSE;
    hero.level = 1;
    hero.experience = 0;
    hero.moraleBonus = static_cast<INT8>(parameters.morale);
    hero.luckBonus = static_cast<INT8>(parameters.luck);
    for (auto &artifact : hero.bodyArtifacts)
        artifact.Clear();
    for (auto &artifact : hero.backpackArtifacts)
        artifact.Clear();
    libc::memset(hero.blockedArtifacts, 0, sizeof(hero.blockedArtifacts));
    hero.backpackCount = 0;
    for (int i = 0; i < 4; ++i)
        hero.primarySkill[i] = static_cast<INT8>(parameters.primary[i]);
    hero.secSkillCount = parameters.secondaryCount;
    for (int i = 0; i < 28; ++i)
    {
        hero.secSkill[i] = static_cast<INT8>(parameters.secondary[i]);
        hero.secSkillPosition[i] = static_cast<INT8>(parameters.secondaryOrder[i]);
    }
    for (int i = 0; i < 70; ++i)
    {
        hero.learnedSpells[i] = parameters.spells[i] && H3Spell::Get()[i].battlefieldSpell;
        hero.availableSpell[i] = FALSE;
    }
    hero.army.Clear();
    for (int i = 0; i < 7; ++i)
    {
        hero.army.type[i] = parameters.creatures[i];
        hero.army.count[i] = parameters.counts[i];
    }
    GiveExperience(hero, parameters.experience);
    RandomizeEquipment(hero, parameters.artifactCount);
    hero.UpdateAvailableSpell();
    hero.spellPoints = static_cast<INT16>(parameters.mana);
}

int RunBattle(H3AdventureManager *manager)
{
    auto &s = *simulator;
    const auto pool = CreaturePool();
    if (pool.empty())
    {
        H3Messagebox::Show(Text("creatures_missing", "No valid creatures in the configured range."));
        return -2;
    }
    const int attackerId = s.random.Between(0, h3::limits::HEROES - 1);
    int defenderId = s.random.Between(0, h3::limits::HEROES - 2);
    if (defenderId >= attackerId)
        ++defenderId;
    const int attackerOwner = P_Game->GetPlayerID();
    if (attackerOwner < 0 || attackerOwner > 7)
        return -2;
    int defenderOwner = (attackerOwner + 1) % 8;
    for (int i = 0; i < 8; ++i)
        if (i != attackerOwner && !P_Game->IsHuman(i))
        {
            defenderOwner = i;
            break;
        }
    PrepareHero(s.heroes[0], s.random.Hero(attackerId, s.settings, pool), attackerOwner);
    PrepareHero(s.heroes[1], s.random.Hero(defenderId, s.settings, pool), defenderOwner);
    s.tile = P_Game->GetMapItem(H3Position::Pack(0, 0, 0));
    if (!s.tile)
        return -2;
    s.originalLand = s.tile->land;
    s.originalQuickCombat = IntAt(0x006987CC);
    for (int i = 0; i < 2; ++i)
    {
        auto commander = WoG::NPC::Get(s.heroes[i].id);
        s.commanders[i] = *commander;
        commander->on = FALSE;
        // Hero information windows resolve the hero by ID in H3Main::heroes
        // (e.g. 0x4E1AC7), even while combat uses our independent hero copies.
        // Publish their level/XP for those readers and restore them after combat.
        s.heroLevels[i].Apply(s.heroes[i]);
        const int artifacts =
            static_cast<int>(std::count_if(std::begin(s.heroes[i].bodyArtifacts), std::end(s.heroes[i].bodyArtifacts),
                                           [](const H3Artifact &artifact) { return artifact.id >= 7; }));
        const auto description = H3String::Format(
            "side=%d hero=%d combat_level=%d ui_level=%d experience=%d artifacts=%d", i, s.heroes[i].id,
            s.heroes[i].level, s.heroLevels[i].hero->level, s.heroes[i].experience, artifacts);
        Era::WriteLog("Utils_RandomCombatSimulator", "PrepareHero", description.String());
    }
    s.inCombat = true;
    if (s.settings.randomTerrain)
        s.tile->land = static_cast<INT8>(s.random.Between(0, 7));
    // Show an interactive battle even when the player's usual preference is
    // Quick Combat. Preserve the normal results dialog, but skip adventure-map
    // aftermath: these heroes are copies and will never be removed from a map.
    IntAt(0x006987CC) = FALSE;
    s.skipAfterCombat->Apply();
    manager->DemobilizeHero();
    const int result = THISCALL_11(int, ADVENTURE_MANAGER_START_BATTLE, manager, H3Position::Pack(0, 0, 0),
                                   &s.heroes[0], &s.heroes[0].army, defenderOwner, nullptr, &s.heroes[1],
                                   &s.heroes[1].army, s.random.Between(0, INT_MAX), TRUE, FALSE);
    RestoreBattleState();
    return result;
}

// OnGameEnter precedes activation of the adventure manager. The first message
// while it is the active manager is the safe point to enter the combat manager.
int __stdcall StartPendingBattle(HiHook *hook, H3AdventureManager *manager, H3Msg *msg)
{
    if (simulator->battlePending && manager->status == H3Manager::ACTIVE && P_ExecutiveMgr->active_mgr == manager)
    {
        simulator->battlePending = false;
        const int result = RunBattle(manager);
        Era::EGameMenuTarget target = Era::EGameMenuTarget::PAGE_MAIN;
        if (result != -2 &&
            H3Messagebox::Choice(Text("restart", "Battle finished. Restart the map for another random battle?")))
            target = Era::EGameMenuTarget::PAGE_RESTART;
        // RunBattle and Choice have returned; all local game objects have been
        // destroyed before ERA raises its menu-navigation exception.
        Era::FastQuitToGameMenu(target);
        return 0;
    }
    return THISCALL_2(int, hook->GetDefaultFunc(), manager, msg);
}

char __stdcall SaveGame(HiHook *hook, H3Game *game, LPCSTR name, DWORD a3, DWORD a4, DWORD a5, DWORD a6)
{
    if (simulator->inCombat)
        return TRUE;
    return THISCALL_6(char, hook->GetDefaultFunc(), game, name, a3, a4, a5, a6);
}
} // namespace

void SetupBattlefield()
{
    if (!simulator || !simulator->inCombat)
        return;
    // Read the current map's option, including ERA's relocated WoG array.
    // Locked checkbox states 2/3 mean disabled/enabled, respectively.
    const auto options = static_cast<const int *>(Era::GetRealAddr(reinterpret_cast<void *>(WOG_OPTIONS_ARRAY)));
    if (!options || !(options[STACK_EXPERIENCE_OPTION] & 1))
        return;
    auto combat = P_CombatManager;
    auto &s = *simulator;
    if (!combat || combat->hero[0] != &s.heroes[0] || combat->hero[1] != &s.heroes[1])
        return;
    // Battle stack experience is valid starting at OnSetupBattlefield. EA:E
    // changes the temporary combat record and applies its rank bonuses without
    // changing the real hero's adventure-map experience (we fight with copies).
    for (int side = 0; side < 2; ++side)
        for (auto &stack : combat->stacks[side])
        {
            if (stack.slotIndex < 0 || stack.slotIndex >= 7 || stack.type < 0 || stack.numberAlive <= 0 ||
                stack.IsSiege() || stack.IsSummon() || s.heroes[side].army.type[stack.slotIndex] != stack.type)
                continue;
            const int limit = (std::max)(0, CDECL_1(int, CREATURE_EXPERIENCE_LIMIT, stack.type));
            const int experience = s.random.Between(0, limit);
            const auto command = H3String::Format("EA%d:E%d/2/d/d;", -stack.Index() - 1, experience);
            Era::ExecErmCmd(command.String());
        }
}

void RestoreBattleState()
{
    if (!simulator || !simulator->inCombat)
        return;
    auto &s = *simulator;
    s.skipAfterCombat->Undo();
    IntAt(0x006987CC) = s.originalQuickCombat;
    if (s.tile)
        s.tile->land = s.originalLand;
    for (int i = 0; i < 2; ++i)
    {
        *WoG::NPC::Get(s.heroes[i].id) = s.commanders[i];
        s.heroLevels[i].Restore();
    }
    s.tile = nullptr;
    s.inCombat = false;
}

void Initialize()
{
    if (simulator)
        return;
    simulator = new Simulator();
    ReadSettings();
    const auto settings = H3String::Format("build=%s %s map=%s experience=%d..%d artifacts=%d..%d", __DATE__, __TIME__,
                                           simulator->mapName.c_str(), simulator->settings.experience.min,
                                           simulator->settings.experience.max, simulator->settings.artifacts.min,
                                           simulator->settings.artifacts.max);
    Era::WriteLog("Utils_RandomCombatSimulator", "Initialize", settings.String());
    simulator->skipAfterCombat = _PI->WriteJmp(0x004AE010, 0x004AE61B);
    simulator->skipAfterCombat->Undo();
    _PI->WriteHiHook(0x004FBDA0, THISCALL_, MainMenuProc);
    _PI->WriteHiHook(0x004D5B20, THISCALL_, ChooseSingleScenario);
    _PI->WriteHiHook(0x00584EC0, THISCALL_, StartSelectedMap);
    _PI->WriteHiHook(0x00408710, THISCALL_, StartPendingBattle);
    _PI->WriteHiHook(0x004BEB60, THISCALL_, SaveGame);
    const mainmenu::MenuWidgetInfo widget{BUTTON_NAME, Text("button", "Random combat"), mainmenu::eMenuFlags::MAIN,
                                          StartButton};
    simulator->buttonRegistered = mainmenu::MainMenu_RegisterWidget(widget) != FALSE;
    if (!simulator->buttonRegistered)
        H3Messagebox::Show(
            Text("api_missing", "Could not register the simulator button with Interface_MainMenuAPI.era."));
}

void GameEnter()
{
    if (!simulator)
        return;
    RestoreBattleState();
    simulator->mapLaunchRequested = false;
    simulator->menuLaunch.Clear();
    simulator->battlePending = true;
}

void GameLeave()
{
    RestoreBattleState();
    if (simulator)
    {
        simulator->battlePending = false;
        simulator->menuLaunch.Clear();
        // StartSelectedMap (or GameEnter) consumes the scenario-selection flag.
    }
}

void ReloadButtonText()
{
    if (simulator && simulator->buttonRegistered)
        mainmenu::MainMenu_SetDialogButtonText(BUTTON_NAME, Text("button", "Random combat"));
}
} // namespace randomcombat
