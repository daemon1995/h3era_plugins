#include "pch.h"
#include "../headers/Objects/ObjectDefinitions.h"
#include "../headers/Era/eraModList.hpp"
void AdditionalProperties::LoadProperties()
{
    definitions = object_config::CollectDefinitions([](const char *key) { return EraJS::read(key); },
        modList::GetEraModList(modList::CASE_TO_LOWER));
}
int AdditionalProperties::InsertPropertiesIntoObjectsList(H3TextFile *resource, std::vector<std::string> &loadedDefinitions)
{
    loadedDefinitions.clear();
    if (!resource || !resource->Size()) return 0;
    int count = 0;
    if (!object_config::ReadObjectCount((*resource)[0], count) || unsigned(count) >= resource->Size()) return 0;
    std::vector<std::string> lines;
    if (!object_config::ReadObjectDefinitions(resource->begin(), resource->Size(), lines)) return 0;
    object_config::MergeObjectProperties(lines, definitions);
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
