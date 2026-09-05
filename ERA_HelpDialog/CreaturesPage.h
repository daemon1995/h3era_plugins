#pragma once

#include "DlgEnums.h"
#include "HelpPage.h"

namespace main
{

namespace buttons
{
// IDs private to the creatures section. Header controls use 100..106.
enum eCreatureItem
{
    CREATURE_CATEGORY_FIRST = 300,
    CREATURE_CATEGORY_ALL = CREATURE_CATEGORY_FIRST,
    CREATURE_CATEGORY_TOWN_FIRST,
    CREATURE_CATEGORY_TOWN_LAST = CREATURE_CATEGORY_TOWN_FIRST + 8,
    CREATURE_CATEGORY_NEUTRAL,
    CREATURE_CATEGORY_WAR_MACHINES,
    CREATURE_CATEGORY_COMMANDERS,
    CREATURE_CATEGORY_LAST = CREATURE_CATEGORY_COMMANDERS,
    CREATURE_CATEGORY_COUNT = CREATURE_CATEGORY_LAST - CREATURE_CATEGORY_FIRST + 1,
    CREATURE_CATEGORY_SCROLLBAR = 320,
    CREATURE_CONTENT_SCROLLBAR = 321,
    CREATURE_ITEM_FIRST = 400,
    CREATURE_ITEM_LAST = CREATURE_ITEM_FIRST + 99,
    CREATURE_LEVEL_FIRST = 330,
    CREATURE_LEVEL_LAST = CREATURE_LEVEL_FIRST + 6,
    CREATURE_LEVEL_COUNT = CREATURE_LEVEL_LAST - CREATURE_LEVEL_FIRST + 1
};
} // namespace buttons

class CreatureCategoriesPage final : public HelpPage
{
    static CreatureCategoriesPage *instance;

    const int pageX;
    const int pageY;
    const int pageWidth;
    const int pageHeight;

    std::vector<H3DlgDef *> categoryIcons;
    std::vector<H3DlgCaptionButton *> categoryButtons;
    H3DlgScrollbar *scrollBar = nullptr;
    int firstCategoryIndex = 0;
    int visibleCategoryCount = 0;
    int activeCategoryIndex = 0;

  public:
    CreatureCategoriesPage(int x, int y, int width, int height, H3Dlg *dialog);
    ~CreatureCategoriesPage() override;

    void Redraw(int firstCategory = 0);
    void SetActiveCategory(int categoryIndex) noexcept;

  private:
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class CreaturesPage : public HelpPage
{
    static CreaturesPage *instance;

    const int pageX;
    const int pageY;
    const int pageWidth;
    const int pageHeight;

    std::vector<int> creatureIds;
    std::vector<H3DlgDef *> creaturePortraits;
    std::vector<H3DlgText *> creatureNames;
    std::vector<H3DlgDefButton *> levelButtons;
    H3DlgScrollbar *contentScrollBar = nullptr;
    H3DlgPcx16 *backPcx = nullptr;
    int activeCategory = buttons::CREATURE_CATEGORY_ALL;
    int activeLevel = -1;
    int columns = 1;
    int rows = 1;
    int firstRow = 0;

  public:
    CreaturesPage(int x, int y, int width, int height, H3Dlg *dialog);
    ~CreaturesPage() override;

    void SetCategory(int categoryIndex);
    void SetLevel(int level);
    void Redraw(int requestedFirstRow = 0);

    // Extension point for catalogue integrations. The default implementation
    // is intentionally empty; derived pages can execute their own action.
    virtual void CallCustomFunction(int creatureId, BOOL rightClick) noexcept;
    virtual void OnLeftClick(int creatureId) noexcept;
    virtual void OnRightClick(int creatureId) noexcept;
    BOOL ProcessItemMessage(H3Msg &msg);

  private:
    BOOL MatchesCategory(int creatureId) const noexcept;
    void RebuildCreatureList();
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class CreaturesSection final : public HelpSection
{
    CreatureCategoriesPage categoriesPage;
    CreaturesPage contentPage;

  public:
    CreaturesSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                     int contentY, int contentWidth, int contentHeight, H3Dlg *dialog);

    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
