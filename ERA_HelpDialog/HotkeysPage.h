#pragma once
#include "DlgPage.h"
#include "ModInformation.h"
#include "HelpLogic.h"

namespace main
{
class HotkeysSection final : public DlgSection, private DlgPage
{
    struct DisplayLine
    {
        bool heading = false;
        std::string left, right;
        int sourceId = -1, hotkeyId = -1;
    };

    const std::vector<ModInformation *> &mods;
    std::vector<H3DlgCaptionButton *> contexts;
    std::vector<H3DlgText *> headingRows, keyRows, actionRows;
    std::vector<H3DlgFrame *> rowSeparators;
    std::vector<DisplayLine> lines;
    H3DlgScrollbar *categoryScroll = nullptr;
    H3DlgScrollbar *contentScroll = nullptr;
    int categoriesX = 0, categoriesY = 0, categoriesWidth = 0, categoriesHeight = 0;
    int contentX = 0, contentY = 0, contentWidth = 0, contentHeight = 0;
    int categoryRows = 1, categoryFirst = 0;
    int visibleRows = 1, firstLine = 0;
    static HotkeysSection *visibleSection;

    void Rebuild(int focusModId = -1, int focusHotkeyId = -1);
    void RefreshCategories();
    void RenderRows();
    static void __fastcall CategoryScrollProc(INT32 tick, H3BaseDlg *dlg);
    static void __fastcall ContentScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    HotkeysSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                   int contentY, int contentWidth, int contentHeight, H3Dlg *dialog,
                   const std::vector<ModInformation *> &mods);
    virtual ~HotkeysSection() override = default;
    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    void UpdateMousePosition(const H3Msg &msg) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
    helpdlg::HotkeyFilter Filter() const;
    void SetFilter(const helpdlg::HotkeyFilter &filter);
    void FocusHotkey(LPCSTR folder, int context, int hotkeyId);
};
} // namespace main

namespace helpdlg
{
bool ShowHotkeyPreview(const HotKey &key, LPCSTR modName);
}
