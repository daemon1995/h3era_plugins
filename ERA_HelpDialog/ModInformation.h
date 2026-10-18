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
// Text pointers borrow permanent EraJS storage; the parsed document does not copy it.
struct HelpObject
{
    eHelpObjectKind kind = eHelpObjectKind::Text;
    int x = 0, y = 0, width = 0, height = 0, zOrder = 0, frame = 0;
    LPCSTR value = h3_NullString;
    LPCSTR action = h3_NullString;
    BOOL overlay = FALSE;
};
struct Content
{
    LPCSTR text = h3_NullString;
    std::string generatedText; // Only generated plugin listings own their article text.
    std::vector<HelpObject> objects;
};
struct Category
{
    LPCSTR name = h3_NullString;
    Content *content = nullptr;
    virtual ~Category()
    {
        delete content;
    }
};
struct HotKey
{
    hkcategories::eType type = hkcategories::OTHER_DLG;
    LPCSTR keys = h3_NullString, name = h3_NullString, description = h3_NullString;
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
    LPCSTR name = h3_NullString;
    H3String path;
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
