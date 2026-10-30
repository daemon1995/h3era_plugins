#include "WoGObjectState.h"

namespace wog
{
namespace state
{
namespace
{
const H3MapItem *positionItem = nullptr;
H3Point positionValue(-1, -1, -1);
} // namespace

ObjectPositionScope::ObjectPositionScope(const H3MapItem *item, H3Position position) noexcept
    : previousItem(positionItem), previousPosition(positionValue)
{
    positionItem = item;
    positionValue = H3Point(position.GetX(), position.GetY(), position.GetZ());
}
ObjectPositionScope::~ObjectPositionScope()
{
    positionItem = previousItem;
    positionValue = previousPosition;
}

int Day() noexcept
{
    return P_Main ? static_cast<int>(P_Main->date.CurrentDay()) + 1 : 1;
}
int Week() noexcept
{
    return (Day() - 1) / 7 + 1;
}
int Month() noexcept
{
    return (Day() - 1) / 28 + 1;
}
void ResetMapRandomSeed() noexcept
{
    Set("wog_objects.native.randomSeed", Era::RandomRangeWithFreeParam(1, 0x7FFFFFFF, 0x574F474F));
}
int MapRandomSeed() noexcept
{
    const std::string key = "wog_objects.native.randomSeed";
    int seed = Get(key);
    if (!seed)
    {
        ResetMapRandomSeed();
        seed = Get(key);
    }
    return seed;
}

H3Point Position(const H3MapItem *item) noexcept
{
    if (item && item == positionItem)
        return positionValue;
    if (!item || !P_Main)
        return H3Point(-1, -1, -1);
    const auto *begin = P_Main->mainSetup.mapitems;
    const size_t count =
        size_t(P_Main->mainSetup.mapSize) * P_Main->mainSetup.mapSize * (P_Main->mainSetup.hasUnderground ? 2 : 1);
    const uintptr_t address = reinterpret_cast<uintptr_t>(item), base = reinterpret_cast<uintptr_t>(begin);
    if (address < base || address - base >= count * sizeof(H3MapItem) || (address - base) % sizeof(H3MapItem))
        return H3Point(-1, -1, -1);
    return P_Main->GetCoordinates(const_cast<H3MapItem *>(item));
}

std::string ObjectKey(const H3MapItem *item, const int subtype, const char *field, const int player)
{
    const H3Point p = Position(item);
    return H3String::Format("wog_objects.native.%d.%d.obj.%d.%d.%d.p%d.%s", item ? int(item->objectType) : -1,
                            subtype, p.x, p.y, p.z, player,
                            field ? field : "value")
        .String();
}
std::string HeroKey(const int subtype, const int heroId, const char *field)
{
    return H3String::Format("wog_objects.native.%d.hero.%d.%s", subtype, heroId, field ? field : "value").String();
}
std::string PlayerKey(const int subtype, const int player, const char *field)
{
    return H3String::Format("wog_objects.native.%d.player.%d.%s", subtype, player, field ? field : "value").String();
}
int Get(const std::string &key, const int defaultValue) noexcept
{
    if (key.empty())
        return defaultValue;
    const std::string presentKey = key + ".present";
    if (Era::GetAssocVarIntValue(presentKey.c_str()) == 0)
        return defaultValue;
    return Era::GetAssocVarIntValue(key.c_str());
}
void Set(const std::string &key, const int value) noexcept
{
    if (key.empty())
        return;
    Era::SetAssocVarIntValue(key.c_str(), value);
    Era::SetAssocVarIntValue((key + ".present").c_str(), 1);
}
bool IsStamp(const std::string &key, const int stamp) noexcept
{
    return Get(key, -0x3FFFFFFF) == stamp;
}
void Stamp(const std::string &key, const int stamp) noexcept
{
    Set(key, stamp);
}

bool HeroEver(const int subtype, const int heroId, const char *field) noexcept
{
    return Get(HeroKey(subtype, heroId, field), 0) != 0;
}
void SetHeroEver(const int subtype, const int heroId, const char *field) noexcept
{
    Set(HeroKey(subtype, heroId, field), 1);
}
void ResetHero(const int heroId) noexcept
{
    const auto clear = [heroId](int subtype, const char *field) {
        const auto key = HeroKey(subtype, heroId, field);
        const auto presentKey = key + ".present";
        if (!Era::GetAssocVarIntValue(presentKey.c_str()))
            return;
        Era::SetAssocVarIntValue(key.c_str(), 0);
        // Restore each reader's default, including the cave's -1 sentinels.
        Era::SetAssocVarIntValue(presentKey.c_str(), 0);
    };
    for (int subtype : {3, 24})
        clear(subtype, "visited");
    for (int subtype : {32, 62})
        clear(subtype, "day");
    for (int subtype : {14, 18, 23, 29})
        for (const char *field : {"week", "oldLevel", "addedSkill", "expiresWeek"})
            clear(subtype, field);
    for (int subtype : {-1, 2, 48})
        clear(subtype, "doses");
    clear(22, "pendingSpell");
    clear(33, "aiDay");
    for (const char *field : {"day", "rogueServices", "ambushClient", "stolenArtifact", "wealthLesson",
                             "retaliationLesson", "spellcastingLesson", "voodooLesson", "studiedToday",
                             "stoneTablet", "magicRoot", "herbalExtract", "tradeCargo", "poolBlessings",
                             "altarPowers", "wine", "huntingTrophy", "oreSample", "ancientCoin"})
        clear(57, field);
    for (int variable = 63; variable <= 78; ++variable)
        clear(57, ("journal." + std::to_string(variable)).c_str());
}
bool ObjectPlayerPeriod(const H3MapItem *item, const int subtype, const int player, const char *field,
                        const int period) noexcept
{
    if (item && (((subtype == 9 || subtype == 20) && std::string(field) == "week") ||
                 (subtype == 65 && std::string(field) == "visitedMonth")))
        return player >= 0 && player < 8 && setup::PeriodVisitors{item->setup}.IsPeriod(period) &&
               THISCALL_2(bool, 0x529B70, item, player);
    return IsStamp(ObjectKey(item, subtype, field, player), period);
}
void SetObjectPlayerPeriod(H3MapItem *item, const int subtype, const int player, const char *field,
                           const int period, H3Hero *hero) noexcept
{
    if (item && (((subtype == 9 || subtype == 20) && std::string(field) == "week") ||
                 (subtype == 65 && std::string(field) == "visitedMonth")))
    {
        setup::PeriodVisitors value{item->setup};
        if (player < 0 || player > 7 || !value.UpdatePeriod(period))
            return;
        item->setup = value.word;
        SetObjectAsVisited(item, player, hero);
        return;
    }
    Stamp(ObjectKey(item, subtype, field, player), period);
}
void StoreSetup(H3MapItem *item, uint32_t word, H3Hero *hero) noexcept
{
    if (!item)
        return;
    item->setup = word;
    const H3Point p = Position(item);
    if (P_Main && p.x >= 0 && p.y >= 0 && p.z >= 0)
    {
        auto *tile = P_Main->mainSetup.GetMapItem(p.x, p.y, p.z);
        if (tile && tile->objectType == item->objectType && tile->objectSubtype == item->objectSubtype)
            tile->setup = word;
        if (hero && hero->objectBelow && hero->objectTypeUnder == item->objectType &&
            hero->x == p.x && hero->y == p.y && hero->z == p.z)
            hero->objectBelowSetup = word;
    }
}
void SetObjectAsVisited(H3MapItem *item, int player, H3Hero *hero) noexcept
{
    if (!item || (item->objectType != 63 && item->objectType != 141) || player < 0 || player > 7)
        return;
    THISCALL_2(void, 0x4FC620, item, player);
    StoreSetup(item, item->setup, hero);
}
bool ObjectDay(const H3MapItem *item, int day) noexcept
{
    return item && day > 0 && setup::DayStamp{item->setup}.Day() == day;
}
void SetObjectDay(H3MapItem *item, int day, H3Hero *hero) noexcept
{
    auto value = setup::DayStamp::At(day);
    StoreSetup(item, value.word, hero);
}

} // namespace state
} // namespace wog
