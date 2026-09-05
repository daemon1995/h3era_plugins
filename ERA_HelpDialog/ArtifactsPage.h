#pragma once

#include "HelpPage.h"

namespace main
{

namespace artifacts
{
enum eItem
{
    CATEGORY_FIRST = 500,
    CATEGORY_ALL = CATEGORY_FIRST,
    CATEGORY_TREASURE,
    CATEGORY_MINOR,
    CATEGORY_MAJOR,
    CATEGORY_RELIC,
    CATEGORY_OTHER,
    CATEGORY_LAST = CATEGORY_OTHER,
    CATEGORY_COUNT = CATEGORY_LAST - CATEGORY_FIRST + 1,
    CONTENT_SCROLLBAR = 520,
    ITEM_FIRST = 600,
    ITEM_LAST = ITEM_FIRST + 99
};
}

class ArtifactCategoriesPage final : public HelpPage
{
    H3DlgCaptionButton *buttons[artifacts::CATEGORY_COUNT] = {};
    int activeCategory = artifacts::CATEGORY_ALL;

  public:
    ArtifactCategoriesPage(int x, int y, int width, int height, H3Dlg *dialog);

    BOOL IsCategory(int itemId) const noexcept;
    void SetActiveCategory(int itemId) noexcept;
};

class ArtifactsPage : public HelpPage
{
    static ArtifactsPage *instance;
    const int pageX;
    const int pageY;
    const int pageWidth;
    const int pageHeight;
    std::vector<int> artifactIds;
    std::vector<H3DlgDef *> portraits;
    std::vector<H3DlgText *> names;
    H3DlgScrollbar *scrollBar = nullptr;
    H3DlgPcx16 *backPcx = nullptr;
    int activeCategory = artifacts::CATEGORY_ALL;
    int columns = 1;
    int rows = 1;
    int firstRow = 0;

  public:
    ArtifactsPage(int x, int y, int width, int height, H3Dlg *dialog);
    ~ArtifactsPage() override;

    void SetCategory(int categoryId);
    void Redraw(int firstRow = 0);

    virtual void CallCustomFunction(int artifactId, BOOL rightClick) noexcept;
    virtual void OnLeftClick(int artifactId) noexcept;
    virtual void OnRightClick(int artifactId) noexcept;
    BOOL ProcessItemMessage(H3Msg &msg);

  private:
    BOOL MatchesCategory(int artifactId) const noexcept;
    void RebuildArtifactList();
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class ArtifactsSection final : public HelpSection
{
    ArtifactCategoriesPage categoriesPage;
    ArtifactsPage contentPage;

  public:
    ArtifactsSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                     int contentY, int contentWidth, int contentHeight, H3Dlg *dialog);

    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
