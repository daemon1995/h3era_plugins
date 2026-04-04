#pragma once
#include "pch.h"

#include <unordered_map>
namespace ERI
{
constexpr int MITHRIL_DLG_TEXT_ITEM_ID = 1016;
constexpr int MITHRIL_DLG_BACK_PCX_ITEM_ID = 1025;
constexpr int MITHRIL_DLG_DEF_ITEM_ID = 1019;
constexpr LPCSTR RESOURCE_HINT_FORMAT = "{%s}: %s";
enum eFrameState
{
    FRAME_STATE_NONE,
    FRAME_STATE_BUILD,
};

struct ResourceBarInfo
{
    BOOL createMithril = FALSE;
    int firstResX = 0;
    int itemWidth = 0;
    int itemStep = 0;
    eFrameState frameState = FRAME_STATE_NONE;
    H3DlgItem* mithrilDefItem = nullptr;
    H3DlgItem* mithrilTextItem = nullptr;
    H3DlgItem* mithrilBackPcxItem = nullptr;
};
class ExtendedResourcesInfo : public IGamePatch
{
    static ExtendedResourcesInfo *instance;
    static constexpr LPCSTR MITHRIL_DEF_NAME = "SMALmith.def";
    int m_resbarLastHintItemId = -1;
    H3DlgItem *advMapHintControl = nullptr;
    H3DlgItem *kingdomHintcontrol = nullptr;
    H3DlgItem *townMgrHintcontrol = nullptr;
    H3DlgItem *townHallHintcontrol = nullptr;
    // bool lastHintIsMitril;

    std::unordered_map<DWORD, H3LoadedPcx *> mithrilBackPcxCache;


    std::unordered_map<DWORD, ResourceBarInfo> resourceBarPatchInfos;

  private:
    ExtendedResourcesInfo(PatcherInstance *_pi);

  public:
    virtual ~ExtendedResourcesInfo();

  private:
    virtual void CreatePatches() override;

  private:
    BOOL ShowMithrilRMCHint(const H3Msg *msg, H3DlgItem *hintZone) const noexcept;

  private:
    static H3DlgItem *AlignOriginalResources(LoHook *h, HookContext *c, const ResourceBarInfo &info);
    static H3DlgItem *BuildMithril(LoHook *h, HookContext *c, const ResourceBarInfo &info);

    static _LHF_(AfterDlgResBarCreate);
    static _LHF_(H3ResourceBarPanel__Refresh);

    static void __stdcall H3ResourceBarPanel__HideResources(HiHook *h, H3ResourceBarPanel *resourceBarPanel);
    static _LHF_(OnAdvMgrDlgRightClick);
    static int __stdcall KingdomOverviewDlgProc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg);

  public:
    const int LastHintItemId() const noexcept;
    void SetLastHintItemId(const int itemId) noexcept;
    static ExtendedResourcesInfo &Get();
};
} // namespace ERI
