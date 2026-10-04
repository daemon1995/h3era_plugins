#pragma once
#include "ModJson.h"

namespace hkcategories
{
enum eType : int
{
    ANY_DLG = -1,
    NONE = 0,
    ADV_MAP_DLG,
    HERO_DLG,
    TOWN_DLG,
    COMBAT_DLG,
    MAIN_MENU_DLG,
    OTHER_DLG
};
} // namespace hkcategories
enum class eHelpObjectKind : unsigned char
{
    Text,
    Image,
    Button,
    Link,
    ErmFunction
};
struct HelpObject
{
    eHelpObjectKind kind = eHelpObjectKind::Text;
    int x = 0, y = 0, width = 0, height = 0, zOrder = 0, frame = 0;
    H3String value;
    H3String action;
    BOOL overlay = FALSE;
};
struct Content
{
    H3String text;
    std::vector<HelpObject> objects;
};
struct Category
{
    H3String name;
    Content *content = nullptr;
    virtual ~Category()
    {
        delete content;
    }
};
struct HotKey
{
    hkcategories::eType type = hkcategories::OTHER_DLG;
    H3String keys, name, description;
    int id = -1;
};
struct HotKeysCategory : Category
{
    std::vector<HotKey> hotkeys;
};
struct ModInformation
{
    BOOL hasSomeInfo = FALSE;
    const UINT id;
    H3String name, path;
    ModJsonDocument document;
    HotKeysCategory *hotkeysCategory = nullptr;
    std::vector<Category *> categories;
    ModInformation(LPCSTR folder, UINT id, bool physicalMod = true);
    ~ModInformation();
    ModInformation(const ModInformation &) = delete;
    ModInformation &operator=(const ModInformation &) = delete;
    size_t Size() const noexcept
    {
        return categories.size();
    }
};
