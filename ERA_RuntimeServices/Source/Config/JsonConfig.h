#pragma once
#include "../../../headers/Era/eraJSON.hpp"
#include <algorithm>
#include <climits>
#include <string>
#include <vector>

namespace erm_functions { namespace json_config {
// Read current ERA language data on every call; missing keys and malformed values stay distinct.
// ERA flattens JSON arrays into key.0, key.1, ... . No language-data cache is kept.
template <class Lookup>
bool ReadValue(const char *key, int &value, const Lookup &lookup)
{
    const char *text = lookup(key);
    return text && std::strcmp(key, text) != 0 && EraJS::ParseNumber(text, value);
}

template <class Lookup>
int ReadInt(const char *key, int defaultValue, const Lookup &lookup)
{
    int value;
    return key && *key && ReadValue(key, value, lookup) ? value : defaultValue;
}

// A null output with capacity 0 queries size; otherwise return the number copied (up to capacity).
// Scalars count as one-element arrays. Invalid elements return 0 without partial writes.
template <class Lookup>
int ReadIntArray(const char *key, int *output, int capacity, const Lookup &lookup)
{
    if (!key || !*key || capacity < 0 || (!output && capacity != 0)) return 0;
    std::vector<int> values;
    int total = 0;
    std::string itemKey(key);
    itemKey.reserve(itemKey.size() + 12);
    itemKey.push_back('.');
    const auto prefixLength = itemKey.size();
    for (int index = 0; index < INT_MAX; ++index)
    {
        itemKey.resize(prefixLength);
        itemKey += std::to_string(index);
        const char *text = lookup(itemKey.c_str());
        if (!text || itemKey == text) break;
        int value;
        if (!EraJS::ParseNumber(text, value)) return 0;
        ++total;
        if (output && total <= capacity) values.push_back(value);
    }
    if (total == 0)
    {
        int value;
        if (ReadValue(key, value, lookup))
        {
            total = 1;
            if (output && capacity > 0) values.push_back(value);
        }
    }
    if (!output) return total;
    const int count = static_cast<int>(values.size());
    std::copy_n(values.begin(), count, output);
    return count;
}
} }
