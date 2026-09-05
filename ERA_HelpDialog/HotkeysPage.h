#pragma once

#include "HelpPage.h"
#include "ModInformation.h"

namespace main
{

namespace hotkeys
{
enum eItem
{
    CATEGORY_FIRST = 400,
    CATEGORY_ALL = CATEGORY_FIRST,
    CATEGORY_EVERYWHERE,
    CATEGORY_ADVENTURE,
    CATEGORY_HERO,
    CATEGORY_TOWN,
    CATEGORY_COMBAT,
    CATEGORY_OTHER,
    CATEGORY_LAST = CATEGORY_OTHER,
    CATEGORY_COUNT = CATEGORY_LAST - CATEGORY_FIRST + 1
};
}

class HotkeysCategoriesPage final : public HelpPage
{
    H3DlgCaptionButton *buttons[hotkeys::CATEGORY_COUNT] = {};
    int activeCategory = hotkeys::CATEGORY_ALL;

  public:
    HotkeysCategoriesPage(int x, int y, int width, int height, H3Dlg *dialog);

    BOOL IsCategory(int itemId) const noexcept;
    int ActiveCategory() const noexcept;
    void SetActiveCategory(int itemId) noexcept;
};

class HotkeysPage final : public HelpPage
{
    const std::vector<ModInformation *> &mods;
    H3DlgScrollableText *textScroll = nullptr;
    H3String renderedText;
    int activeCategory = hotkeys::CATEGORY_ALL;

  public:
    HotkeysPage(int x, int y, int width, int height, H3Dlg *dialog,
                const std::vector<ModInformation *> &mods);

    void SetCategory(int categoryId);

  private:
    bool MatchesCategory(const HotKey &hotkey) const noexcept;
    void RebuildText();
};

class HotkeysSection final : public HelpSection
{
    HotkeysCategoriesPage categoriesPage;
    HotkeysPage contentPage;

  public:
    HotkeysSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                   int contentY, int contentWidth, int contentHeight, H3Dlg *dialog,
                   const std::vector<ModInformation *> &mods);

    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
