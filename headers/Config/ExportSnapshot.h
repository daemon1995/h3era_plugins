#pragma once
#include "ExportModel.h"
#include "../EraPluginsAPI/ConfigExportAPI.h"
#include <map>
#include <stdexcept>
#include <cctype>
namespace jsonExport {
struct Snapshot
{
    std::vector<jsonExport::Category> categories;
    std::map<std::string, size_t> categoryIds;
    std::map<std::pair<std::string, std::string>, size_t> records;
    std::map<std::string, std::string> fileOwners;
    std::string error;
};
inline bool SafeName(const char *name)
{
    if (!name || !*name) return false;
    for (const char *p = name; *p; ++p)
        if (!(*p >= 'a' && *p <= 'z') && !(*p >= 'A' && *p <= 'Z') &&
            !(*p >= '0' && *p <= '9') && *p != '_' && *p != '-') return false;
    return true;
}
inline void CheckCompatible(const Json &current, const Json &incoming)
{
    if (current.is_null() || (incoming.is_object() && incoming.empty())) return;
    if (current.is_object() && incoming.is_object())
    {
        for (auto it = incoming.begin(); it != incoming.end(); ++it)
            if (current.contains(it.key())) CheckCompatible(current[it.key()], it.value());
    }
    else if (current != incoming) throw std::runtime_error("Conflicting export field owners");
}
inline int AcceptObjects(Snapshot &snapshot, const char *key, const char *stem, const char *identity,
    int original, const Json &textJson, const Json &dataJson, const Json &defaultsJson)
{
    try
    {
        if (!SafeName(key) || !SafeName(stem) || !identity || !*identity) throw std::runtime_error("Invalid export identity");
        if (!textJson.is_object() || !dataJson.is_object() || !defaultsJson.is_object())
            throw std::runtime_error("Export record must be a JSON object");
        auto categoryId = snapshot.categoryIds.find(key);
        if (categoryId == snapshot.categoryIds.end())
        {
            std::string filename = stem;
            for (auto &character : filename) character = char(std::tolower(static_cast<unsigned char>(character)));
            const auto owner = snapshot.fileOwners.find(filename);
            if (owner != snapshot.fileOwners.end() && owner->second != key) throw std::runtime_error("Duplicate export filename");
            snapshot.fileOwners[filename] = key;
            const size_t index = snapshot.categories.size();
            snapshot.categories.push_back({key, stem, false});
            categoryId = snapshot.categoryIds.emplace(key, index).first;
        }
        auto &category = snapshot.categories[categoryId->second];
        if (category.fileStem != stem) throw std::runtime_error("Conflicting category filename");
        const auto recordKey = std::make_pair(std::string(key), std::string(identity));
        auto found = snapshot.records.find(recordKey);
        if (found == snapshot.records.end())
        {
            const size_t index = category.records.size();
            category.records.push_back({original != 0});
            category.records.back().identity = identity;
            found = snapshot.records.emplace(recordKey, index).first;
        }
        auto &record = category.records[found->second];
        if (record.original != (original != 0)) throw std::runtime_error("Conflicting record origin");
        CheckCompatible(record.text, textJson); CheckCompatible(record.data, dataJson); CheckCompatible(record.rmgDefaults, defaultsJson);
        record.text.merge_patch(textJson); record.data.merge_patch(dataJson); record.rmgDefaults.merge_patch(defaultsJson);
        category.supportsData |= !dataJson.empty() || !defaultsJson.empty();
        return 1;
    }
    catch (const std::exception &error) { snapshot.error = error.what(); return 0; }
    catch (...) { snapshot.error = "Cannot collect export snapshot"; return 0; }
}
}
