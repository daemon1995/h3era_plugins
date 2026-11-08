#pragma once

namespace mainmenu
{
namespace config
{
inline const char *Text(const char *key, const char *fallback)
{
    bool found = false;
    const char *text = EraJS::read(key, found);
    return found && *text ? text : fallback;
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
    int result = EraJS::readInt(key, found);
    return found ? result != 0 : fallback;
}
} // namespace config
} // namespace mainmenu
