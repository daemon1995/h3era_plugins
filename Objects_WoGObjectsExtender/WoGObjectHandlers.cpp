#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include <string>

namespace wog { namespace handlers {

BOOL VisitBasic(H3Hero *hero, H3MapItem *item, int subtype, BOOL human);
void RegisterBasicEvents();
BOOL VisitTrading(H3Hero *hero, H3MapItem *item, int subtype, BOOL human);
BOOL VisitSkullObject(H3Hero *hero, H3MapItem *item, BOOL human);
BOOL VisitCombatObject(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept;
int CombatObjectAiWeight(H3MapItem *item, H3Hero *hero) noexcept;
void RegisterCombatEvents() noexcept;
BOOL VisitEmeraldObject(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept;
int EmeraldAiWeight(H3MapItem *item, H3Hero *hero) noexcept;
void RegisterEmeraldEvents() noexcept;

namespace {
using namespace objects;

bool IsMushroom(const int subtype)
{
    return subtype == MAGIC_AIR_MUSHROOM || subtype == MAGIC_WATER_MUSHROOM ||
           subtype == MAGIC_EARTH_MUSHROOM || subtype == MAGIC_FIRE_MUSHROOM;
}
int MushroomSkill(const int subtype)
{
    switch (subtype)
    {
    case MAGIC_FIRE_MUSHROOM: return 14;
    case MAGIC_AIR_MUSHROOM: return 15;
    case MAGIC_WATER_MUSHROOM: return 16;
    case MAGIC_EARTH_MUSHROOM: return 17;
    default: return -1;
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
    case MYSTERIOUS_DWELLING:
        return hero && state::HeroEver(subtype, hero->id);
    case ALMS_HOUSE:
        return hero && state::HeroEver(subtype, hero->id);
    case ARCANE_TOWER:
    case ALTAR_OF_TRANSFORMATION:
        return owner >= 0 && state::ObjectPlayerPeriod(item, subtype, owner, "week", state::Week());
    case BATTLE_ACADEMY:
        return hero && state::IsStamp(state::HeroKey(subtype, hero->id, "day"), state::Day());
    case DEATH_CHAMBER:
        return owner >= 0 && state::Get(state::ObjectKey(item, subtype, "visited", owner), 0) != 0;
    case FORGOTTEN_SHRINE:
        return hero && state::Get(state::HeroKey(subtype, hero->id, "pendingSpell"), 0) > 0;
    case EMERALD_TOWER:
        return owner >= 0 && state::ObjectPlayerPeriod(item, subtype, owner, "visitedMonth", state::Month());
    case LIVING_SKULL:
        return owner >= 0 && state::ObjectPlayerPeriod(item, subtype, owner, "aiSeen", state::Week());
    default:
        if (IsMushroom(subtype) && hero)
            return state::IsStamp(state::HeroKey(subtype, hero->id, "week"), state::Week());
        return false;
    }
}

int BasicAiWeight(H3MapItem *item, H3Hero *hero)
{
    if (!item || !hero)
        return 0;
    const int subtype = item->objectSubtype;
    const Descriptor *desc = FindDescriptor(subtype);
    if (!desc)
        return 0;
    switch (subtype)
    {
    case MYSTERIOUS_DWELLING:
        return state::HeroEver(subtype, hero->id) ? 0 : desc->defaultAiWeight;
    case ALMS_HOUSE:
        return state::HeroEver(subtype, hero->id) ? 0 : desc->defaultAiWeight;
    case ARCANE_TOWER:
    case ALTAR_OF_TRANSFORMATION:
        return state::ObjectPlayerPeriod(item, subtype, hero->owner, "week", state::Week()) ? 0 : desc->defaultAiWeight;
    case BATTLE_ACADEMY:
        return state::IsStamp(state::HeroKey(subtype, hero->id, "day"), state::Day()) ? 0 : desc->defaultAiWeight;
    default:
        if (IsMushroom(subtype))
        {
            const int skill = MushroomSkill(subtype);
            if (skill >= 0 && hero->secSkill[skill] >= 3)
                return 0;
            return state::IsStamp(state::HeroKey(subtype, hero->id, "week"), state::Week()) ? 0 : desc->defaultAiWeight;
        }
        return desc->defaultAiWeight;
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
        return hero->backpackCount > 0 ? desc->defaultAiWeight : 0;
    if (item->objectSubtype == PALACE_OF_DREAMS)
        return hero->movement > 300 && P_Main->players[hero->owner].playerResources.gold >= 5000
                   ? desc->defaultAiWeight : 0;
    return 0;
}

std::string DetailHint(const H3MapItem *item, const H3Hero *hero, const int interactPlayerId)
{
    const int subtype = item->objectSubtype;
    const std::string base = util::Tr("hint." + std::to_string(subtype));
    if (subtype == FORGOTTEN_SHRINE && hero &&
        state::Get(state::HeroKey(subtype, hero->id, "pendingSpell"), 0) > 0)
        return base + "\n" + util::Tr("hint.blessingPending");
    if (IsVisitedForHint(item, hero, interactPlayerId))
        return base + "\n" + util::Tr("hint.unavailable");
    return base;
}

} // namespace

BOOL Init(H3MapItem *item) noexcept
{
    if (!item || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
    // State that does not fit safely in setup is stored in ERA assoc variables. Keep setup free for
    // compatibility with map editors and other object-property plugins.
    return TRUE;
}

BOOL Visit(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept
{
    if (!item || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
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

BOOL AiWeight(H3MapItem *item, H3Hero *hero, const H3Player *, int &weight, int *, H3Position) noexcept
{
    if (!item || !hero || !objects::IsPortedSubtype(item->objectSubtype))
        return FALSE;
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
        weight = BasicAiWeight(item, hero);
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
    default:
        weight = 0;
        break;
    }
    return TRUE;
}

void RegisterEvents() noexcept
{
    RegisterBasicEvents();
    RegisterCombatEvents();
    RegisterEmeraldEvents();
}

}} // namespace wog::handlers
