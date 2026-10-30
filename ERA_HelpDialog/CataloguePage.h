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
    std::vector<H3DlgText *> facetLabels;
    std::vector<H3DlgTextPcx *> facetButtons;
    H3DlgText *sortLabel = nullptr;
    std::vector<H3DlgText *> levelTexts;
    std::vector<std::pair<H3DlgFrame *, H3DlgFrame *>> levelSelectionFrames;
    std::vector<H3DlgPcx16 *> objectLabels;
    std::vector<H3DlgDef *> defPortraits;
    std::vector<H3DlgPcx *> artifactBackgrounds;
    std::vector<H3DlgDef *> fallbackCreaturePortraits;
    std::vector<std::vector<H3DlgDef *>> spellUnderlays;
    std::vector<H3DlgPcx *> pcxPortraits;
    std::vector<H3DlgFrame *> selectionFrames;
    std::vector<H3DlgDef *> banMarkers;
    std::vector<H3LoadedPcx *> portraitCache;
    H3DlgScrollbar *categoryScroll = nullptr;
    H3DlgScrollbar *contentScroll = nullptr;
    H3DlgText *countText = nullptr;
    H3DlgText *emptyText = nullptr;
    H3DlgTextPcx *sortButton = nullptr;
    H3DlgDef *resetButton = nullptr;
    int idBase;
    int x, y, width, height;
    int categoryRows = 1;
    int categoryFirst = 0;
    int searchFocusedId = -1;
    int columns = 1;
    int rows = 1;
    static CatalogueSection *visibleSection;

    void Rebuild(bool resetScroll = true);
    void RefreshCategories();
    void RefreshControls();
    void EnsureCategoryVisible(int category) noexcept;
    bool RefreshBanMarkers();
    int FallbackCreature(int slot, const helpdlg::CatalogueEntry &entry) const noexcept;
    int SortOptionCount() const noexcept;
    LPCSTR SortOptionName(int option) const noexcept;
    int ChooseOption(H3DlgTextPcx *anchor, const std::vector<LPCSTR> &options, int selected) const;
    std::string CategoryCaption(int category, int count) const;
    static void __fastcall CategoryScrollProc(INT32 tick, H3BaseDlg *dlg);
    static void __fastcall ContentScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    CatalogueSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                     int contentWidth, H3Dlg *dialog, const helpdlg::Catalogue &catalogue);
    virtual ~CatalogueSection() override;
    void SetSubtype(int subtype);
    void SetFilter(const helpdlg::CatalogueFilter &state);
    const helpdlg::CatalogueFilter &Filter() const noexcept
    {
        return filter;
    }
    void FocusObject(int objectId, bool openDetails = true, bool fromSearch = false);
    void SetVisible(BOOL state) noexcept override;
    void UpdateMousePosition(const H3Msg &msg) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};
} // namespace main
