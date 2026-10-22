#pragma once
#include "DwellingConfig.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

namespace object_config
{
inline bool ReadObjectCount(const char *text, int &count) noexcept
{
    if (!text)
        return false;
    char *end = nullptr;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (end == text || errno == ERANGE || value < 0 || value > INT_MAX)
        return false;
    while (*end && std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    if (*end)
        return false;
    count = static_cast<int>(value);
    return true;
}

struct ObjectProperty
{
    std::string key;
    int type = 0;
    int subtype = -1;
};

inline bool ParseObjectProperty(const std::string &line, ObjectProperty &result)
{
    std::array<std::string, 9> fields;
    const char *current = line.c_str();
    for (auto &field : fields)
    {
        while (*current && std::isspace(static_cast<unsigned char>(*current)))
            ++current;
        const char *begin = current;
        while (*current && !std::isspace(static_cast<unsigned char>(*current)))
            ++current;
        if (begin == current)
            return false;
        field.assign(begin, current);
    }
    while (*current && std::isspace(static_cast<unsigned char>(*current)))
        ++current;
    if (*current)
        return false;
    for (int column = 1; column <= 4; ++column)
    {
        size_t length = 9;
        if (column <= 2)
            length = 48;
        if (fields[column].size() != length || fields[column].find_first_not_of("01") != std::string::npos)
            return false;
    }
    int values[4]{};
    for (int column = 5; column < 9; ++column)
    {
        char *end = nullptr;
        errno = 0;
        const long value = std::strtol(fields[column].c_str(), &end, 10);
        if (*end || end == fields[column].c_str() || errno == ERANGE || value < 0 || value > INT_MAX)
            return false;
        values[column - 5] = static_cast<int>(value);
    }
    if (!IsObjectType(values[0]) || !IsObjectSubtype(values[1]))
        return false;
    result.type = values[0];
    result.subtype = values[1];
    std::transform(fields[0].begin(), fields[0].end(), fields[0].begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    result.key = fields[0] + "_" + std::to_string(result.type) + "_" + std::to_string(result.subtype);
    return true;
}

// Read only the declared records. Unsupported definitions are excluded before
// either native loader receives the list; trailing resource lines stay inactive.
inline bool ReadObjectDefinitions(const char *const *rows, size_t size, std::vector<std::string> &result)
{
    int count = 0;
    if (!rows || !size || !ReadObjectCount(rows[0], count) || size_t(count) >= size)
        return false;
    std::vector<std::string> lines(1);
    ObjectProperty property;
    for (int i = 1; i <= count; ++i)
        if (rows[i] && ParseObjectProperty(rows[i], property))
            lines.emplace_back(rows[i]);
    lines[0] = std::to_string(lines.size() - 1);
    result.swap(lines);
    return true;
}

// Definitions may be sparse or contain several DEFs for the same subtype.
inline std::array<int, OBJECT_TYPE_LIMIT> SubtypeCounts(const std::vector<std::string> &definitions)
{
    std::array<int, OBJECT_TYPE_LIMIT> counts{};
    ObjectProperty property;
    for (const auto &line : definitions)
        if (ParseObjectProperty(line, property))
            counts[property.type] = (std::max)(counts[property.type], property.subtype + 1);
    return counts;
}

// Preserve native definitions; replace matching DEF/type/subtype entries, append new ones once.
inline void MergeObjectProperties(std::vector<std::string> &lines, const std::vector<std::string> &additions)
{
    if (lines.empty())
        return;
    std::unordered_map<std::string, size_t> indices;
    ObjectProperty property;
    for (size_t i = 1; i < lines.size(); ++i)
        if (ParseObjectProperty(lines[i], property))
            indices[property.key] = i;
    for (const auto &line : additions)
    {
        if (!ParseObjectProperty(line, property))
            continue;
        const auto found = indices.find(property.key);
        if (found != indices.end())
            lines[found->second] = line;
        else
        {
            indices[property.key] = lines.size();
            lines.push_back(line);
        }
    }
    lines[0] = std::to_string(lines.size() - 1);
}
} // namespace object_config

namespace object_config
{
// Mod priority is resolved once, before either host touches native memory.
// Explicit per-subtype definitions override legacy mod lists; the final entry wins.
template <class Lookup, class Mods>
std::vector<std::string> CollectDefinitions(const Lookup &lookup, const Mods &mods)
{
    std::vector<std::string> result;
    ObjectProperty parsed;
    const auto append = [&](const std::string &key, int type = -1, int subtype = -1) {
        const char *line = lookup(key.c_str());
        if (!line || key == line) return false;
        if (ParseObjectProperty(line, parsed) && (type < 0 || (parsed.type == type && parsed.subtype == subtype)))
            result.emplace_back(line);
        return true;
    };
    for (const auto &mod : mods)
    {
        const std::string prefix = "RMG." + mod + ".properties.";
        for (int i = 0; append(prefix + std::to_string(i)); ++i) {}
        for (int group = 0; ; ++group)
        {
            int i = 0;
            for (; append(prefix + std::to_string(group) + "." + std::to_string(i)); ++i) {}
            if (!i) break;
        }
    }
    for (int type = 0; type < OBJECT_TYPE_LIMIT; ++type)
        for (int subtype = 0; subtype < PROPERTY_SUBTYPE_SCAN_LIMIT; ++subtype)
        {
            const std::string prefix = "RMG.objectGeneration." + std::to_string(type) + "." + std::to_string(subtype) + ".properties.";
            for (int i = 0; append(prefix + std::to_string(i), type, subtype); ++i) {}
        }
    return result;
}
}
