#pragma once
#include "../headers/Objects/DwellingConfig.h"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

namespace editorObjects
{
constexpr int OBJECT_TYPE_LIMIT = 232;
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
    if (values[0] >= OBJECT_TYPE_LIMIT || values[1] >= dwellings::MAX_CAPACITY)
        return false;
    result.type = values[0];
    result.subtype = values[1];
    std::transform(fields[0].begin(), fields[0].end(), fields[0].begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    result.key = fields[0] + "_" + std::to_string(result.type) + "_" + std::to_string(result.subtype);
    return true;
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
} // namespace editorObjects

