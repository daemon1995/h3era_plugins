#include "WoGObjectState.h"

namespace wog { namespace state {

int Day() noexcept { return P_Main ? static_cast<int>(P_Main->date.CurrentDay()) + 1 : 1; }
int Week() noexcept { return (Day() - 1) / 7 + 1; }
int Month() noexcept { return (Day() - 1) / 28 + 1; }

H3Point Position(const H3MapItem *item) noexcept
{
    return item && P_Main ? P_Main->GetCoordinates(const_cast<H3MapItem *>(item)) : H3Point(-1, -1, -1);
}

std::string ObjectKey(const H3MapItem *item, const int subtype, const char *field, const int player)
{
    const H3Point p = Position(item);
    return H3String::Format("wog_objects.native.%d.obj.%d.%d.%d.p%d.%s", subtype, p.x, p.y, p.z, player,
                            field ? field : "value").String();
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
bool IsStamp(const std::string &key, const int stamp) noexcept { return Get(key, -0x3FFFFFFF) == stamp; }
void Stamp(const std::string &key, const int stamp) noexcept { Set(key, stamp); }

bool HeroEver(const int subtype, const int heroId, const char *field) noexcept
{
    return Get(HeroKey(subtype, heroId, field), 0) != 0;
}
void SetHeroEver(const int subtype, const int heroId, const char *field) noexcept { Set(HeroKey(subtype, heroId, field), 1); }
bool ObjectPlayerPeriod(const H3MapItem *item, const int subtype, const int player, const char *field,
                        const int period) noexcept
{
    return IsStamp(ObjectKey(item, subtype, field, player), period);
}
void SetObjectPlayerPeriod(const H3MapItem *item, const int subtype, const int player, const char *field,
                           const int period) noexcept
{
    Stamp(ObjectKey(item, subtype, field, player), period);
}

}} // namespace wog::state
