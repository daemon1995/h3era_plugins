#include <string>

#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include "WoGObjectPolicy.h"

extern PatcherInstance *_PI;

namespace wog
{
namespace handlers
{

BOOL VisitBasic(H3Hero *hero, H3MapItem *item, int subtype, BOOL human);
void RegisterBasicEvents();
int EvaluateBasicAi(H3MapItem *, H3Hero *);
BOOL VisitTrading(H3Hero *hero, H3MapItem *item, int subtype, BOOL human);
int JunkAiWeight(H3Hero *hero);
BOOL VisitSkullObject(H3Hero *hero, H3MapItem *item, BOOL human);
BOOL VisitCombatObject(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept;
int CombatObjectAiWeight(H3MapItem *item, H3Hero *hero) noexcept;
void RegisterCombatEvents() noexcept;
BOOL VisitEmeraldObject(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept;
int EmeraldAiWeight(H3MapItem *item, H3Hero *hero) noexcept;
void RegisterEmeraldEvents() noexcept;
BOOL VisitLearningObject(H3Hero *hero, H3MapItem *item, BOOL human);
int LearningAiWeight(H3MapItem *item, H3Hero *hero);
void RegisterLearningEvents();
BOOL VisitPotion(H3Hero *hero, int subtype, BOOL human);
int PotionAiWeight(H3Hero *hero, int subtype);
void RegisterPotionEvents();
BOOL VisitCave(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human);
int CaveAiWeight(H3MapItem *item, H3Hero *hero);
void RegisterCaveEvents();
BOOL InitCave(H3MapItem *);

namespace
{
using namespace objects;

_LHF_(ResetHeroObjects)
{
    // Reading each scenario header also resets all heroes. It must not touch map assoc state.
    if (!policy::GameActive())
        return EXEC_DEFAULT;
    // Hero::Reset (also used by WOG_Hook_HeroReset): EDX still contains the incoming hero ID.
    const int heroId = c->edx;
    if (heroId >= 0 && heroId < H3HeroCount::Get())
        state::ResetHero(heroId);
    return EXEC_DEFAULT;
}

void __stdcall InitializeMapSetups(Era::TEvent *)
{
    if (!P_Main || policy::LoadingGame())
        return;
    const int size = P_Main->mainSetup.mapSize;
    for (int z = 0; z <= int(P_Main->mainSetup.hasUnderground); ++z)
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                auto *tile = P_Main->mainSetup.GetMapItem(x, y, z);
                H3MapItem item;
                item.objectType = tile->objectType;
                item.objectSubtype = tile->objectSubtype;
                item.setup = tile->setup;
                H3Hero *hero = nullptr;
                if (item.objectType == eObject::HERO)
                {
                    hero = P_Main->GetHero(item.setup);
                    if (!hero || !hero->objectBelow)
                        continue;
                    // Hero::Show leaves the underlying subtype on the tile (also used by ExtendedMonoliths).
                    item.objectType = hero->objectTypeUnder;
                    item.setup = hero->objectBelowSetup;
                }
                if ((item.objectType != extender::WOG_OBJECT_TYPE && item.objectType != extender::ERA_OBJECT_TYPE) ||
                    !policy::Handles(&item))
                    continue;
                if (hero)
                    hero->objectBelowSetup = 0;
                else
                    tile->setup = 0;
            }
}

bool IsMushroom(const int subtype)
{
    return subtype == MAGIC_AIR_MUSHROOM || subtype == MAGIC_WATER_MUSHROOM || subtype == MAGIC_EARTH_MUSHROOM ||
           subtype == MAGIC_FIRE_MUSHROOM;
}
int MushroomSkill(const int subtype)
{
    switch (subtype)
    {
    case MAGIC_FIRE_MUSHROOM:
        return 14;
    case MAGIC_AIR_MUSHROOM:
        return 15;
    case MAGIC_WATER_MUSHROOM:
        return 16;
    case MAGIC_EARTH_MUSHROOM:
        return 17;
    default:
        return -1;
    }
}

bool IsVisitedForHint(const H3MapItem *item, const H3Hero *hero, const int player)
{
    if (!item)
        return false;
    const int subtype = item->objectSubtype;
    const int owner = hero ? hero->owner : player;
    switch (subtype)
    {
    case VENUS_FOUNTAIN:
    case BERSERKER_FOUNTAIN:
        return hero && state::Get(state::HeroKey(subtype, hero->id, "doses")) >= 3;
    case SCHOOL_OF_WIZARDRY:
        return hero && state::IsStamp(state::HeroKey(subtype, hero->id, "day"), state::Day());
    case SUMMONING_STONES:
        return state::ObjectDay(item, state::Day());
    case MYSTERIOUS_DWELLING:
        return hero && state::HeroEver(subtype, hero->id);
    case ALMS_HOUSE:
        return hero && state::HeroEver(subtype, hero->id);
    case ARCANE_TOWER:
    case ALTAR_OF_TRANSFORMATION:
        return owner >= 0 && state::ObjectPlayerPeriod(item, subtype, owner, "week", state::Week());
    case BATTLE_ACADEMY:
        return hero && state::IsStamp(state::HeroKey(subtype, hero->id, "day"), state::Day());
    case DEATH_CHAMBER: {
        const int cleared = setup::DeathChamber{item->setup}.ClearedDay();
        return cleared && (!util::OptionEnabled(235) || state::Day() - cleared < 28);
    }
    case FORGOTTEN_SHRINE:
        return hero && state::Get(state::HeroKey(subtype, hero->id, "pendingSpell"), 0) > 0;
    case EMERALD_TOWER:
        return owner >= 0 && state::ObjectPlayerPeriod(item, subtype, owner, "visitedMonth", state::Month());
    default:
        if (IsMushroom(subtype) && hero)
            return state::IsStamp(state::HeroKey(subtype, hero->id, "week"), state::Week());
        return false;
    }
}

int TradingAiWeight(H3MapItem *item, H3Hero *hero)
{
    if (!item || !hero)
        return 0;
    const Descriptor *desc = FindDescriptor(item->objectSubtype);
    if (!desc)
        return 0;
    if (item->objectSubtype == JUNK_MERCHANT)
        return JunkAiWeight(hero);
    if (item->objectSubtype == PALACE_OF_DREAMS)
        return !state::IsStamp(state::HeroKey(PALACE_OF_DREAMS, hero->id, "aiDay"), state::Day()) &&
                       hero->movement > 300 && P_Main->players[hero->owner].playerResources.gold >= 5000
                   ? desc->defaultAiWeight
                   : 0;
    return 0;
}

std::string DetailHint(const H3MapItem *item, const H3Hero *hero, const int interactPlayerId)
{
    const int subtype = item->objectSubtype;
    const std::string base = util::Tr("hint." + std::to_string(subtype));
    if (subtype == FORGOTTEN_SHRINE && hero && state::Get(state::HeroKey(subtype, hero->id, "pendingSpell"), 0) > 0)
        return base + "\n" + util::Tr("hint.blessingPending");
    if (IsVisitedForHint(item, hero, interactPlayerId))
        return base + "\n" + util::Tr("hint.unavailable");
    return base;
}

} // namespace

void AppendVisitHint(H3MapItem *item, const H3Hero *hero, int player, BOOL rightClick) noexcept
{
    switch (item->objectSubtype)
    {
    case SCHOOL_OF_WIZARDRY:
    case MYSTERIOUS_DWELLING:
    case ALMS_HOUSE:
    case BATTLE_ACADEMY:
    case FORGOTTEN_SHRINE:
    case MAGIC_AIR_MUSHROOM:
    case MAGIC_WATER_MUSHROOM:
    case MAGIC_EARTH_MUSHROOM:
    case MAGIC_FIRE_MUSHROOM:
        if (!hero)
            return;
        break;
    case ARCANE_TOWER:
    case ALTAR_OF_TRANSFORMATION:
    case SUMMONING_STONES:
    case DEATH_CHAMBER:
    case EMERALD_TOWER:
        if (!hero && (player < 0 || player > 7))
            return;
        break;
    default:
        return; // Repeatable services do not have a meaningful binary visitation status.
    }
    const std::string text = std::string(h3_TextBuffer) + (rightClick ? "\n\n" : " ") +
                             P_GeneralText->GetText(IsVisitedForHint(item, hero, player) ? 354 : 355);
    util::SetHintText(text);
}

BOOL Init(H3MapItem *item) noexcept
{
    if (!item || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
    if (item->objectSubtype == objects::ADVENTURE_CAVE)
        return InitCave(item);
    // Only object-local state lives in setup. Hero/player effects remain in map-bound assoc memory.
    item->setup = 0;
    return TRUE;
}

BOOL Visit(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept
{
    if (!item || !hero || hero->owner < 0 || hero->owner > 7 || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
    state::ObjectPositionScope position(item, pos);
    switch (item->objectSubtype)
    {
    case MYSTERIOUS_DWELLING:
    case ARCANE_TOWER:
    case MAGIC_AIR_MUSHROOM:
    case MAGIC_WATER_MUSHROOM:
    case ALTAR_OF_TRANSFORMATION:
    case MAGIC_EARTH_MUSHROOM:
    case ALMS_HOUSE:
    case MAGIC_FIRE_MUSHROOM:
    case BATTLE_ACADEMY:
        return VisitBasic(hero, item, item->objectSubtype, human);
    case JUNK_MERCHANT:
    case PALACE_OF_DREAMS:
        return VisitTrading(hero, item, item->objectSubtype, human);
    case LIVING_SKULL:
        return VisitSkullObject(hero, item, human);
    case DEATH_CHAMBER:
    case FORGOTTEN_SHRINE:
    case MIRROR_OF_HOME_WAY:
        return VisitCombatObject(hero, item, pos, human);
    case EMERALD_TOWER:
        return VisitEmeraldObject(hero, item, pos, human);
    case SCHOOL_OF_WIZARDRY:
    case SUMMONING_STONES:
        return VisitLearningObject(hero, item, human);
    case VENUS_FOUNTAIN:
    case BERSERKER_FOUNTAIN:
        return VisitPotion(hero, item->objectSubtype, human);
    case ADVENTURE_CAVE:
        return VisitCave(hero, item, pos, human);
    default:
        return FALSE;
    }
}

BOOL Hint(H3MapItem *item, const H3Hero *hero, int interactPlayerId, BOOL rightClick) noexcept
{
    if (!item || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
    if (!rightClick)
        util::SetHintText(util::ObjectName(item->objectSubtype));
    else
    {
        std::string text = util::ObjectName(item->objectSubtype);
        const std::string detail = DetailHint(item, hero, interactPlayerId);
        if (!detail.empty())
            text += "\n" + detail;
        util::SetHintText(text);
    }
    return TRUE;
}

BOOL AiWeight(H3MapItem *item, H3Hero *hero, const H3Player *, int &weight, int *, H3Position pos) noexcept
{
    weight = 0;
    if (!item || !hero || hero->owner < 0 || hero->owner > 7 || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
    state::ObjectPositionScope position(item, pos);
    switch (item->objectSubtype)
    {
    case MYSTERIOUS_DWELLING:
    case ARCANE_TOWER:
    case MAGIC_AIR_MUSHROOM:
    case MAGIC_WATER_MUSHROOM:
    case ALTAR_OF_TRANSFORMATION:
    case MAGIC_EARTH_MUSHROOM:
    case ALMS_HOUSE:
    case MAGIC_FIRE_MUSHROOM:
    case BATTLE_ACADEMY:
        weight = EvaluateBasicAi(item, hero);
        break;
    case JUNK_MERCHANT:
    case PALACE_OF_DREAMS:
        weight = TradingAiWeight(item, hero);
        break;
    case LIVING_SKULL:
        weight = 0;
        break;
    case DEATH_CHAMBER:
    case FORGOTTEN_SHRINE:
    case MIRROR_OF_HOME_WAY:
        weight = CombatObjectAiWeight(item, hero);
        break;
    case EMERALD_TOWER:
        weight = EmeraldAiWeight(item, hero);
        break;
    case SCHOOL_OF_WIZARDRY:
    case SUMMONING_STONES:
        weight = LearningAiWeight(item, hero);
        break;
    case VENUS_FOUNTAIN:
    case BERSERKER_FOUNTAIN:
        weight = PotionAiWeight(hero, item->objectSubtype);
        break;
    case ADVENTURE_CAVE:
        weight = CaveAiWeight(item, hero);
        break;
    default:
        weight = 0;
        break;
    }
    return TRUE;
}

void RegisterEvents() noexcept
{
    _PI->WriteLoHook(0x4D89B8, ResetHeroObjects);
    Era::RegisterHandler(InitializeMapSetups, "OnAfterErmInstructions");
    RegisterBasicEvents();
    RegisterCombatEvents();
    RegisterEmeraldEvents();
    RegisterLearningEvents();
    RegisterPotionEvents();
    RegisterCaveEvents();
}

} // namespace handlers
} // namespace wog
