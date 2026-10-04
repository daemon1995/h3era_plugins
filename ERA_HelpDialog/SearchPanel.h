#pragma once
#include "DlgPage.h"
#include "HelpUI.h"

namespace helpdlg
{
struct SearchEntry
{
    main::eHelpPage page = main::eHelpPage::MODS;
    int objectId = -1, modId = -1, category = 0, hotkeyContext = 0, hotkeyId = -1;
    std::string hotkeyQuery, label, searchable;
};

class SearchPanel final : public main::DlgSection, private main::DlgPage
{
    std::vector<SearchEntry> entries;
    std::vector<int> matches;
    std::vector<H3DlgCaptionButton *> scopes, rows;
    H3DlgText *countText = nullptr;
    H3DlgScrollbar *scroll = nullptr;
    std::string query;
    int firstIndex = 0, selectedIndex = 0, selection = -1;
    bool backRequested = false, selectionPopup = false;
    int x, y, width, height;
    static SearchPanel *visiblePanel;
    void Rebuild();
    void Accept();
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    SearchPanel(int categoryX, int y, int categoryWidth, int height, int contentX, int contentWidth, H3Dlg *dialog,
                std::vector<SearchEntry> entries);
    ~SearchPanel() override;
    void SetQuery(const std::string &text);
    void SetVisible(BOOL state) noexcept override;
    void UpdateMousePosition(const H3Msg &msg) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
    const SearchEntry *TakeSelection(bool &popup);
    bool TakeBackRequest();
};
} // namespace helpdlg
