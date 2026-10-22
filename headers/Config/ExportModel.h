#pragma once
#include "../json.hpp"
#include "../Objects/RmgSettingsExport.h"
#include <array>
#include <string>
#include <type_traits>
#include <vector>

namespace jsonExport
{
using Json = nlohmann::ordered_json;

inline Json &At(Json &root, const std::string &path)
{
    Json *node = &root;
    size_t start = 0, end;
    do
    {
        end = path.find('.', start);
        const auto field = path.substr(start, end == std::string::npos ? end : end - start);
        if (node->is_array())
        {
            if (field.empty() || field.find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error("Invalid export array index");
            node = &(*node)[static_cast<size_t>(std::stoul(field))];
        }
        else node = &(*node)[field];
        start = end + 1;
    } while (end != std::string::npos);
    return *node;
}
template <typename T, typename Encode> void Put(Json &root, const std::string &path, const T &value, Encode encode)
{
    if constexpr (std::is_convertible_v<T, const char *>)
    {
        if (value) At(root, path) = encode(value);
    }
    else if constexpr (std::is_array_v<T>)
    {
        Json array = Json::array();
        for (const auto &entry : value)
        {
            if constexpr (std::is_convertible_v<decltype(entry), const char *>) array.push_back(encode(entry));
            else array.push_back(static_cast<long long>(entry));
        }
        At(root, path) = std::move(array);
    }
    else At(root, path) = static_cast<long long>(value);
}
inline void PutRmgOverrides(Json &root, const std::string &path, unsigned mask, const int *values)
{
    constexpr const char *fields[] = {"enabled", "map", "zone", "value", "density"};
    for (int field = 0; field < rmgExport::FIELD_COUNT; ++field)
        if (mask & (1U << field)) At(root, path + "." + fields[field]) = values[field];
}
struct Record
{
    bool original = true;
    Json text, data;
    Json rmgDefaults;
    std::string identity;
};
struct Options
{
    bool mergeTextAndData = false;
    bool ignoreRmgDefaults = true;
};
enum class OutputKind { TEXT, DATA, COMBINED };
struct Output
{
    OutputKind kind;
    std::string fileName;
    Json json;
};
struct Selection
{
    bool original = true, mods = true, text = true, data = true;
    bool Includes(const Record &record) const { return record.original ? original : mods; }
};
struct Category
{
    std::string key;
    std::string fileStem;
    bool supportsData;
    std::vector<Record> records;

    // Count identities, not fields, DEF variants, array capacity or gaps in subtype IDs.
    std::array<int, 4> Counts(const Selection &selection, bool ignoreRmgDefaults = true) const
    {
        std::array<int, 4> counts{};
        for (const auto &record : records)
        {
            const bool hasData = !record.data.empty() || (!ignoreRmgDefaults && !record.rmgDefaults.empty());
            if ((selection.text && !record.text.empty()) || (selection.data && hasData))
                ++counts[record.original ? 0 : 1];
            if (selection.Includes(record))
            {
                if (!record.text.empty()) ++counts[2];
                if (hasData) ++counts[3];
            }
        }
        return counts;
    }
    Json Build(const Selection &selection, bool data, bool ignoreRmgDefaults = true) const
    {
        Json result;
        if (data ? !selection.data || !supportsData : !selection.text) return result;
        for (const auto &record : records)
            if (selection.Includes(record))
            {
                if (data && !ignoreRmgDefaults && !record.rmgDefaults.empty()) result.merge_patch(record.rmgDefaults);
                const auto &source = data ? record.data : record.text;
                if (!source.empty()) result.merge_patch(source);
            }
        return result;
    }
    std::vector<Output> Outputs(const Selection &selection, const Options &options) const
    {
        std::vector<Output> outputs;
        auto text = Build(selection, false, options.ignoreRmgDefaults);
        auto data = Build(selection, true, options.ignoreRmgDefaults);
        if (options.mergeTextAndData)
        {
            if (!data.empty()) text.merge_patch(data);
            if (!text.empty()) outputs.push_back({OutputKind::COMBINED, std::string(fileStem) + ".json", std::move(text)});
        }
        else
        {
            if (!text.empty()) outputs.push_back({OutputKind::TEXT, std::string(fileStem) + "Text.json", std::move(text)});
            if (!data.empty()) outputs.push_back({OutputKind::DATA, std::string(fileStem) + "Data.json", std::move(data)});
        }
        return outputs;
    }
};
} // namespace jsonExport
