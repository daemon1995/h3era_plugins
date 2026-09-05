#pragma once

#include "HelpDialogDependencies.h"
#include "ModJson.h"

#include <vector>

namespace hkcategories
{
enum eType : int
{
    ANY_DLG = -1,
    NONE,
    ADV_MAP_DLG,
    HERO_DLG,
    TOWN_DLG,
    COMBAT_DLG,
    MAIN_MENU_DLG,
    OTHER_DLG
};

} // namespace hkcategories
struct Content;
struct Category;
struct ModInformation;
struct HotKeysCategory;

// Universal content model for future mod pages. Parsing is intentionally not
// implemented yet; pages can later render these nodes into a mod body buffer.
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
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int zOrder = 0;
    H3String value;
    H3String action;
    BOOL overlay = FALSE;
};

struct Category
{
    H3LoadedPcx16 *iconPcx = nullptr;
    H3String name;

    Content *content = nullptr;
    //	const UINT id;
  public:
    //	Category(LPCSTR jsonKeyName/*, const UINT id*/);

    ~Category();

  public:
    const Category &Content() const noexcept;
    void ShowContent() const noexcept;

    // void
};

struct HotKey
{
    hkcategories::eType type;
    H3String keys;
    H3String name;
    H3String description;
};

struct HotKeysCategory : public Category
{
    std::vector<HotKey> hotkeys;
};

struct Content
{
    H3String text;
    std::vector<HelpObject> objects;
};
struct LastActiveDlgModInfo
{
    int modId = -1;
    int categoryId = -1;
    int scrollBarPos = 0;
    const ModInformation *mod = nullptr;
};

struct ModInformation
{
  protected:
  private:
    static LastActiveDlgModInfo lastActiveModInfo;

  public:
    // to check if some data is stored in the json
    BOOL hasSomeInfo;
    const UINT id;
    H3String name;
    H3String path;
    ModJsonDocument document;
    int m_lastActiveCategoryId = -1;
    Category *activeCategory = nullptr;
    HotKeysCategory *hotkeysCategory = nullptr;
    std::vector<Category *> categories;

  public:
    ModInformation(LPCSTR modFolderName, const UINT id);
    virtual ~ModInformation();

  private:
    HotKeysCategory *CreateHotkeysCategory() const noexcept;
    Category *CreateNativeCategory(const int index) const noexcept;

  public:
    //	virtual void Activate();
    const Category &ActiveCategory() const noexcept;
    size_t Size() const noexcept;
    void GetInfo();
    void SetVisible(const BOOL state);
    void StoreModInfoAsActive() const noexcept;
    static const LastActiveDlgModInfo &GetLastActiveModInfo() noexcept;

    //		void DrawCategory(const int id) const noexcept;
};
