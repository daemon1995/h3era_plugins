#pragma once

#include "DlgEnums.h"
#include "HelpPage.h"

namespace main
{

namespace spells
{
enum eItem
{
    CATEGORY_FIRST = 1000,
    CATEGORY_ALL = CATEGORY_FIRST,
    CATEGORY_FIRE,
    CATEGORY_AIR,
    CATEGORY_WATER,
    CATEGORY_EARTH,
    CATEGORY_LAST = CATEGORY_EARTH,
    CATEGORY_COUNT = CATEGORY_LAST - CATEGORY_FIRST + 1,
    CATEGORY_SCROLLBAR = 1020,
    CONTENT_SCROLLBAR = 1021,
    LEVEL_FIRST = 1030,
    LEVEL_LAST = LEVEL_FIRST + 5,
    LEVEL_COUNT = LEVEL_LAST - LEVEL_FIRST + 1,
    ITEM_FIRST = 1100,
    ITEM_LAST = ITEM_FIRST + 99
};
} // namespace spells

class SpellCategoriesPage final : public HelpPage
{
    H3DlgCaptionButton *buttons[spells::CATEGORY_COUNT] = {};
    int activeCategory = spells::CATEGORY_ALL;

  public:
    SpellCategoriesPage(int x, int y, int width, int height, H3Dlg *dialog);

    BOOL IsCategory(int itemId) const noexcept;
    void SetActiveCategory(int itemId) noexcept;
};

class SpellsPage final : public HelpPage
{
    static SpellsPage *instance;
    const int pageX;
    const int pageY;
    const int pageWidth;
    const int pageHeight;
    std::vector<int> spellIds;
    std::vector<H3DlgDef *> portraits;
    std::vector<H3DlgText *> names;
    std::vector<H3DlgCaptionButton *> levelButtons;
    H3DlgScrollbar *scrollBar = nullptr;
    H3DlgPcx16 *backPcx = nullptr;
    int activeCategory = spells::CATEGORY_ALL;
    int activeLevel = -1;
    int columns = 1;
    int rows = 1;
    int firstRow = 0;

  public:
    SpellsPage(int x, int y, int width, int height, H3Dlg *dialog);
    ~SpellsPage() override;

    void SetCategory(int categoryId);
    void SetLevel(int level);
    void Redraw(int firstRow = 0);

  private:
    BOOL MatchesCategory(int spellId) const noexcept;
    void RebuildSpellList();
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class SpellsSection final : public HelpSection
{
    SpellCategoriesPage categoriesPage;
    SpellsPage contentPage;

  public:
    SpellsSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                  int contentY, int contentWidth, int contentHeight, H3Dlg *dialog);

    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
