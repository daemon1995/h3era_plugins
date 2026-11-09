#include "pch.h"
#include "../headers/Objects/ObjectDefinitions.h"
#include "../headers/Era/eraModList.hpp"
#include "../headers/Era/GameResources.hpp"
void AdditionalProperties::LoadProperties()
{
    definitions = object_config::CollectDefinitions([](const char *key) { return EraJS::read(key); },
        modList::GetEraModList(modList::CASE_TO_LOWER));

    // Check without loading DEFs or allowing the native MSK loader to use default.msk.
    // Main and preview setups may load the same properties: report each missing file once per process.
    static std::set<std::string> reportedMissing;
    std::set<std::string> files;
    for (const auto &line : definitions)
    {
        const auto begin = line.find_first_not_of(" \t\r\n\f\v");
        if (begin == std::string::npos)
            continue;
        std::string def = line.substr(begin, line.find_first_of(" \t\r\n\f\v", begin) - begin);
        std::transform(def.begin(), def.end(), def.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        files.insert(def);
        const auto extension = def.find_last_of('.');
        files.insert(def.substr(0, extension) + ".msk");
    }
    std::string missing;
    for (const auto &file : files)
        if (!game_resources::Exists(file, true) && reportedMissing.insert(file).second)
            missing += file + "\r\n";
    if (!missing.empty())
    {
        bool translated = false;
        const char *text = EraJS::read("RMG.text.missingObjectResources", translated);
        const std::string message =
            std::string(translated ? text : "Files required by added objects are missing. Install these files and "
                                           "restart the game before generating a map:") + "\r\n\r\n" + missing;
        MessageBoxA(nullptr, message.c_str(), "RMG: missing object resources", MB_OK | MB_ICONWARNING);
    }
}
bool AdditionalProperties::IsOriginalObject(int type, int subtype) const noexcept
{
    const auto found = originalObjects.lower_bound({type, subtype < 0 ? 0 : subtype});
    return found != originalObjects.end() && found->first == type &&
           (subtype == -1 || found->second == subtype);
}
bool AdditionalProperties::IsAddedObject(int type, int subtype) const noexcept
{
    return addedObjects.count({type, subtype}) != 0;
}
int AdditionalProperties::InsertPropertiesIntoObjectsList(H3TextFile *resource, std::vector<std::string> &loadedDefinitions)
{
    loadedDefinitions.clear();
    if (!resource || !resource->Size()) return 0;
    int count = 0;
    if (!object_config::ReadObjectCount((*resource)[0], count) || unsigned(count) >= resource->Size()) return 0;
    std::vector<std::string> lines;
    if (!object_config::ReadObjectDefinitions(resource->begin(), resource->Size(), lines)) return 0;
    object_config::ObjectProperty property;
    if (!originalsCaptured)
    {
        for (size_t i = 1; i < lines.size(); ++i)
            if (object_config::ParseObjectProperty(lines[i], property))
                originalObjects.emplace(property.type, property.subtype);
        originalsCaptured = true;
    }
    object_config::MergeObjectProperties(lines, definitions);
    addedObjects.clear();
    for (size_t i = 1; i < lines.size(); ++i)
        if (object_config::ParseObjectProperty(lines[i], property) &&
            !IsOriginalObject(property.type, property.subtype))
            addedObjects.emplace(property.type, property.subtype);
    auto *editable = static_cast<EditableH3TextFile *>(resource);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const char *line = Era::ToStaticStr(lines[i].c_str());
        if (i < resource->Size()) (*resource)[i] = line;
        else editable->AddLine(line);
    }
    loadedDefinitions.assign(lines.begin() + 1, lines.end());
    return int(lines.size() - 1) - count;
}
