#pragma once

#include "CatalogueData.h"
#include "DlgPage.h"

namespace main
{
class CatalogueSection final : public DlgSection, private DlgPage
{
    const helpdlg::Catalogue &catalogue;
    helpdlg::CatalogueFilter filter;
    std::vector<int> filtered;
    std::vector<H3DlgCaptionButton *> categoryButtons;
    std::vector<H3DlgCaptionButton *> facetButtons;
    std::vector<H3DlgCaptionButton *> levelButtons;
    std::vector<H3DlgCaptionButton *> objectButtons;
    std::vector<H3DlgDef *> defPortraits;
    std::vector<std::vector<H3DlgDef *>> spellUnderlays;
    std::vector<H3DlgPcx *> pcxPortraits;
    std::vector<H3DlgFrame *> selectionFrames;
    std::vector<H3DlgDef *> banMarkers;
    std::vector<H3LoadedPcx *> portraitCache;
    H3DlgScrollbar *categoryScroll = nullptr;
    H3DlgScrollbar *contentScroll = nullptr;
    H3DlgEdit *queryEdit = nullptr;
    H3DlgText *countText = nullptr;
    H3DlgText *emptyText = nullptr;
    H3DlgCaptionButton *sortButton = nullptr;
    int idBase;
    int x, y, width, height;
    int categoryRows = 1;
    int categoryFirst = 0;
    int columns = 1;
    int rows = 1;
    static CatalogueSection *visibleSection;

    void Rebuild(bool resetScroll = true);
    void RefreshCategories();
    void RefreshControls();
    bool RefreshBanMarkers();
    static void __fastcall CategoryScrollProc(INT32 tick, H3BaseDlg *dlg);
    static void __fastcall ContentScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    void ReleaseInputFocus(int keepItemId = -1) noexcept override;
    CatalogueSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                     int contentWidth, H3Dlg *dialog, const helpdlg::Catalogue &catalogue);
    ~CatalogueSection() override;
    void SetSubtype(int subtype);
    void SetFilter(const helpdlg::CatalogueFilter &state);
    const helpdlg::CatalogueFilter &Filter() const noexcept
    {
        return filter;
    }
    void FocusObject(int objectId, bool openDetails = true);
    void SetVisible(BOOL state) noexcept override;
    void UpdateMousePosition(const H3Msg &msg) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};
} // namespace main
