#pragma once
#include <unordered_map>
// temp struct to allow add line into protected field "H3Vector<LPCSTR> text;" at 0x1C
struct EditableH3TextFile : public H3TextFile
{
    void AddLine(LPCSTR txt)
    {
        this->text.Add(txt);
    }

    size_t GetLineCount() const noexcept
    {
        return this->text.Size();
    }
};

struct AdditionalProperties
{

    static constexpr LPCSTR PROPERTY_TYPE_JSON = "RMG.%s.properties.%d";
    static constexpr LPCSTR PROPERTY_SUBTYPE_JSON = "RMG.%s.properties.%d.%d";
    static constexpr LPCSTR PROPERTY_COMMON_JSON = "RMG.objectGeneration.%d.%d.properties.%d";
    static constexpr LPCSTR UNIQUE_PROPERTY_FORMAT = "%s_%s_%s";
    std::unordered_map<std::string, std::string> propertiesMap;

    BOOL AddProperty(std::string &other) noexcept;
    const std::string *FindPropertyReplace(LPCSTR other) noexcept;

  public:
    void LoadCommonProperies() noexcept;
    void LoadAdditionalPropertiesFromMods() noexcept;

  public:
    size_t InsertPropertiesIntoObjectsList(H3TextFile *objectsTxt) noexcept;
    static std::string GetMapKey(LPCSTR other) noexcept;
};
