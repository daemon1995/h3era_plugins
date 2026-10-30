#pragma once
#include <string>
#include <set>
#include <utility>
#include <vector>
struct EditableH3TextFile : public H3TextFile
{
    void AddLine(LPCSTR line) { text.Add(line); }
};
struct AdditionalProperties
{
    std::vector<std::string> definitions;
    // Identity is a type/subtype pair, not a DEF or the largest subtype.
    std::set<std::pair<int, int>> originalObjects, addedObjects;
    bool originalsCaptured = false;
    void LoadProperties();
    bool IsOriginalObject(int type, int subtype) const noexcept;
    bool IsAddedObject(int type, int subtype) const noexcept;
    int InsertPropertiesIntoObjectsList(H3TextFile *objectsTxt, std::vector<std::string> &loadedDefinitions);
};
