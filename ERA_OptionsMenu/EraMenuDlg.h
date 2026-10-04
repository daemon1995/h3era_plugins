#pragma once

#include "pch.h"
#include "OptionsRuntime.h"
#include "OptionMenuModel.h"
#include "OptionsMenuUI.h"

class EraMenuDlg final : public era_options::OptionsDialog
{
    struct SidebarEntry { int mod; int page; };
    struct OptionRow
    {
        H3DlgText *name = nullptr;
        H3DlgDefButton *checkbox = nullptr;
        H3DlgDefButton *radio = nullptr;
        era_options::OptionCell cell;
    };

    era_options::OptionMenuModel model;
    std::vector<SidebarEntry> sidebarEntries;
    std::vector<H3DlgCaptionButton *> modRows;
    std::vector<OptionRow> optionRows;
    std::vector<int> rowHeights;
    std::vector<era_options::OptionGridRow> grid;
    H3DlgScrollbar *modScroll = nullptr;
    H3DlgScrollbar *optionScroll = nullptr;
    std::map<std::pair<int, int>, H3DlgScrollbar *> pageScrollbars;
    H3DlgEdit *search = nullptr;
    H3DlgText *summary = nullptr;
    H3DlgText *empty = nullptr;
    H3DlgText *pageTitle = nullptr;
    int firstMod = 0;
    int firstOption = 0;
    int lastOptionStart = 0;
    int listHeight = 0;
    int sidebarHeight = 0;
    int sidebarWidth = 0;
    int optionX = 0;
    int columnWidth = 0;
    int textWidth = 0;
    bool activated = false;
    bool resizeRequested = false;
    bool bansRequested = false;
    bool fullScreen = false;
    bool allowEditing = true;
    bool currentMap = false;

    EraMenuDlg(int width, int height, bool fullScreen, const era_options::OptionMenuModel &state, int sidebarPosition,
        bool allowEditing, bool currentMap);
    int DisplayValue(const era_options::EOption &option) const;
    void CreateBackground();
    void CreateWidgets();
    void RebuildOptions();
    void SelectPageScrollbar();
    void RefreshOptions();
    void RefreshMods(bool revealSelection = false);
    void SyncSearch();
    void UpdateScrollActivity();
    bool SaveAndClose();
    void ShowErrors();
    era_options::EOption *OptionAtControl(int controlId) const;
    const OptionRow *RowAtControl(int controlId) const;
    static void __fastcall ModScrollProc(INT32 tick, H3BaseDlg *dlg);
    static void __fastcall OptionScrollProc(INT32 tick, H3BaseDlg *dlg);
    INT vDialogProc(H3Msg &msg) override;
    INT vPreProcess(H3Msg &msg) override;
    BOOL OnKeyPress(eVKey key, eMsgFlag flag) override;
    BOOL OnLeftClick(INT itemId, H3Msg &msg) override;
    BOOL OnRightClick(H3DlgItem *item) override;
    BOOL OnMouseWheel(INT direction) override;
    BOOL OnMouseHover(H3DlgItem *item) override;
    void OnClose(INT itemId) override;

  public:
    static BOOL CreateAndRun(bool showBans = false, bool allowEditing = true);
};
