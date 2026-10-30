#include <algorithm>
#include <limits>
#include <map>

#include "WoGObjectHero.h"
#include "WoGObjectResources.h"
#include "WoGObjectUtils.h"
#include "WoGObjectsExtender.h"
#include "WoGObjectPolicy.h"

namespace wog
{
namespace util
{
namespace
{
constexpr int FIRST_PICTURE_ID = 30729;
constexpr int CHOOSE_PICTURE_OR_CANCEL = 10;
using SetSelectablePictures = void(__stdcall *)(int);

int ChoosePicturesImpl(const char *text, const H3PictureVector &pictures)
{
    if (!pictures.Size())
        return -1;
    const HMODULE era = GetModuleHandleA("era.dll");
    const auto setSelectablePictures =
        era ? reinterpret_cast<SetSelectablePictures>(GetProcAddress(era, "_SetDialog8SelectablePicsMask")) : nullptr;
    if (setSelectablePictures)
        setSelectablePictures((1 << pictures.Size()) - 1);
    Era::TComplexDialogOpts options{};
    options.msgType = CHOOSE_PICTURE_OR_CANCEL;
    H3Messagebox::Display(text, pictures, options.value);
    const int choice = H3WindowManager::Get()->ClickedItemID() - FIRST_PICTURE_ID;
    return choice >= 0 && choice < static_cast<int>(pictures.Size()) ? choice : -1;
}
} // namespace

std::string Tr(const std::string &suffix, const std::vector<std::string> &parameters)
{
    std::string key = "wog_objects.native." + suffix;
    for (const auto &entry : {std::make_pair("skull.", "wog.12.native."),
                              std::make_pair("altar.", "wog.14.native."),
                              std::make_pair("arcane.", "wog.104.native."),
                              std::make_pair("junk.", "wog.108.native."),
                              std::make_pair("palace.", "wog.11.native.")})
        if (suffix.find(entry.first) == 0)
            key = entry.second + suffix.substr(libc::strlen(entry.first));
    if (suffix == "arcane.visited") key = "wog.104.visited";
    if (suffix == "altar.visited") key = "wog.14.visited";
    if (suffix == "altar.noMon") key = "wog.14.noMon";
    if (suffix == "skull.tired") key = "wog.12.search.tired";
    if (suffix == "skull.noGold") key = "wog.12.search.noGold";
    bool found = false;
    EraJS::read(key, found);
    return found ? Era::tr(key.c_str(), parameters) : suffix;
}
std::string ObjectName(const int subtype)
{
    const char *script = subtype == 50 ? "12" : subtype == 20 ? "14" : subtype == 9 ? "104" : nullptr;
    if (script)
    {
        const std::string key = std::string("wog.") + script + ".objName";
        bool found = false;
        EraJS::read(key, found);
        if (found) return Era::tr(key.c_str());
    }
    if (const char *name = extender::GetObjectName(ERA_OBJECT_TYPE, subtype))
        if (*name)
            return name;
    const std::string key = H3String::Format("RMG.objectGeneration.%d.%d.name", ERA_OBJECT_TYPE, subtype).String();
    bool found = false;
    EraJS::read(key, found);
    if (found)
        return Era::tr(key.c_str());
    return H3String::Format("WoG object %d", subtype).String();
}
void SetHintText(const std::string &text) noexcept
{
    libc::sprintf(h3_TextBuffer, "%s", text.c_str());
}

int ChoosePictures(const char *text, const H3PictureVector &pictures)
{
    return ChoosePicturesImpl(text, pictures);
}
bool AddDialogBackground(H3Dlg &dialog, int width, int height, bool statusBar)
{
    auto *canvas = H3LoadedPcx16::Create(width, height);
    if (!canvas)
        return false;
    canvas->FillRectangle(0, 0, width, height, 35, 30, 24);
    H3PcxLoader texture(NH3Dlg::Assets::DIBOXBACK);
    // HelpUI::AddSafeBackground uses bounded tiles: H3API BackgroundRegion overruns DIBOXBACK in HD.
    if (texture.Get() && texture->width > 0 && texture->height > 0)
        for (int y = 0; y < height; y += texture->height)
            for (int x = 0; x < width; x += texture->width)
                texture->DrawToPcx16(0, 0, (std::min)(texture->width, width - x),
                                     (std::min)(texture->height, height - y), canvas, x, y, FALSE);
    H3DefLoader frame(NH3Dlg::Assets::DLGBOX);
    if (frame.Get())
        canvas->FrameRegion(0, 0, width, height, statusBar, 0, FALSE);
    const bool added = dialog.AddBackground(canvas) != FALSE;
    canvas->Destroy();
    return added;
}

int Random(const int minValue, const int maxValue) noexcept
{
    return maxValue <= minValue ? minValue : H3Random::MultiplayerRandom(minValue, maxValue);
}

int ReadReceiver(const std::string &command, const int initial)
{
    constexpr const char *key = "wog_objects.native.bridge.result";
    const int saved = Era::GetAssocVarIntValue(key);
    Era::SetAssocVarIntValue(key, initial);
    Era::ExecErmCmd(command.c_str());
    const int value = Era::GetAssocVarIntValue(key);
    Era::SetAssocVarIntValue(key, saved);
    return value;
}
void Receiver(const std::string &command)
{
    Era::ExecErmCmd(command.c_str());
}
int Resource(const int player, const int resource)
{
    if (player < 0 || player > 7 || resource < 0 || resource > 7)
        return 0;
    return resource < 7 ? P_Main->players[player].playerResources.asArray[resource] : native::Mithril(player);
}
bool ChangeResource(H3Hero *hero, const eResource resource, const int amount)
{
    if (!hero || hero->owner < 0 || hero->owner > 7 || resource < 0 || resource > 7)
        return false;
    const int64_t result = int64_t(Resource(hero->owner, resource)) + amount;
    if (result < 0 || result > (std::numeric_limits<int>::max)())
        return false;
    if (resource < 7)
        hero->AddResource(resource, amount);
    else
        native::SetMithril(hero->owner, int(result));
    return true;
}
bool GiveExperience(H3Hero *hero, const int amount)
{
    if (!hero || amount <= 0 || hero->experience > (std::numeric_limits<int>::max)() - amount)
        return false;
    // WoG common.cpp::AddExp: commit experience, release the active dialog, run normal level-ups.
    hero->experience += amount;
    THISCALL_2(void, 0x50D7B0, H3WindowManager::Get(), 0);
    THISCALL_1(void, 0x4DA990, hero);
    return true;
}
bool GivePrimary(H3Hero *hero, const int skill, const int amount)
{
    if (!hero || skill < 0 || skill > 3)
        return false;
    const int old = static_cast<unsigned char>(hero->primarySkill[skill]);
    if (old + amount < 0 || old + amount > 125)
        return false;
    // HE:F changes these bytes directly. Keep the same cap without invoking the parser.
    hero->primarySkill[skill] = static_cast<INT8>(old + amount);
    return static_cast<unsigned char>(hero->primarySkill[skill]) == old + amount;
}
bool LearnSkill(H3Hero *hero, const int skill, const int level)
{
    if (!hero || skill < 0 || skill >= 28 || level < 0 || level > 3)
        return false;
    const int old = hero->secSkill[skill];
    if (old == level)
        return true;
    if (!level)
        native::RemoveSecondarySkill(*hero, skill);
    else if (level > old)
        hero->LearnSecondarySkill(skill, level - old);
    else
        hero->secSkill[skill] = static_cast<INT8>(level);
    return hero->secSkill[skill] == level;
}
bool LearnSpell(H3Hero *hero, const int spell)
{
    if (!hero || spell < 0 || spell >= 70 || spell >= int(H3SpellCount::Get()))
        return false;
    // HE:M sets both the learned and currently available spell arrays.
    hero->learnedSpells[spell] = hero->availableSpell[spell] = TRUE;
    return true;
}
bool ConfirmPictures(const char *text, const H3PictureVector &pictures)
{
    Era::TComplexDialogOpts options{};
    options.msgType = 2;
    H3Messagebox::Display(text, pictures, options.value);
    return H3WindowManager::Get()->ClickedOK() != FALSE;
}
int StackAt(const int hex) noexcept
{
    if (!P_CombatManager || hex < 0 || hex >= 187)
        return -1;
    const auto &square = P_CombatManager->squares[hex];
    return square.stackSide >= 0 && square.stackSide < 2 && square.stackIndex >= 0 && square.stackIndex < 21
               ? square.stackSide * 21 + square.stackIndex
               : -1;
}
void CastCreatureSpell(H3CombatCreature *caster, int spell, int hex, int school, int power)
{
    if (!caster || !P_CombatManager || caster->numberAlive <= 0)
        return;
    auto *combat = P_CombatManager->Get();
    const int side = combat->currentMonSide, slot = combat->currentMonIndex;
    combat->currentMonSide = caster->side;
    combat->currentMonIndex = caster->sideIndex;
    combat->CastSpell(spell, hex, 1, -1, school, power);
    combat->currentMonSide = side;
    combat->currentMonIndex = slot;
}
void CastHeroSpell(int side, int spell, int hex, int school)
{
    if (!P_CombatManager || side < 0 || side > 1 || !P_CombatManager->hero[side])
        return;
    auto *combat = P_CombatManager->Get();
    const int previous = combat->currentActiveSide;
    combat->currentActiveSide = side;
    combat->CastSpell(spell, hex, 0, -1, -1, school);
    combat->currentActiveSide = previous;
}
bool Fight(H3Hero *hero, H3Position position, int creature, int count)
{
    if (!hero || !P_AdventureManager || !ValidCreature(creature) || count <= 0)
        return false;
    // Use the same native WoG battle wrapper as the other object extenders, including AI/network setup.
    H3Army defenders;
    defenders.Clear();
    defenders.AddStack(creature, count, 0);
    const int seed = H3Random::MultiplayerRandom(1, 0x7FFFFFFF);
    return THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), position.Mixed(), hero, &hero->army, ePlayer::NONE,
                       static_cast<H3Town *>(nullptr), static_cast<H3Hero *>(nullptr), &defenders, seed, TRUE,
                       TRUE) != 0;
}
namespace
{
// WoG structs.h::_Hero_::Flags; H3API leaves this byte private/unnamed.
#pragma pack(push, 1)
struct HeroFlagsView
{
    BYTE prefix[0x48];
    BYTE flags;
};
#pragma pack(pop)
static_assert(offsetof(HeroFlagsView, flags) == 0x48, "WoG hero flags layout");
} // namespace
bool TacticsEnabled(const H3Hero *hero) noexcept
{
    return hero && (reinterpret_cast<const HeroFlagsView *>(hero)->flags & 2) != 0;
}
void SetTactics(H3Hero *hero, bool enabled) noexcept
{
    if (hero)
    {
        auto &flags = reinterpret_cast<HeroFlagsView *>(hero)->flags;
        flags = enabled ? flags | 2 : flags & ~2;
    }
}
void ClearObstacles()
{
    if (!P_CombatManager)
        return;
    // ERA stdlib RemoveBattleObstacle: engine erases/reindexes and releases DEF references.
    for (int i = int(P_CombatManager->obstacleInfo.Count()) - 1; i >= 0; --i)
        THISCALL_2(void, 0x466710, P_CombatManager->Get(), i);
}
void PlaceObstacle(int object, int hex)
{
    if (!P_CombatManager || object < 0 || object > 90 || hex < 0 || hex >= 187)
        return;
    H3Obstacle obstacle{};
    obstacle.info = &P_ObstacleInfo[object];
    obstacle.def = H3LoadedDef::Load(obstacle.info->defName);
    if (!obstacle.def)
        return;
    obstacle.anchorHex = static_cast<UINT8>(hex);
    obstacle.ownerSide = -1;
    obstacle.featureTriggered = 1;
    obstacle.animationIndex = UINT32(-1);
    auto *combat = P_CombatManager->Get();
    if (!combat->obstacleInfo.Add(obstacle))
    {
        obstacle.def->Dereference();
        return;
    }
    THISCALL_5(void, 0x466590, combat, &obstacle, combat->obstacleInfo.Count() - 1, hex, 2);
}
void PlaceBattleStack(H3CombatCreature *stack, int hex)
{
    if (!P_CombatManager || !stack || stack->numberAlive <= 0 || stack->position < 0 ||
        stack->position >= 187 || hex < 0 || hex >= 187)
        return;
    // Native placement check also adjusts the anchor when a double-wide stack needs room.
    if (!THISCALL_4(int, 0x446960, stack, hex, 1, &hex) || hex == stack->position)
        return;
    auto *combat = P_CombatManager->Get();
    THISCALL_1(void, 0x43E970, stack); // Unlink auras at the previous position.
    THISCALL_1(void, 0x43EB30, stack); // Release bindings affected by relocation.
    THISCALL_2(void, 0x468310, combat, stack);
    THISCALL_3(void, 0x4683A0, combat, stack, hex);
    stack->position = hex;
    THISCALL_1(void, 0x43E790, stack); // Rebuild adjacent protection auras.
}
void RevealArea(int x, int y, int z, int player, int radius, bool reveal)
{
    if (!P_Main || player < 0 || player > 7 || radius < 0)
        return;
    // common.cpp::ShowArea / HideArea: the engine preserves team visibility and reveals owned sites again.
    if (reveal)
        THISCALL_7(void, 0x49CDD0, P_Main->Get(), x, y, z, player, radius, 0);
    else
    {
        THISCALL_6(void, 0x49D040, P_Main->Get(), x, y, z, player, radius);
        THISCALL_1(void, 0x4C7910, P_Main->Get());
    }
}
int CleanPrimary(const H3Hero *hero, const int skill)
{
    if (!hero || skill < 0 || skill > 3)
        return 0;
    // Same native remove/equip sequence as ERA stdlib GetHeroPrimarySkillsWithoutArts.
    // This is used for human training quotes only; AI evaluation must not fire AE events.
    H3Hero *mutableHero = const_cast<H3Hero *>(hero);
    H3Artifact equipped[19];
    for (int slot = 0; slot < 19; ++slot)
        equipped[slot] = hero->bodyArtifacts[slot];
    for (int slot = 0; slot < 19; ++slot)
        if (mutableHero->bodyArtifacts[slot].id >= 0)
            mutableHero->RemoveArtifact(slot);
    const int result = static_cast<unsigned char>(hero->primarySkill[skill]);
    for (int slot = 0; slot < 19; ++slot)
        if (equipped[slot].id >= 0)
            mutableHero->GiveArtifact(equipped[slot], slot);
    return result;
}

bool ValidCreature(const int id) noexcept
{
    return id >= 0 && id < H3CreatureCount::Get();
}
bool ValidArtifact(const int id) noexcept
{
    return id >= 0 && id < H3ArtifactCount::Get();
}
bool ArtifactEnabled(const int id) noexcept
{
    return ValidArtifact(id) && !H3ArtifactSetup::Get()[id].disabled && !native::ArtifactBanned(id);
}
int CreatureGoldCost(const int id) noexcept
{
    return ValidCreature(id) ? (std::max)(0, P_CreatureInformation[id].cost.gold) : 0;
}
long long ArmyAiValue(const H3Army &army) noexcept
{
    long long result = 0;
    for (int i = 0; i < 7; ++i)
        if (ValidCreature(army.type[i]) && army.count[i] > 0)
            result +=
                static_cast<long long>((std::max)(0, P_CreatureInformation[army.type[i]].aiValue)) * army.count[i];
    return result;
}

bool CanAddArmy(const H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept
{
    std::map<int, int64_t> unique;
    for (const auto &s : stacks)
        if (ValidCreature(s.first) && s.second > 0)
            unique[s.first] += s.second;
    int freeSlots = 0;
    for (int i = 0; i < 7; ++i)
        if (army.type[i] < 0 || army.count[i] <= 0)
            ++freeSlots;
    for (const auto &s : unique)
    {
        int64_t total = s.second;
        bool exists = false;
        for (int i = 0; i < 7; ++i)
            if (army.type[i] == s.first && army.count[i] > 0)
            {
                exists = true;
                total += army.count[i];
            }
        if (total > (std::numeric_limits<int>::max)())
            return false;
        if (!exists && --freeSlots < 0)
            return false;
    }
    return true;
}
namespace
{
// WoG/crexpo.h CrExpo (16 bytes). Storage is owned and saved by WoG, never by this plugin.
struct StackExperience
{
    int experience, count;
    UINT32 flags, location;
};
static_assert(sizeof(StackExperience) == 16, "WoG stack experience layout");
StackExperience *Experience(H3Army &army, int slot)
{
    int type = 0;
    UINT32 location = 0;
    CDECL_4(void, 0x71A1B7, &army, slot, &type, &location);
    return CDECL_2(StackExperience *, 0x718617, type, location);
}
} // namespace
bool AddArmy(H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept
{
    if (!CanAddArmy(army, stacks))
        return false;
    const H3Army before = army;
    H3Army updated = army;
    for (const auto &s : stacks)
    {
        if (!ValidCreature(s.first) || s.second <= 0)
            continue;
        if (!updated.AddStack(s.first, s.second, -1))
            return false;
    }
    // HE:C2 mode 0 dilutes an experienced stack when untrained recruits join it.
    for (int slot = 0; slot < 7; ++slot)
        if (updated.type[slot] >= 0 && updated.count[slot] > 0 &&
            (updated.type[slot] != before.type[slot] || updated.count[slot] != before.count[slot]))
            if (auto *experience = Experience(army, slot))
            {
                const int oldCount = before.type[slot] == updated.type[slot] ? (std::max)(0, before.count[slot]) : 0;
                experience->experience = int(int64_t(experience->experience) * oldCount / updated.count[slot]);
                experience->count = updated.count[slot];
                experience->flags =
                    (experience->flags & ~UINT32(0x1FE0)) | ((UINT32(updated.type[slot]) << 5) & 0x1FE0);
            }
    army = updated;
    return true;
}
bool TransferArmyStack(H3Army &source, int slot, H3Army &destination) noexcept
{
    if (&source == &destination || slot < 0 || slot >= 7 ||
        !CanAddArmy(destination, {{source.type[slot], source.count[slot]}}))
        return false;
    const int creature = source.type[slot], count = source.count[slot];
    if (!ValidCreature(creature) || count <= 0)
        return false;
    int target = -1;
    for (int i = 0; i < 7; ++i)
        if (destination.type[i] == creature && destination.count[i] > 0)
        {
            target = i;
            break;
        }
    if (target < 0)
        for (int i = 0; i < 7; ++i)
            if (destination.type[i] < 0 || destination.count[i] <= 0)
            {
                target = i;
                break;
            }
    if (target < 0)
        return false;
    const int oldCount = destination.type[target] == creature ? (std::max)(0, destination.count[target]) : 0;
    int sourceType = 0, targetType = 0;
    UINT32 sourceLocation = 0, targetLocation = 0;
    CDECL_4(void, 0x71A1B7, &source, slot, &sourceType, &sourceLocation);
    CDECL_4(void, 0x71A1B7, &destination, target, &targetType, &targetLocation);
    // CrExpoSet::HComb merges weighted experience AND stack artifacts before committing the armies.
    if (CDECL_8(int, 0x719A95, sourceType, targetType, sourceLocation, targetLocation, creature, creature, count,
                oldCount) < 0)
        return false;
    source.type[slot] = -1;
    source.count[slot] = 0;
    destination.type[target] = creature;
    destination.count[target] = oldCount + count;
    return true;
}
void TransformArmyStack(H3Hero *hero, int slot, int creature) noexcept
{
    if (!hero || slot < 0 || slot >= 7 || !ValidCreature(creature))
        return;
    if (hero->army.type[slot] == creature)
        return;
    if (auto *experience = Experience(hero->army, slot))
    {
        // HE:C0's mode 0 resets experience on a species change while preserving stack artifacts.
        experience->experience = 0;
        experience->count = hero->army.count[slot];
        experience->flags = (experience->flags & ~UINT32(0x1FE0)) | ((UINT32(creature) << 5) & 0x1FE0);
    }
    hero->army.type[slot] = creature;
}

int CountBackpackArtifact(const H3Hero *hero, const int artifactId, const int subtype) noexcept
{
    if (!hero)
        return 0;
    int count = 0;
    for (int i = 0; i < hero->backpackCount && i < 64; ++i)
        if (hero->backpackArtifacts[i].id == artifactId &&
            (subtype < 0 || hero->backpackArtifacts[i].subtype == subtype))
            ++count;
    return count;
}
bool RemoveBackpackArtifact(H3Hero *hero, const int artifactId, const int subtype) noexcept
{
    if (!hero)
        return false;
    for (int i = 0; i < hero->backpackCount && i < 64; ++i)
    {
        if (hero->backpackArtifacts[i].id == artifactId &&
            (subtype < 0 || hero->backpackArtifacts[i].subtype == subtype))
        {
            hero->RemoveBackpackArtifact(i);
            return true;
        }
    }
    return false;
}

int ChoosePrimary(const char *text, const int excluded)
{
    H3PictureVector pics;
    int ids[4] = {-1, -1, -1, -1};
    int n = 0;
    for (int i = 0; i < 4; ++i)
        if (i != excluded)
        {
            ids[n++] = i;
            pics.Add(H3PictureCategories::PrimarySkill(static_cast<ePrimary>(i), 1));
        }
    const int index = ChoosePicturesImpl(text, pics);
    return index >= 0 && index < n ? ids[index] : -1;
}
int ChooseArmySlot(const H3Hero *hero, const char *text, bool)
{
    if (!hero)
        return -1;
    H3PictureVector pics;
    int slots[7];
    int n = 0;
    for (int i = 0; i < 7; ++i)
        if (ValidCreature(hero->army.type[i]) && hero->army.count[i] > 0)
        {
            slots[n++] = i;
            pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(hero->army.type[i]), hero->army.count[i]));
        }
    const int index = ChoosePicturesImpl(text, pics);
    return index >= 0 && index < n ? slots[index] : -1;
}
int ChooseTown(const char *text, const int preferred)
{
    (void)preferred;
    // Selectable native message boxes support eight pictures. Page 1 has towns 0..7;
    // page 2 has Conflux plus Neutral. Return -1 for Neutral and -2 for cancel.
    {
        H3PictureVector pics;
        for (int t = 0; t < 8; ++t)
            pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(NativeTownCreature(t, 0, false)), 1));
        const int index = ChoosePicturesImpl(text, pics);
        if (index >= 0)
            return index;
        if (!H3Messagebox::Choice(Tr("common.moreTowns").c_str()))
            return -2;
    }
    H3PictureVector pics;
    pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(NativeTownCreature(8, 0, false)), 1));
    // Faerie Dragon is only a visual marker for the Neutral destination; no fixed creature is implied.
    pics.Add(H3PictureCategories::Creature(eCreature::FAERIE_DRAGON, 1));
    const int index = ChoosePicturesImpl(text, pics);
    if (index == 0)
        return 8;
    if (index == 1)
        return -1;
    return -2;
}

int NativeTownCreature(const int town, const int level, const bool upgraded) noexcept
{
    if (town < 0 || town > 8 || level < 0 || level > 6)
        return -1;
    const int id = town * 14 + level * 2 + (upgraded ? 1 : 0);
    return ValidCreature(id) ? id : -1;
}
int FirstCreatureAtOrBelowFightValue(const int town, const int level, const int maxFightValue) noexcept
{
    int best = -1;
    for (int i = 0; i < H3CreatureCount::Get(); ++i)
    {
        const auto &c = P_CreatureInformation[i];
        if (c.town == town && c.level == level && c.fightValue <= maxFightValue)
        {
            if (best < 0 || c.fightValue > P_CreatureInformation[best].fightValue)
                best = i;
        }
    }
    return best;
}
bool IsLegacyEnabled(const H3MapItem *item, const int) noexcept
{
    return policy::Handles(item);
}
bool OptionEnabled(int optionId) noexcept
{
    return WoGObjectsExtender::IsWoGOptionEnabled(optionId);
}

} // namespace util
} // namespace wog
