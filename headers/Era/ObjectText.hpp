#pragma once
#include "eraJson.hpp"
#include <cstdio>
#include <string>

namespace EraJS
{
// nullptr means missing; an explicitly empty translation is still an override.
inline const char *ReadObjectText(int type, int subtype, const char *field) noexcept
{
    if (type < 0 || type >= 232 || subtype < 0 || subtype >= 65536 || !field)
        return nullptr;
    char key[128];
    const int length = std::snprintf(key, sizeof(key), "RMG.objectGeneration.%d.%d.%s", type, subtype, field);
    if (length < 0 || size_t(length) >= sizeof(key))
        return nullptr;
    bool found = false;
    const char *text = read(key, found);
    if (found)
        return text;
    return nullptr;
}

inline const char *ReadObjectName(int type, int subtype) noexcept
{
    return ReadObjectText(type, subtype, "name");
}
inline const char *ReadObjectDescription(int type, int subtype) noexcept
{
    return ReadObjectText(type, subtype, "description");
}

// Editor tooltip buffers have room for 199 bytes and a terminator.
inline bool ReadObjectHint(int type, int subtype, const char *nativeName, std::string &result,
                            size_t maximum = 199)
{
    const char *name = ReadObjectName(type, subtype);
    const char *description = ReadObjectDescription(type, subtype);
    if (!name && (!description || !*description))
        return false;
    if (!name)
        name = nativeName;
    result = name ? name : "";
    if (description && *description)
    {
        if (!result.empty())
            result += "\n";
        result += description;
    }
    if (result.size() > maximum)
        result.resize(maximum);
    return true;
}
} // namespace EraJS
