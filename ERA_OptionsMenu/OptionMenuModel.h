#pragma once

#include "OptionRegistry.h"
#include <map>
#include <functional>

namespace era_options
{
struct MenuCategory
{
    std::string name;
    std::vector<EOption *> options;
    bool collapsed = false;
    int order = 0;
};

struct MenuPage
{
    std::string name;
    std::vector<MenuCategory> categories;
    std::vector<EOption *> options;
};

struct MenuMod
{
    std::string folder;
    std::string name;
    bool expanded = false;
    std::vector<MenuPage> pages;
};

// Section is only a fallback for old category descriptions; page is independent.
const std::string &OptionCategory(const OptionDefinition &definition);
std::wstring NormalizeOptionSearch(const std::string &text, unsigned codePage);
bool MatchesOptionSearch(const EOption &option, const std::wstring &query, unsigned codePage);

// Rows mark vertical cell starts; each column has independent cell heights.
int LastGridStart(const std::vector<int> &rowHeights, int viewportHeight);

enum class OptionCellKind { Empty, Checkbox, Radio, Heading, Category };
struct OptionCell
{
    EOption *option = nullptr;
    OptionCellKind kind = OptionCellKind::Empty;
    int choice = -1;
    const MenuCategory *category = nullptr;
    int categoryIndex = -1;
};
struct OptionGridRow { OptionCell left, right; int height = 0; int leftHeight = 0; int rightHeight = 0; };
// Fill complete categories down the left column, then the right before scrolling.
// Split a category only if necessary; a complete radio group stays in one column.
std::vector<OptionGridRow> BuildOptionGrid(const std::vector<MenuCategory> &categories,
    const std::function<int(const OptionCell &)> &measureHeight, int viewportHeight);
const std::string &OptionCellName(const OptionCell &cell);
struct MenuDialogSize { int width; int height; };
MenuDialogSize OptionsDialogSize(int screenWidth, int screenHeight, bool fullScreen);
bool DialogShadowFits(int x, int y, int width, int height, int screenWidth, int screenHeight);
enum class ScrollArea { None, Mods, Options };
ScrollArea ScrollAreaAt(int mouseX, int mouseY, int sidebarWidth, int sidebarTop, int sidebarHeight,
                        int optionX, int optionTop, int optionWidth, int optionHeight);

class OptionMenuModel final
{
    std::vector<MenuMod> mods;
    std::vector<EOption *> allOptions;
    std::vector<EOption *> searchMatches;
    std::vector<MenuCategory> searchCategories;
    std::vector<MenuCategory> emptyCategories;
    std::vector<EOption *> empty;
    std::map<std::pair<std::string, std::string>, int> pagePositions;
    std::string query;
    std::wstring normalizedQuery;
    unsigned codePage;
    int selectedMod = -1;
    int selectedPage = -1;
    int searchPosition = 0;

  public:
    explicit OptionMenuModel(const OptionRegistry &registry, unsigned codePage);
    const std::vector<MenuMod> &Mods() const { return mods; }
    int SelectedMod() const { return selectedMod; }
    int SelectedPage() const { return selectedPage; }
    const std::string &Query() const { return query; }
    bool IsSearching() const { return !normalizedQuery.empty(); }
    const std::vector<EOption *> &Options() const;
    const std::vector<MenuCategory> &Categories() const;
    void ClickMod(int mod);
    void SelectPage(int mod, int page);
    void ToggleCategory(int category);
    void SetQuery(const std::string &text);
    int ScrollPosition() const;
    void SetScrollPosition(int position);
};
} // namespace era_options
