#pragma once

#include "DlgEnums.h"
#include "HelpPage.h"

namespace main
{

namespace buttons
{
enum eHeroItem
{
    HERO_CATEGORY_FIRST = 350,
    HERO_CATEGORY_ALL = HERO_CATEGORY_FIRST,
    HERO_CATEGORY_TOWN_FIRST,
    HERO_CATEGORY_TOWN_LAST = HERO_CATEGORY_TOWN_FIRST + 8,
    HERO_CATEGORY_LAST = HERO_CATEGORY_TOWN_LAST,
    HERO_CATEGORY_COUNT = HERO_CATEGORY_LAST - HERO_CATEGORY_FIRST + 1,
    HERO_CATEGORY_SCROLLBAR = 370,
    HERO_CONTENT_SCROLLBAR = 371,
    HERO_ITEM_FIRST = 800,
    HERO_ITEM_LAST = HERO_ITEM_FIRST + 99
};
} // namespace buttons

class HeroCategoriesPage final : public HelpPage
{
    static HeroCategoriesPage *instance;

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
    HeroCategoriesPage(int x, int y, int width, int height, H3Dlg *dialog);
    ~HeroCategoriesPage() override;

    void Redraw(int firstCategory = 0);
    void SetActiveCategory(int categoryIndex) noexcept;

  private:
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class HeroesPage : public HelpPage
{
    static HeroesPage *instance;

    const int pageX;
    const int pageY;
    const int pageWidth;
    const int pageHeight;

    std::vector<int> heroIds;
    std::vector<H3DlgPcx *> heroPortraits;
    std::vector<H3LoadedPcx *> loadedHeroPortraits;
    std::vector<H3DlgText *> heroNames;
    H3DlgScrollbar *scrollBar = nullptr;
    H3DlgPcx16 *backPcx = nullptr;
    int activeCategory = buttons::HERO_CATEGORY_ALL;
    int columns = 1;
    int rows = 1;
    int firstRow = 0;

  public:
    HeroesPage(int x, int y, int width, int height, H3Dlg *dialog);
    ~HeroesPage() override;

    void SetCategory(int categoryIndex);
    void Redraw(int requestedFirstRow = 0);

    virtual void CallCustomFunction(int heroId, BOOL rightClick) noexcept;
    virtual void OnLeftClick(int heroId) noexcept;
    virtual void OnRightClick(int heroId) noexcept;
    BOOL ProcessItemMessage(H3Msg &msg);

  private:
    BOOL MatchesCategory(int heroId) const noexcept;
    H3LoadedPcx *GetHeroPortrait(int heroId) noexcept;
    void RebuildHeroList();
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class HeroesSection final : public HelpSection
{
    HeroCategoriesPage categoriesPage;
    HeroesPage contentPage;

  public:
    HeroesSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                  int contentY, int contentWidth, int contentHeight, H3Dlg *dialog);

    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
