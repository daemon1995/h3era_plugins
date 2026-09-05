#pragma once

#include "HelpPage.h"
#include "ModInformation.h"

namespace main
{

namespace modpage
{
enum eItem
{
    CATEGORY_FIRST = 700,
    CATEGORY_SCROLLBAR = 720,
    CONTENT_SCROLLBAR = 721
};
}

class ModCategoriesPage final : public HelpPage
{
    int pageX;
    int pageY;
    int pageWidth;
    int pageHeight;
    std::vector<H3DlgCaptionButton *> buttons;
    H3DlgScrollbar *scrollBar = nullptr;
    const ModInformation *activeMod = nullptr;
    int firstIndex = 0;
    int visibleCount = 1;
    static ModCategoriesPage *instance;

  public:
    ModCategoriesPage(int x, int y, int width, int height, H3Dlg *dialog);

    void SetMod(const ModInformation *mod);
    BOOL IsCategory(int itemId) const noexcept;
    int CategoryIndex(int itemId) const noexcept;
    void SetActiveCategory(int index);
    void Refresh();

  private:
    void CreateForMod(const ModInformation *mod);
    void RedrawItems(int firstIndex);
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class ModContentPage final : public HelpPage
{
    int pageX;
    int pageY;
    int pageWidth;
    int pageHeight;
    H3DlgScrollableText *textScroll = nullptr;
    H3String renderedText;
    const ModInformation *activeMod = nullptr;
    int activeCategory = 0;

  public:
    ModContentPage(int x, int y, int width, int height, H3Dlg *dialog);

    void SetMod(const ModInformation *mod);
    void SetCategory(int index);
    void RefreshVisibility();

  private:
    void RebuildText();
};

class ModSection final : public HelpSection
{
    ModCategoriesPage categoriesPage;
    ModContentPage contentPage;
    const ModInformation *activeMod = nullptr;

  public:
    ModSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
               int contentY, int contentWidth, int contentHeight, H3Dlg *dialog);

    void SetMod(const ModInformation *mod);
    void SetSubtype(int subtype);
    void SetVisible(BOOL state) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
