#include <algorithm>
#include <limits>
#include <vector>

#include "WoGObjectCaveData.h"
#include "WoGObjectCaveState.h"
#include "WoGObjectIds.h"
#include "WoGObjectPolicy.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"

extern PatcherInstance *_PI;
namespace wog
{
namespace handlers
{
namespace
{
using namespace objects;
state::IntValue CaveValue(const H3Hero *hero, int index)
{
    static const char *fields[] = {
        "rogueServices", "ambushClient", "stolenArtifact", "wealthLesson",  "retaliationLesson", "spellcastingLesson",
        "voodooLesson",  "studiedToday", "stoneTablet",    "magicRoot",     "herbalExtract",     "tradeCargo",
        "poolBlessings", "altarPowers",  "wine",           "huntingTrophy", "oreSample",         "ancientCoin"};
    return state::IntValue(state::HeroKey(ADVENTURE_CAVE, hero->id, fields[index - 61]),
                           index == 62 || index == 63 ? -1 : 0);
}
bool Enabled()
{
    return util::OptionEnabled(OPT_ADVENTURE_CAVE);
}
bool Human(H3Hero *hero)
{
    return hero && hero->owner >= 0 && hero->owner < 8 && P_Main->IsHuman(hero->owner);
}
void Show(const char *key, bool human, const std::vector<std::string> &args = {})
{
    if (human)
        H3Messagebox::Show(util::Tr(std::string("cave.") + key, args).c_str());
}
bool Ask(const char *key, bool human, const std::vector<std::string> &args = {})
{
    return !human || H3Messagebox::Choice(util::Tr(std::string("cave.") + key, args).c_str());
}
bool Enemy(int owner, int other)
{
    if (other < 0 || other > 7 || owner == other)
        return false;
    return !P_Main->mapInfo.hasTeams || P_Main->mapInfo.playerTeam[owner] != P_Main->mapInfo.playerTeam[other];
}
void Morale(H3Hero *hero, int delta)
{
    hero->moraleBonus = static_cast<INT8>((std::max)(-3, (std::min)(3, int(hero->moraleBonus) + delta)));
}
void Alignment(H3Hero *hero, int action)
{
    const int town = hero->hero_class / 2;
    const int alignment = town < 3 ? 1 : town < 6 ? -1 : 0;
    if (alignment)
        Morale(hero, 2 * alignment * action);
}
void Journal(H3Hero *hero, int variable, bool remove = false)
{
    if (!hero)
        return;
    state::Set(state::HeroKey(ADVENTURE_CAVE, hero->id, ("journal." + std::to_string(variable)).c_str()), !remove);
}
_LHF_(ShowNativeCaveJournal)
{
    // WoG's MakeQuestLog call site passes MQL_hp in EAX (WoG Sources/_B1.cpp, erm.cpp).
    // Keep the native cave notes independent of QW's fixed global z-string slots.
    const auto *hero = c->Eax<H3Hero *>();
    if (!hero || hero->id < 0 || hero->id >= H3HeroCount::Get())
        return EXEC_DEFAULT;
    std::string notes;
    for (int variable = 63; variable <= 78; ++variable)
    {
        if (state::Get(state::HeroKey(ADVENTURE_CAVE, hero->id,
                                     ("journal." + std::to_string(variable)).c_str())) == 0)
            continue;
        bool found = false;
        EraJS::read("wog_objects.native.cave.journal." + std::to_string(variable), found);
        if (found)
            notes += "\n\n" + util::Tr("cave.journal." + std::to_string(variable));
    }
    if (!notes.empty())
        H3Messagebox::Show((util::Tr("cave.journalTitle", {"hero", hero->name}) + notes).c_str());
    return EXEC_DEFAULT;
}
void __stdcall InstallCaveJournal(Era::TEvent *)
{
    // ERA loads .era plugins before InitWoG, which installs MakeQuestLog at this call site.
    // Chain that final call only after WoG has patched it.
    _PI->WriteLoHook(0x4DD82F, ShowNativeCaveJournal);
}
void Store(H3MapItem *item, H3Hero *hero, H3Position pos, cave::State data)
{
    state::ObjectPositionScope position(item, pos);
    state::StoreSetup(item, data.word, hero);
}
cave::State Read(H3MapItem *item, H3Position pos)
{
    cave::State data{item->setup};
    if (data.Initialized())
        return data;
    int rooms[4];
    for (auto &room : rooms)
        room = util::Random(0, 63);
    return cave::State::Fresh(util::Random(0, 3), rooms);
}
int battleType = 0, battleVariant = 0, battleRound = -1, battleWinner = -1, battleHero = -1, oldTactics = 0;
bool machinesRemoved = false, extraCast[2] = {}, actionBusy = false;
H3Artifact machines[3];
void RestoreMachines()
{
    if (!machinesRemoved || battleHero < 0)
        return;
    machinesRemoved = false;
    H3Hero *hero = P_Main->GetHero(battleHero);
    for (int i = 0; i < 3; ++i)
    {
        if (machines[i].id >= 0 && hero->bodyArtifacts[13 + i].id < 0)
            hero->GiveArtifact(machines[i], 13 + i);
    }
}
void __stdcall BeforeBattle(Era::TEvent *)
{
    if (!battleType || battleHero < 0 || policy::LoadingGame())
        return;
    util::Receiver("BA:B12;");
    oldTactics = util::TacticsEnabled(P_Main->GetHero(battleHero));
    state::Set("wog_objects.native.57.battle.tactics", oldTactics);
    util::SetTactics(P_Main->GetHero(battleHero), false);
    H3Hero *hero = P_Main->GetHero(battleHero);
    for (int i = 0; i < 3; ++i)
    {
        machines[i] = hero->bodyArtifacts[13 + i];
        if (machines[i].id >= 4 && machines[i].id <= 6)
            hero->RemoveArtifact(13 + i);
        else
            machines[i].id = -1;
    }
    machinesRemoved = true;
}
void __stdcall SetupBattle(Era::TEvent *)
{
    battleRound = -1;
    extraCast[0] = extraCast[1] = false;
    if (policy::LoadingGame())
        return;
    state::Set("wog_objects.native.57.battle.round", -1);
    if (battleType >= 1 && battleType <= 10)
    {
        util::Receiver("BF:C;"); // WoG private BF_Prepare gate: suppress the later random layout.
        util::ClearObstacles();
        const auto &field = cave_data::battlefields[battleType - 1];
        for (unsigned i = 0; i < field.count; ++i)
            util::PlaceObstacle(field.entries[i].object, field.entries[i].hex);
    }
    RestoreMachines();
}
H3CombatCreature *Stack(int side, int type)
{
    if (!P_CombatManager)
        return nullptr;
    for (auto &stack : P_CombatManager->stacks[side])
        if (stack.type == type && stack.numberAlive > 0)
            return &stack;
    return nullptr;
}
void Summon(int type, int count, int side, int hex)
{
    if (util::ValidCreature(type) && count > 0 && util::StackAt(hex) == -1)
        P_CombatManager->SummonCreature(side, type, count, hex, FALSE, FALSE);
}
void __stdcall PositionBattle(Era::TEvent *)
{
    if (!battleType || !P_CombatManager || policy::LoadingGame())
        return;
    if (battleType == 1 && IntAt(0x6987CC) != 0)
    {
        for (int hex : {4, 31, 48, 173, 165})
            Summon(70, 10, 1, hex);
        return;
    }
    const int *positions = cave_data::positions[battleType - 1];
    for (int pass = 0; pass < 2; ++pass)
        for (int side = 0; side < 2; ++side)
            for (int slot = 0; slot < 7; ++slot)
                if (P_CombatManager->stacks[side][slot].numberAlive > 0)
                    util::PlaceBattleStack(&P_CombatManager->stacks[side][slot], positions[side * 7 + slot]);
    int total = 0, type = -1;
    for (int slot = 0; slot < 7; ++slot)
    {
        const auto &s = P_CombatManager->stacks[1][slot];
        if (s.numberAlive > 0)
        {
            total += s.numberAlive;
            type = s.type;
        }
    }
    if (type >= 0)
        for (int slot = 0; slot < 7; ++slot)
        {
            if (P_CombatManager->stacks[1][slot].numberAlive <= 0)
                Summon(type, 1, 1, positions[7 + slot]);
            auto &stack = P_CombatManager->stacks[1][slot];
            stack.numberAlive = stack.numberAtStart = (std::max)(1, total / 7);
        }
    if (battleType == 1)
        for (int hex : {4, 31, 48, 173, 165})
            Summon(70, 10, 1, hex);
    if (battleType == 2)
        for (int hex : {60, 95, 129, 110, 80, 47, 133, 148})
            Summon(143, 10, 1, hex);
    if (battleType == 3)
        util::Receiver("BM21:Q1/40/0 Q1/42/0 Q1/82/0 Q1/83/0 Q1/116/0 Q1/117/0 Q1/141/0 Q1/143/0 Q1/145/0 Q1/57/0 "
                       "Q1/58/0 Q1/59/0 Q1/60/0 Q1/124/0 Q1/125/0 Q1/126/0 Q1/127/0 Q1/128/0 Q1/129/0;");
    if (battleType == 7)
    {
        Summon(116, 10, 1, 27);
        Summon(116, 10, 1, 163);
        Summon(35, 90, 1, 99);
    }
    if (battleType == 8)
        util::Receiver("BM21:Q0/76/0 Q0/77/0 Q0/93/0 Q0/94/0 Q0/95/0 Q0/110/0 Q0/111/0;");
    if (battleType == 9)
    {
        for (int hex : {58, 60, 73, 78, 107, 112, 126, 127, 128})
            Summon(43, 25, 1, hex);
        for (int hex : {38, 45, 164})
            Summon(45, 30, 1, hex);
    }
    if (battleType == 10)
        for (int i = 0; i < 7; ++i)
        {
            const int from[] = {39, 72, 105, 138, 130, 97, 64}, to[] = {42, 75, 108, 141, 128, 95, 62};
            int a = util::StackAt(from[i]);
            int b = util::StackAt(to[i]);
            if (a >= 0 && b >= 0)
            {
                const int qty = P_CombatManager->stacks[a / 21][a % 21].numberAlive;
                if (qty > 0)
                    P_CombatManager->stacks[b / 21][b % 21].numberAlive =
                        P_CombatManager->stacks[b / 21][b % 21].numberAtStart = qty;
            }
        }
}
void Mass(int side, int spell, int duration, int expertise = 3)
{
    for (auto &s : P_CombatManager->stacks[side])
        if (s.numberAlive > 0)
            s.ApplySpell(spell, duration, expertise, P_CombatManager->hero[side]);
}
void __stdcall RoundBattle(Era::TEvent *)
{
    if (!P_CombatManager)
        return;
    ++battleRound;
    extraCast[0] = extraCast[1] = false;
    for (int side = 0; side < 2; ++side)
        state::Set("wog_objects.native.57.battle.extraCast." + std::to_string(side), 0);
    state::Set("wog_objects.native.57.battle.round", battleRound);
    for (int side = 0; side < 2; ++side)
    {
        H3Hero *hero = P_CombatManager->hero[side];
        if (!hero)
            continue;
        if (battleRound == 0)
        {
            const int spells[] = {41, 53, 27, 43};
            for (int i = 0; i < 4; ++i)
                if (CaveValue(hero, 73) & (1 << i))
                    Mass(side, spells[i], 255);
        }
        for (auto &s : P_CombatManager->stacks[side])
            if (s.numberAlive > 0)
            {
                if (battleRound == 0 && (CaveValue(hero, 74) & 4) && util::ValidCreature(s.type) &&
                    P_CreatureInformation[s.type].level == 0)
                {
                    s.baseHP *= 2;
                    s.info.hitPoints *= 2;
                }
                if (battleRound >= 1 && battleRound <= 20 && (CaveValue(hero, 74) & 8))
                {
                    s.info.attack += 2;
                    s.info.defence += 2;
                    s.info.speed += 2;
                }
            }
    }
    if (battleType == 3 && battleVariant && battleRound >= 1)
    {
        if (auto *s = Stack(1, 137))
            for (int hex : {63, 148})
            {
                int other = hex - 16 - (hex / 17 % 2);
                if (util::StackAt(hex) == -1 && util::StackAt(other) == -1 &&
                    !(P_CombatManager->squares[hex].obstacleBits & 0x3F) &&
                    !(P_CombatManager->squares[other].obstacleBits & 0x3F))
                    util::CastCreatureSpell(s, 13, hex, 1, 1);
            }
    }
    if (battleType == 4 && battleRound < 2)
        Mass(0, 70, 4 - battleRound, 0);
    if (battleType == 4 && battleVariant && battleRound == 1)
        Summon(7, 30, 0, 1);
    if (battleType == 6 && battleVariant && battleRound > 0 && battleRound < 5)
    {
        const int hex[] = {171, 1, 15, 185};
        Summon(159, 30, 1, hex[battleRound % 4]);
    }
    if (battleType == 7)
    {
        auto *mage = Stack(1, 35);
        auto *caster = mage ? mage : Stack(1, 116);
        if (mage && battleRound == 0)
            mage->ApplySpell(34, 3, 3, nullptr);
        if (caster && battleRound == 1)
            for (int hex : {25, 93, 161})
                util::CastCreatureSpell(caster, 64, hex, 3, 3);
        if (mage && battleRound > 0)
            for (auto &s : P_CombatManager->stacks[0])
                if (s.numberAlive > 0)
                    util::CastCreatureSpell(mage, 15, s.position, 1, 1);
    }
    if (battleType == 8 && battleVariant && battleRound == 1)
        Mass(0, 71, 4);
}
void __stdcall BeforeAction(Era::TEvent *)
{
    if (!P_CombatManager || actionBusy)
        return;
    const int side = P_CombatManager->currentActiveSide, action = int(P_CombatManager->action);
    if (side < 0 || side > 1)
        return;
    auto *hero = P_CombatManager->hero[side];
    if (!hero)
        return;
    const int target = util::StackAt(P_CombatManager->actionTarget);
    if ((CaveValue(hero, 74) & 1) && action >= 6 && action <= 7 && target >= 0 && target < 42)
    {
        auto &s = P_CombatManager->stacks[target / 21][target % 21];
        if (s.numberAlive > 0)
            s.ApplySpell(71, 3, 1, hero);
    }
    if ((CaveValue(hero, 74) & 2) && action == 1 && P_CombatManager->actionParameter == 15)
    {
        actionBusy = true;
        const int mana = hero->spellPoints;
        hero->spellPoints = (std::min)(32767, mana + 100);
        for (auto &s : P_CombatManager->stacks[1 - side])
            if (s.numberAlive > 0 && (1 - side) * 21 + s.sideIndex != target)
                util::CastHeroSpell(side, 15, s.position, 2);
        hero->spellPoints = mana;
        actionBusy = false;
    }
}
void __stdcall AfterAction(Era::TEvent *)
{
    if (!P_CombatManager || actionBusy || int(P_CombatManager->action) != 1)
        return;
    const int side = P_CombatManager->currentActiveSide;
    if (side < 0 || side > 1 || extraCast[side])
        return;
    H3Hero *hero = P_CombatManager->hero[side];
    if (hero && CaveValue(hero, 66) >= 3)
    {
        extraCast[side] = true;
        state::Set("wog_objects.native.57.battle.extraCast." + std::to_string(side), 1);
        P_CombatManager->heroCasted[side] = FALSE;
    }
}
_LHF_(AfterMelee)
{
    if (!P_CombatManager)
        return EXEC_DEFAULT;
    auto *atk = c->Esi<H3CombatCreature *>(), *def = c->Edi<H3CombatCreature *>();
    if (!atk || !def || def->side < 0 || def->side > 1 || atk->side < 0 || atk->side > 1)
        return EXEC_DEFAULT;
    H3Hero *defHero = P_CombatManager->hero[def->side], *atkHero = P_CombatManager->hero[atk->side];
    if (defHero && CaveValue(defHero, 65) >= 3 && (def->type < 145 || def->type > 149) && def->numberAlive > 0 &&
        def->retaliations < (std::numeric_limits<int>::max)())
        ++def->retaliations;
    if (atkHero && CaveValue(atkHero, 67) >= 3 && atk == P_CombatManager->activeStack && def->numberAlive > 0)
    {
        def->info.flyer = 0;
        def->info.shooter = 0;
    }
    return EXEC_DEFAULT;
}
void __stdcall AfterBattle(Era::TEvent *)
{
    state::Set("wog_objects.native.57.battle.active", 0);
    if (!battleType)
        return;
    if (P_CombatManager)
        battleWinner = P_CombatManager->winnerSide;
    RestoreMachines();
    if (battleHero >= 0)
        util::SetTactics(P_Main->GetHero(battleHero), oldTactics != 0);
}
bool Fight(H3Hero *hero, H3Position pos, int type, int count, int field, int variant, bool human)
{
    if (!util::ValidCreature(type) || !hero->army.HasCreatures())
        return false;
    if (!human && util::ArmyAiValue(hero->army) < int64_t(P_CreatureInformation[type].aiValue) * count * 3)
        return false;
    battleType = field;
    battleVariant = variant;
    battleHero = hero->id;
    battleWinner = -1;
    state::Set("wog_objects.native.57.battle.active", field != 0);
    state::Set("wog_objects.native.57.battle.type", field);
    state::Set("wog_objects.native.57.battle.variant", variant);
    state::Set("wog_objects.native.57.battle.hero", hero->id);
    const int owner = hero->owner;
    util::Fight(hero, pos, type, count);
    RestoreMachines();
    const bool won = battleWinner == 0 && hero->owner == owner && hero->army.HasCreatures();
    battleType = 0;
    battleHero = -1;
    return won;
}
int ArmyHP(const H3Army &army)
{
    int64_t sum = 0;
    for (int i = 0; i < 7; ++i)
        if (util::ValidCreature(army.type[i]) && army.count[i] > 0)
            sum += int64_t(army.count[i]) * P_CreatureInformation[army.type[i]].hitPoints;
    return int((std::min)(sum, int64_t((std::numeric_limits<int>::max)())));
}
void Loot(H3Hero *hero, int variable)
{
    if (CaveValue(hero, variable) < (std::numeric_limits<int>::max)())
        ++CaveValue(hero, variable);
    Journal(hero, variable);
}
void Orb(H3Hero *hero, int variant)
{
    for (int id = 0; id < H3HeroCount::Get(); ++id)
    {
        auto *other = P_Main->GetHero(id);
        if (other->owner < 0 || other->owner > 7)
            continue;
        if ((variant == 0 && Enemy(hero->owner, other->owner)) || (variant == 2 && hero->owner == other->owner))
            util::RevealArea(other->x, other->y, other->z, hero->owner, variant == 0 ? 3 : 9, variant == 0);
    }
    for (auto &town : P_Main->towns)
        if ((variant == 1 && town.owner != hero->owner) || (variant == 3 && town.owner == hero->owner))
            util::RevealArea(town.x, town.y, town.z, hero->owner, variant == 1 ? 4 : 16, variant == 1);
}
std::vector<H3Point> Obelisks()
{
    std::vector<H3Point> result;
    const int size = P_Main->mainSetup.mapSize;
    for (int z = 0; z <= int(P_Main->mainSetup.hasUnderground); ++z)
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                if (P_Main->mainSetup.GetMapItem(x, y, z)->objectType == 57)
                    result.push_back(H3Point(x, y, z));
    return result;
}
void RogueQuest(H3Hero *hero, int room, bool human)
{
    auto obelisks = Obelisks();
    if (obelisks.empty() || !human)
        return;
    if (!Ask("release", human))
    {
        Alignment(hero, -1);
        if (hero->hero_class / 2 >= 3 && util::Random(0, 99) < 25 && hero->backpackCount < 64)
        {
            const int arts[] = {8, 14, 20, 26, 37, 45, 105, 107, 56, 52};
            int art = arts[util::Random(0, 9)];
            if (util::ArtifactEnabled(art))
                hero->GiveBackpackArtifact(H3Artifact(art, -1));
        }
        return;
    }
    const auto p = obelisks[(room * 7) % obelisks.size()];
    const auto key = state::PlayerKey(ADVENTURE_CAVE, hero->owner, ("obelisk." + std::to_string(room)).c_str());
    state::Set(key, p.Pack() + 1);
    util::RevealArea(p.x, p.y, p.z, hero->owner, 0);
    Show("obelisk", human, {"x", std::to_string(p.x), "y", std::to_string(p.y)});
}
int RoguePrice(int price, int friendliness)
{
    return friendliness < 2 ? price * 2 : friendliness == 2 ? price * 3 / 2 : price;
}
int RogueService(H3Hero *hero, int room, int service, int friendliness, bool human)
{
    if (CaveValue(hero, 61) & (1 << room))
    {
        Show("roguesUsed", human);
        return 0;
    }
    int price = RoguePrice(service == 0 ? 1350 : service == 1 ? 7500 : service == 2 ? 4000 : 3000, friendliness);
    int victim = -1, artifact = -1, slot = -1;
    H3Artifact stolen;
    H3Point mine(-1, -1, -1);
    if (service == 1)
    {
        const int size = P_Main->mainSetup.mapSize;
        for (int z = 0; z <= int(P_Main->mainSetup.hasUnderground) && mine.x < 0; ++z)
            for (int y = 0; y < size && mine.x < 0; ++y)
                for (int x = 0; x < size && mine.x < 0; ++x)
                {
                    auto *m = P_Main->mainSetup.GetMapItem(x, y, z);
                    if (m->objectType != 53 || m->objectSubtype != 6)
                        continue;
                    const int owner = m->mine.Get()->owner;
                    const std::string key = H3String::Format("wog_objects.native.57.mine.%d.%d.%d", x, y, z).String();
                    if (Enemy(hero->owner, owner) && !state::IsStamp(key, state::Week()))
                    {
                        mine = H3Point(x, y, z);
                        victim = owner;
                    }
                }
    }
    if (service == 2)
    {
        int hp = (std::numeric_limits<int>::max)();
        for (int id = 0; id < H3HeroCount::Get(); ++id)
        {
            auto *h = P_Main->GetHero(id);
            if (!Enemy(hero->owner, h->owner))
                continue;

            const int value = ArmyHP(h->army);
            if (CaveValue(h, 62) < 0 && value > 0 && value < hp)
            {
                hp = value;
                victim = id;
            }
        }
    }
    if (service == 3)
    {
        const int ids[30] = {52, 53, 46, 47, 48, 49,  50,  51,  64,  63,  65, 112, 114, 111, 80,
                             82, 81, 79, 92, 93, 117, 116, 115, 125, 126, 91, 86,  88,  89,  87};
        for (int tier = 2; tier >= 0 && victim < 0; --tier)
            for (int id = 0; id < H3HeroCount::Get() && victim < 0; ++id)
            {
                auto *h = P_Main->GetHero(id);
                if (!Enemy(hero->owner, h->owner))
                    continue;
                for (int s = 0; s < 19 && victim < 0; ++s)
                    for (int n = tier * 10; n < tier * 10 + 10; ++n)
                        if (h->bodyArtifacts[s].id == ids[n])
                        {
                            victim = id;
                            slot = s;
                            stolen = h->bodyArtifacts[s];
                            artifact = stolen.id;
                            price = RoguePrice(3000 * (tier + 1), friendliness);
                            break;
                        }
            }
    }
    if ((service != 0 && victim < 0) || (service == 3 && hero->backpackCount >= 64))
    {
        Show("noService", human);
        return 0;
    }
    if (!Ask("service", human,
             {"service", util::Tr("cave.services." + std::to_string(service)), "gold", std::to_string(price)}))
        return 0;
    if (util::Resource(hero->owner, 6) < price)
    {
        Show("poor", human);
        return -1;
    }
    bool ok = false;
    if (service == 0)
        ok = util::AddArmy(hero->army, {{143, 15}});
    if (service == 1)
    {
        auto *nativeMine = P_Main->mainSetup.GetMapItem(mine.x, mine.y, mine.z)->mine.Get();
        if (nativeMine && nativeMine->owner == victim)
        {
            nativeMine->owner = static_cast<INT8>(hero->owner);
            nativeMine->army.Clear();
            state::Set(state::PlayerKey(ADVENTURE_CAVE, victim, "mineNotice"), 1);
            state::Stamp(H3String::Format("wog_objects.native.57.mine.%d.%d.%d", mine.x, mine.y, mine.z).String(),
                         state::Week());
            ok = true;
        }
    }
    if (service == 2)
    {
        auto *h = P_Main->GetHero(victim);
        if (Enemy(hero->owner, h->owner) && CaveValue(h, 62) < 0)
        {
            CaveValue(h, 62) = hero->id;
            ok = true;
        }
    }
    if (service == 3)
    {
        auto *h = P_Main->GetHero(victim);
        if (Enemy(hero->owner, h->owner) && h->bodyArtifacts[slot].id == artifact &&
            h->bodyArtifacts[slot].subtype == stolen.subtype)
        {
            h->RemoveArtifact(slot);
            hero->GiveBackpackArtifact(stolen);

            CaveValue(h, 63) = artifact;
            ok = true;
        }
    }
    if (ok)
    {
        util::ChangeResource(hero, eResource::GOLD, -price);
        CaveValue(hero, 61) |= 1 << room;
        Show("serviceDone", human);
    }
    return ok ? 1 : 0;
}
bool HandleRoom(H3Hero *hero, H3MapItem *item, H3Position pos, cave::State &data, int room, bool human)
{
    int value = data.Room(room), kind = value & 3, param = (value >> 2) & 15;
    if (value & 64)
    {
        Show("empty", human);
        return false;
    }
    if (kind == 0)
    {
        int friendliness = (param & 3) + (ArmyHP(hero->army) > 2000);
        for (int i = 0; i < 7; ++i)
            if (hero->army.type[i] == 143 && hero->army.count[i] > 0)
            {
                ++friendliness;
                break;
            }
        const bool sleepy = util::Random(0, 5) == 5;
        bool fight = false;
        if (human)
        {
            const auto pick =
                H3Messagebox::Choose(util::Tr(sleepy         ? "cave.roguesSleeping"
                                              : friendliness ? "cave.roguesMenu"
                                                             : "cave.roguesHostile")
                                         .c_str(),
                                     H3PictureCategories::Morale(friendliness || sleepy ? 1 : -1),
                                     H3PictureCategories::Creature(static_cast<eCreature>(143), sleepy ? 100 : 200));
            if (pick == H3Messagebox::NONE)
                return false;
            fight = pick == H3Messagebox::RIGHT;
            if (!fight && !sleepy && !friendliness)
            {
                Morale(hero, -2);
                return false;
            }
            if (sleepy && !fight && !Ask("wake", human))
                return false;
        }
        else if (friendliness < 2)
            return false;
        if (!fight)
        {
            const int result = RogueService(hero, room, (param >> 2) & 3, friendliness + int(sleepy), human);
            if (result < 0 && (param & 3) > 0 && util::Random(0, 1))
                data.Room(room, value - 4);
            return result > 0;
        }
        if (sleepy)
            Alignment(hero, -1);
        if (Fight(hero, pos, eCreature::ROGUE, sleepy ? 100 : 200, 2, 0, human))
        {
            data.Finish(room);
            util::ChangeResource(hero, eResource::GOLD, util::Random(20, 40) * 100);
            util::ChangeResource(hero, eResource::GEMS, util::Random(3, 8));
            if (util::Random(0, 1))
                RogueQuest(hero, room, human);
        }
        return true;
    }
    if (kind == 1)
    {
        const int requirement = param & 3, lesson = (param >> 2) & 3;
        bool ready = requirement == 0;
        if (requirement == 1)
        {
            int slot = -1;
            for (int art : {6, 94, 95, 96})
                for (int i = 0; i < 19 && slot < 0; ++i)
                    if (hero->bodyArtifacts[i].id == art)
                        slot = i;
            if (hero->secSkill[27] >= 3 || slot >= 0)
            {
                if (Ask("heal", human))
                {
                    if (hero->secSkill[27] < 3)
                        hero->RemoveArtifact(slot);
                    ready = true;
                }
            }
        }
        if (requirement == 2)
        {
            int living = 0;
            for (int i = 0; i < data.Count(); ++i)
                if (!(data.Room(i) & 64) && (data.Room(i) & 3) != 3)
                    ++living;
            ready = living < 2;
        }
        if (requirement == 3)
        {
            ready = true;
            for (int i = 0; i < 83; ++i)
            {
                if (i >= 13 && i <= 17)
                    continue;
                auto &a = i < 19 ? hero->bodyArtifacts[i] : hero->backpackArtifacts[i - 19];
                if (a.id >= 0)
                    ready = false;
            }
        }
        if (!ready)
        {
            Show("hermitBlocked", human, {"reason", util::Tr("cave.hermitRequirement." + std::to_string(requirement))});
            return false;
        }
        if (requirement == 1 || requirement == 2)
            data.Room(room, value & ~12);
        if (CaveValue(hero, 68) || CaveValue(hero, 64 + lesson) >= 3)
        {
            Show("lessonUsed", human);
            return false;
        }
        if (Ask("lesson", human,
                {"lesson", util::Tr("cave.lessons." + std::to_string(lesson)), "level",
                 std::to_string(CaveValue(hero, 64 + lesson) + 1)}))
        {
            ++CaveValue(hero, 64 + lesson);
            CaveValue(hero, 68) = 1;
            if (CaveValue(hero, 64 + lesson) == 3)
            {
                data.Finish(room);
                Journal(hero, 64 + lesson);
            }
            return true;
        }
        return false;
    }
    if (kind == 2)
    {

        const int types[] = {eCreature::SHARPSHOOTER, eCreature::GREATER_BASILISK, eCreature::MINOTAUR_KING,
                             creature::GHOST,         eCreature::GOLD_GOLEM,       eCreature::POWER_LICH,
                             eCreature::FAMILIAR,     eCreature::CRUSADER},
                  counts[] = {70, 84, 56, 210, 70, 77, 175, 7};
        const int gold[] = {5000, 7000, 8000, 3000, 9000, 5000, 4000, 5000}, resources[] = {3, 1, 5, 4, -1, -1, -1, -1},
                  loot[] = {-1, -1, -1, -1, 69, 70, 72, 71};
        const int index = param & 7;
        if (Ask("fight", human,
                {"creature", P_CreatureInformation[types[index]].namePlural, "count", std::to_string(counts[index])}) &&
            Fight(hero, pos, types[index], counts[index], 3 + index, param >> 3, human))
        {
            data.Finish(room);
            util::ChangeResource(hero, eResource::GOLD, gold[index]);
            if (resources[index] >= 0)
                util::ChangeResource(hero, static_cast<eResource>(resources[index]), 5);
            if (loot[index] >= 0)
                Loot(hero, loot[index]);
            Show("victory", human, {"gold", std::to_string(gold[index])});
            return true;
        }
        return false;
    }
    const int feature = param >> 2, variant = param & 3;
    if (feature == 0)
    {
        if (CaveValue(hero, 73) & (1 << variant))
        {
            Show("poolUsed", human);
            return false;
        }
        CaveValue(hero, 73) |= 1 << variant;
        Show("pool", human, {"spell", P_Spell[variant == 0 ? 41 : variant == 1 ? 53 : variant == 2 ? 27 : 43].name});
        return true;
    }
    if (feature == 1)
    {
        Orb(hero, variant);
        state::SetObjectPlayerPeriod(item, ADVENTURE_CAVE, hero->owner, ("orb." + std::to_string(room)).c_str(),
                                     state::Day());
        Show("orb", human);
        return true;
    }
    if (feature == 2)
    {
        const int gold[] = {2000, 1500, 1000, 0};
        util::ChangeResource(hero, eResource::GOLD, gold[variant]);
        Loot(hero, 75 + variant);
        data.Finish(room);
        Show("cache", human, {"gold", std::to_string(gold[variant])});
        return true;
    }
    const int mana[] = {15, 25, 10, 10};
    if ((CaveValue(hero, 74) & (1 << variant)) || hero->spellPoints < mana[variant])
    {
        Show("altarUsed", human);
        return false;
    }
    if (Ask("altar", human,
            {"mana", std::to_string(mana[variant]), "effect", util::Tr("cave.altars." + std::to_string(variant))}))
    {
        hero->spellPoints -= mana[variant];
        CaveValue(hero, 74) |= 1 << variant;
        return true;
    }
    return false;
}
int RoomScore(const H3Hero *hero, int value, int room)
{
    if (value & 64)
        return 0;
    const int kind = value & 3, param = (value >> 2) & 15;
    if (kind == 0 && (param >> 2) == 0 && !(CaveValue(hero, 61) & (1 << room)))
    {
        int friendliness = (param & 3) + (ArmyHP(hero->army) > 2000);
        for (int slot = 0; slot < 7; ++slot)
            if (hero->army.type[slot] == eCreature::ROGUE && hero->army.count[slot] > 0)
            {
                ++friendliness;
                break;
            }
        return friendliness >= 2 && util::Resource(hero->owner, 6) >= RoguePrice(1350, friendliness) + 1000 &&
                       util::CanAddArmy(hero->army, {{eCreature::ROGUE, 15}})
                   ? 900
                   : 0;
    }
    if (kind == 1)
        return !CaveValue(hero, 68) && CaveValue(hero, 64 + (param >> 2)) < 3 && (param & 3) == 0 ? 1800 : 0;
    if (kind == 3)
    {
        int feature = param >> 2, variant = param & 3;
        if (feature == 0)
            return (CaveValue(hero, 73) & (1 << variant)) ? 0 : 1200;
        if (feature == 2)
            return 1600;
        if (feature == 3)
        {
            const int mana[] = {15, 25, 10, 10};
            return !(CaveValue(hero, 74) & (1 << variant)) && hero->spellPoints >= mana[variant] + 10 ? 800 : 0;
        }
        return 0;
    }
    if (kind == 2)
    {
        const int types[] = {137, 107, 79, 159, 116, 65, 43, 7}, counts[] = {70, 84, 56, 210, 70, 77, 175, 7};
        int n = param & 7;
        return util::ArmyAiValue(hero->army) > int64_t(P_CreatureInformation[types[n]].aiValue) * counts[n] * 4 ? 1800
                                                                                                                : 0;
    }
    return 0;
}
void __stdcall Observe(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human)
{
    if (!hero || !item || hero->owner < 0 || hero->owner > 7 ||
        (!Enabled() && !state::Get("wog_objects.native.57.active")))
        return;

    if (item->objectType == 101 && CaveValue(hero, 64) >= 3)
    {
        auto *chest = reinterpret_cast<H3MapitemTreasureChest *>(&item->setup);
        if (!chest->hasArtifact)
            chest->bonus = (std::min)(15u, chest->bonus * 2u);
    }
    int variable = -1, price = 0;
    switch (item->objectType)
    {
    case eObject::SHRINE_OF_MAGIC_INCANTATION:
    case eObject::SHRINE_OF_MAGIC_GESTURE:
    case eObject::SHRINE_OF_MAGIC_THOUGHT:
        variable = 69;
        price = (item->objectType - 87) * 1000;
        break;
    case eObject::WINDMILL:
        variable = 71;
        price = 2000;
        break;
    case eObject::WITCH_HUT:
        variable = 70;
        price = 5000;
        break;
    case eObject::TOWN:
        if (item->town.Get()->IsBuildingBuilt(14))
        {
            variable = 72;
            price = 1000;
        }
        break;
    case eObject::TAVERN:
        variable = 75;
        price = 2000;
        break;
    case eObject::MARLETTO_TOWER:
        variable = 76;
        price = 2000;
        break;
    case eObject::MERCENARY_CAMP:
        variable = 77;
        price = 2000;
        break;
    case eObject::SCHOOL_OF_MAGIC:
        variable = 78;
        price = 4000;
        break;
    case eObject::OBELISK:
        for (int room = 0; room < 4; ++room)
        {
            const auto key = state::PlayerKey(ADVENTURE_CAVE, hero->owner, ("obelisk." + std::to_string(room)).c_str());
            if (state::Get(key) == int(pos.Mixed()) + 1)
            {
                state::Set(key, 0);
                if (util::Random(0, 4))
                {
                    util::ChangeResource(hero, eResource::GOLD, util::Random(5, 10) * 500);
                    const int a[] = {9, 17, 22, 27, 31, 38, 41, 97, 69, 91},
                              b[] = {56, 57, 63, 65, 100, 107, 59, 66, 67, 92};
                    for (int art : {a[util::Random(0, 9)], b[util::Random(0, 9)]})
                        if (util::ArtifactEnabled(art) && hero->backpackCount < 64)
                            hero->GiveBackpackArtifact(H3Artifact(art, -1));
                    Show("obeliskReward", human);
                }
                else
                    Show("obeliskLie", human);
            }
        }
        break;
    default:
        return;
    }
    if (variable < 0 || CaveValue(hero, variable) <= 0)
        return;
    const int64_t gold = int64_t(CaveValue(hero, variable)) * price;
    if (gold > (std::numeric_limits<int>::max)() - util::Resource(hero->owner, 6))
        return;
    if (Ask("sell", human,
            {"item", util::Tr("cave.item." + std::to_string(variable)), "count",
             std::to_string(CaveValue(hero, variable)), "gold", std::to_string(gold)}))
    {
        util::ChangeResource(hero, eResource::GOLD, int(gold));
        CaveValue(hero, variable) = 0;
        Journal(hero, variable, true);
    }
}
void __stdcall EveryDay(Era::TEvent *)
{
    for (int id = 0; id < H3HeroCount::Get(); ++id)
    {
        auto *hero = P_Main->GetHero(id);
        if (hero->owner < 0 || hero->owner > 7)
            continue;

        const auto key = state::HeroKey(ADVENTURE_CAVE, id, "day");
        if (state::IsStamp(key, state::Day()))
            continue;
        state::Stamp(key, state::Day());
        CaveValue(hero, 61) = CaveValue(hero, 68) = 0;
        if ((state::Day() - 1) % 7 == 0)
            CaveValue(hero, 73) = CaveValue(hero, 74) = 0;
    }
}
void __stdcall PlayerTurn(Era::TEvent *)
{
    const int player = P_CurrentPlayerID;
    if (player < 0 || player > 7)
        return;
    const auto notice = state::PlayerKey(ADVENTURE_CAVE, player, "mineNotice");
    if (state::Get(notice))
    {
        state::Set(notice, 0);
        Show("mineStolen", P_Main->IsHuman(player));
    }
    for (int id = 0; id < H3HeroCount::Get(); ++id)
    {
        auto *hero = P_Main->GetHero(id);
        if (hero->owner != player)
            continue;

        if (CaveValue(hero, 63) >= 0)
        {
            const int art = CaveValue(hero, 63);
            CaveValue(hero, 63) = -1;
            if (util::ValidArtifact(art))
                Show("stolen", Human(hero), {"artifact", H3Artifact(art, -1).GetName(), "hero", hero->name});
        }
        if (CaveValue(hero, 62) >= 0)
        {
            CaveValue(hero, 62) = -1;
            Show("ambush", Human(hero));
            H3Position pos(hero->x, hero->y, hero->z);
            Fight(hero, pos, 143, 210, 0, 0, Human(hero));
        }
    }
}
void __stdcall Reset(Era::TEvent *)
{
    battleType = 0;
    battleHero = -1;
    battleWinner = -1;
    machinesRemoved = false;
    actionBusy = false;
}
void __stdcall Load(Era::TEvent *)
{
    battleRound = state::Get("wog_objects.native.57.battle.round", -1);
    for (int side = 0; side < 2; ++side)
        extraCast[side] = state::Get("wog_objects.native.57.battle.extraCast." + std::to_string(side)) != 0;
    if (!state::Get("wog_objects.native.57.battle.active"))
        return;
    battleType = state::Get("wog_objects.native.57.battle.type");
    battleVariant = state::Get("wog_objects.native.57.battle.variant");
    battleHero = state::Get("wog_objects.native.57.battle.hero", -1);
    oldTactics = state::Get("wog_objects.native.57.battle.tactics");
}
class ForkDialog final : public H3Dlg
{
    int choice = -1;

  public:
    ForkDialog(const std::string &left, const std::string &right) : H3Dlg(460, 240, -1, -1, FALSE, FALSE)
    {
        util::AddDialogBackground(*this, 460, 240);
        const auto message = util::Tr("cave.fork", {"left", left, "right", right});
        CreateText(24, 18, 412, 30, util::ObjectName(ADVENTURE_CAVE).c_str(), NH3Dlg::Text::BIG, eTextColor::GOLD, -1,
                   eTextAlignment::MIDDLE_CENTER);
        CreateText(28, 58, 404, 108, message.c_str(), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1,
                   eTextAlignment::MIDDLE_CENTER);
        for (int i = 0; i < 2; ++i)
        {
            const auto label = util::Tr(i ? "cave.right" : "cave.left");
            AddItem(H3DlgCaptionButton::Create(32 + i * 152, 190, 10 + i, "GSPBUT2.def", label.c_str(),
                                               NH3Dlg::Text::SMALL, 0, 1, FALSE, 0, eTextColor::HIGHLIGHT));
        }
        auto *close = H3DlgDefButton::Create(372, 182, 1, "RMG_cncl.def", 0, 1, FALSE, 0);
        if (close)
        {
            close->AddHotkey(eVKey::H3VK_ESCAPE);
            AddItem(close);
        }
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        if (id == 1 || id == 10 || id == 11)
        {
            choice = id == 1 ? -1 : id - 10;
            Stop();
            return FALSE;
        }
        return TRUE;
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_ESCAPE)
        {
            Stop();
            return FALSE;
        }
        return TRUE;
    }
    int Run()
    {
        const int saved = P_WindowManager->resultItemID;
        Start();
        P_WindowManager->resultItemID = saved;
        return choice;
    }
};
std::string PassageName(cave::State data, int first, int end)
{
    std::string text;
    for (int room = first; room < end; ++room)
    {
        const auto name =
            util::Tr(data.Room(room) & 64 ? "cave.emptyLabel" : "cave.room." + std::to_string(data.Room(room) & 3));
        if (text.find(name) != std::string::npos)
            continue;
        if (!text.empty())
            text += ", ";
        text += name;
    }
    return text;
}
int ChoosePassage(cave::State data, H3Position pos)
{
    const bool sign = pos.GetX() % 4 >= 2;
    std::string left = util::Tr("cave.unknown"), right = left;
    if (sign)
    {
        left = PassageName(data, 0, data.Count() > 2 ? 2 : 1);
        right = PassageName(data, data.Count() > 2 ? 2 : 1, data.Count());
        if (pos.GetX() % 4 == 3 && pos.GetY() % 2 == 0)
            std::swap(left, right);
    }
    int branch = ForkDialog(left, right).Run();
    if (branch < 0)
        return -1;
    if (data.Count() == 2)
        return branch;
    if (branch == 1 && data.Count() == 3)
        return 2;
    const int next = ForkDialog(util::Tr("cave.unknown"), util::Tr("cave.unknown")).Run();
    return next < 0 ? -1 : branch * 2 + next;
}
} // namespace

BOOL InitCave(H3MapItem *item)
{
    if (item->objectType == extender::ERA_OBJECT_TYPE)
    {
        item->setup = 0; // Generate chambers lazily on the first visit.
    }
    return TRUE;
}
BOOL VisitCave(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human)
{
    state::Set("wog_objects.native.57.active", 1);

    auto data = Read(item, pos);
    Store(item, hero, pos, data);
    if (data.Count() > 1 && pos.GetX() % 4 == 1 && !data.EntranceCleared())
    {
        if (!Ask("entrance", human) || !Fight(hero, pos, 71, 70, 1, 0, human))
            return TRUE;
        data.ClearEntrance();
        Store(item, hero, pos, data);
    }
    if (!human)
    {
        int best = -1, score = 0;
        for (int r = 0; r < data.Count(); ++r)
        {
            const int s = RoomScore(hero, data.Room(r), r);
            if (s > score)
            {
                score = s;
                best = r;
            }
        }
        if (best >= 0)
        {
            HandleRoom(hero, item, pos, data, best, false);
            Store(item, hero, pos, data);
        }
        return TRUE;
    }
    for (;;)
    {
        int room = 0;
        if (data.Count() > 1)
        {
            room = ChoosePassage(data, pos);
            if (room < 0)
                return TRUE;
        }
        HandleRoom(hero, item, pos, data, room, true);
        Store(item, hero, pos, data);
        if (hero->owner < 0 || !hero->army.HasCreatures() || data.Count() == 1)
            return TRUE;
    }
}
int CaveAiWeight(H3MapItem *item, H3Hero *hero)
{
    cave::State data{static_cast<uint32_t>(state::Get(state::ObjectKey(item, ADVENTURE_CAVE, "state")))};
    if (!data.Initialized())
        return 200;
    const auto pos = state::Position(item);
    if (data.Count() > 1 && pos.x % 4 == 1 && !data.EntranceCleared() &&
        util::ArmyAiValue(hero->army) < int64_t(P_CreatureInformation[71].aiValue) * 70 * 3)
        return 0;
    int best = 0;
    for (int i = 0; i < data.Count(); ++i)
        best = (std::max)(best, RoomScore(hero, data.Room(i), i));
    return best;
}
void RegisterCaveEvents()
{
    Era::RegisterHandler(InstallCaveJournal, "OnAfterWoG");
    extender::RegisterObjectVisitObserver(Observe);
    Era::RegisterHandler(BeforeBattle, "OnBeforeBattleUniversal");
    Era::RegisterHandler(SetupBattle, "OnSetupBattlefield");
    Era::RegisterHandler(PositionBattle, "OnAfterTacticsPhase");
    Era::RegisterHandler(RoundBattle, "OnBattleRound");
    Era::RegisterHandler(BeforeAction, "OnBeforeBattleAction");
    Era::RegisterHandler(AfterAction, "OnBattleActionEnd");
    Era::RegisterHandler(AfterBattle, "OnAfterBattleUniversal");
    Era::RegisterHandler(EveryDay, "OnEveryDay");
    Era::RegisterHandler(PlayerTurn, "OnEveryDay");
    Era::RegisterHandler(Reset, "OnBeforeLoadGame");
    Era::RegisterHandler(Reset, "OnGameLeave");
    Era::RegisterHandler(Load, "OnAfterLoadGame");
    _PI->WriteLoHook(4462998, AfterMelee);
}
} // namespace handlers
} // namespace wog
