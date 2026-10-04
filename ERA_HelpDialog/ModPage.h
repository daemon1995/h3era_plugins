#pragma once

#include "DlgPage.h"
#include "ModInformation.h"
#include <map>

namespace main
{

namespace modpage
{
enum eItem
{
    CATEGORY_FIRST = 16000,
    CATEGORY_SCROLLBAR = 15998,
    CONTENT_SCROLLBAR = 15999
};
} // namespace modpage

class ModCategoriesPage final : public DlgPage
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
    ~ModCategoriesPage() override;

    void SetMod(const ModInformation *mod);
    BOOL IsCategory(int itemId) const noexcept;
    int CategoryIndex(int itemId) const noexcept;
    void SetActiveCategory(int index);
    void UpdateScrollbarActivation(const H3Msg &msg) noexcept;
    void Refresh();

  private:
    void CreateForMod(const ModInformation *mod);
    void RedrawItems(int firstIndex);
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
};

class ModContentPage final : public DlgPage
{
    int pageX;
    int pageY;
    int pageWidth;
    int pageHeight;
    H3DlgScrollableText *textScroll = nullptr;
    H3String renderedText;
    const ModInformation *activeMod = nullptr;
    int activeCategory = 0;
    std::vector<H3DlgItem *> objectItems;
    std::vector<std::pair<int, H3String>> actions;
    H3String pendingAction;
    const Category *renderedCategory = nullptr;
    struct ObjectControls
    {
        std::vector<H3DlgItem *> items;
        std::vector<std::pair<int, H3String>> actions;
    };
    std::map<const Category *, ObjectControls> objectCache;
    std::map<const Category *, int> scrollPositions;
    int nextObjectId = 21000;

  public:
    ModContentPage(int x, int y, int width, int height, H3Dlg *dialog);

    void SetMod(const ModInformation *mod);
    void SetCategory(int index);
    void RefreshVisibility();
    void SetVisible(BOOL state) noexcept override;
    void UpdateScrollbarActivation(const H3Msg &msg) noexcept;
    BOOL ProcessMessage(H3Msg &msg);
    H3String TakeAction();
    int ScrollPosition() const;
    void SetScrollPosition(int position);

  private:
    void RebuildText();
    void RebuildObjects(const Category *category);
};

class ModSection final : public DlgSection
{
    ModCategoriesPage categoriesPage;
    ModContentPage contentPage;
    const ModInformation *activeMod = nullptr;

  public:
    ModSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX, int contentY,
               int contentWidth, int contentHeight, H3Dlg *dialog);

    void SetMod(const ModInformation *mod);
    void SetSubtype(int subtype);
    void SetVisible(BOOL state) noexcept override;
    void UpdateMousePosition(const H3Msg &msg) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
    H3String TakeAction()
    {
        return contentPage.TakeAction();
    }
    int ScrollPosition() const
    {
        return contentPage.ScrollPosition();
    }
    void SetScrollPosition(int position)
    {
        contentPage.SetScrollPosition(position);
    }
};

} // namespace main
