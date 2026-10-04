#pragma once
#include "pch.h"
#include "ObjectBansRuntime.h"
#include "OptionsMenuUI.h"

class ObjectBansDlg final : public era_options::OptionsDialog
{
  public:
    struct State
    {
        int tab = 0;
        std::array<era_options::BanObjectFilter, era_options::BanObjectKindCount> filters;
    };
  private:
    struct Cell
    {
        H3DlgCaptionButton *name = nullptr;
        std::array<H3DlgDef *, 3> images = {};
        H3DlgPcx *hero = nullptr;
        H3DlgDefButton *check = nullptr;
        H3DlgFrame *bannedFrame = nullptr;
        int object = -1;
    };
    int tab = 0, columns = 1, cellWidth = 100;
    bool optionsRequested = false;
    bool currentMap = false;
    bool allowEditing = true;
    era_options::ObjectBanRules mapRules;
    const era_options::ObjectBanRules &Rules() const;
    std::array<int, era_options::BanObjectKindCount> cellHeights = {};
    std::array<era_options::BanObjectFilter, era_options::BanObjectKindCount> filters;
    std::array<std::vector<era_options::BanObjectInfo>, era_options::BanObjectKindCount> objects;
    std::array<H3LoadedPcx *, era_options::BanHeroCount> heroPortraits = {};
    std::vector<int> filtered;
    std::vector<Cell> cells;
    std::array<H3DlgScrollbar *, era_options::BanObjectKindCount> scrollbars = {};
    std::array<H3DlgCaptionButton *, era_options::BanObjectKindCount> tabs = {};
    std::vector<H3DlgDefButton *> sources;
    std::vector<H3DlgText *> sourceNames;
    H3DlgEdit *query = nullptr;
    H3DlgCaptionButton *group = nullptr, *level = nullptr, *status = nullptr;
    H3DlgText *count = nullptr;
    era_options::BanObjectKind Kind() const;
    int GridTop() const { return tab ? 112 : 170; }
    int GridRows() const;
    const era_options::BanObjectInfo *AtControl(int id) const;
    void ShowPreview(const era_options::BanObjectInfo &entry);
    void SwitchTab(int next);
    void Rebuild(bool reset = true);
    void Refresh();
    void SetVisibleBans(bool banned);
    void SyncQuery();
    bool SaveAndClose();
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);
    INT vDialogProc(H3Msg &msg) override;
    INT vPreProcess(H3Msg &msg) override;
    BOOL OnLeftClick(INT itemId, H3Msg &msg) override;
    BOOL OnRightClick(H3DlgItem *item) override;
    BOOL OnMouseHover(H3DlgItem *item) override;
    BOOL OnKeyPress(eVKey key, eMsgFlag flag) override;
    void OnClose(INT itemId) override;

  public:
    ObjectBansDlg(int width, int height, const State &state, bool allowEditing = true, bool currentMap = false);
    ~ObjectBansDlg();
    State GetState() const { return {tab, filters}; }
    bool OptionsRequested() const { return optionsRequested; }
};
