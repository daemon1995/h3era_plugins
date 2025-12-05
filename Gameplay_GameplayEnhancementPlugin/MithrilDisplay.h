#pragma once
using namespace h3;
namespace ERI
{
constexpr int MITHRIL_TEXT_ID = 1016;
constexpr int MITHRIL_BACK_PCX_ID = 1025;
constexpr int MITHRIL_DEF_ID = 1019;
class ExtendedResourcesInfo : public IGamePatch
{
    static ExtendedResourcesInfo *instance;
    static constexpr LPCSTR MITHRIL_DEF_NAME = "SMALmith.def";

    int m_resbarLastHintItemId = -1;
    H3DlgItem *advMapHintControl = nullptr;
    H3DlgItem *kingdomHintcontrol = nullptr;
    // bool lastHintIsMitril;

  private:
    ExtendedResourcesInfo(PatcherInstance *_pi);

  public:
    virtual ~ExtendedResourcesInfo();

  private:
    virtual void CreatePatches() override;

  private:
    BOOL ShowMithrilRMCHint(const H3Msg *msg, H3DlgItem *hintZone) const noexcept;

  private:
    static BOOL8 BuildMithril(H3ResourceBarPanel *resourceBarPanel, int textItemWidth, bool buildFrame = true);
    static _LHF_(OnAdvMgrDlgResBarCreate);
    static _LHF_(OnKingdomOverviewDlgResBarCreate);
    static void __stdcall H3ResourceBarPanel__Refresh(HiHook *h, H3ResourceBarPanel *resourceBarPanel, const BOOL redraw,
                                                             const BOOL redrawScreen);

    static void __stdcall H3ResourceBarPanel__Hide(HiHook *h, H3ResourceBarPanel *resourceBarPanel);
    static _LHF_(OnAdvMgrDlgRightClick);
    static int __stdcall KingdomOverviewDlgProc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg);

  public:
    const int LastHintItemId() const noexcept;
    void SetLastHintItemId(const int itemId) noexcept;
    static ExtendedResourcesInfo &Get();
};
} // namespace ERI
