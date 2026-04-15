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
enum eCreateResBarAddress
{
    CREATE_ADV_MAN = 0x4021B2,
    CREATE_KINGDOM_OVERVIEW = 0x51F042,
    CREATE_PUZZLE_MAP = 0x52CA2E,
    CREATE_TOWN_MGR = 0x5C6976,
    CREATE_MAGE_GUILD = 0x5CEB83,
    CREATE_TOWN_HALL = 0x5D2C71,
    CREATE_TOWN_FORT = 0x5D3A16,
    CREATE_THIEVE_GUILD = 0x5DF251,
};
struct ResourceBarInfo
{
    BOOL createMithril = FALSE;
    int firstResX = 0;
    int customTextItemWidth = 0;
    int itemStep = 0;
    eFrameState frameState = FRAME_STATE_NONE;
    H3DlgItem *mithrilDefItem = nullptr;
    H3DlgItem *mithrilTextItem = nullptr;
    H3LoadedPcx *mithrilBackPcxCache = nullptr;
};
class ExtendedResourcesInfo : public IGamePatch
{
    static ExtendedResourcesInfo *instance;
    static constexpr LPCSTR MITHRIL_DEF_NAME = "SMALmith.def";
    int m_resbarLastHintItemId = -1;
    // bool lastHintIsMitril;

    std::unordered_map<DWORD, ResourceBarInfo> resourceBarPatchInfos;

  private:
    ExtendedResourcesInfo(PatcherInstance *_pi);

  public:
    virtual ~ExtendedResourcesInfo();

  private:
    virtual void CreatePatches() override;

  private:
    static BOOL ShowMithrilRMCHint(const H3Msg *msg, H3DlgItem *hintZone) noexcept;
    static H3DlgItem *GetMitrilBarHintZone(DWORD patchAddress) noexcept;

  private:
    static BOOL AlignOriginalResources(HookContext *c, const ResourceBarInfo &info);
    static H3DlgItem *BuildMithril(HookContext *c, ResourceBarInfo &info);

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
