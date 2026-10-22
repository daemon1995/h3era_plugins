#pragma once
#include <string>
#include <vector>
struct EditableH3TextFile : public H3TextFile
{
    void AddLine(LPCSTR line) { text.Add(line); }
};
struct AdditionalProperties
{
    std::vector<std::string> definitions;
    void LoadProperties();
    int InsertPropertiesIntoObjectsList(H3TextFile *objectsTxt, std::vector<std::string> &loadedDefinitions);
};
