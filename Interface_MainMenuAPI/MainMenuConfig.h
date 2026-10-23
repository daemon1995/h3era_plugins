#pragma once

namespace mainmenu
{
namespace config
{
inline const char *Text(const char *key, const char *fallback)
{
    bool found = false;
    const char *text = EraJS::read(key, found);
    return found ? text : fallback;
}

inline int Int(const char *key, int fallback)
{
    bool found = false;
    const int value = EraJS::readInt(key, found);
    return found ? value : fallback;
}

inline bool Enabled(const char *key, bool fallback)
{
    bool found = false;
    const char *text = EraJS::read(key, found);
    if (!found)
        return fallback;
    if (!strcmp(text, "true"))
        return true;
    if (!strcmp(text, "false"))
        return false;
    int value = 0;
    return EraJS::ParseNumber(text, value) ? value != 0 : fallback;
}
} // namespace config
} // namespace mainmenu
