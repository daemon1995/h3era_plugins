#pragma once

namespace scroll
{

using namespace h3;
constexpr int DRAG_MOUSE_ACCESS = 25;
constexpr int MAP_MARGIN = 8;

struct Calculator
{
    H3POINT pos;
    H3POINT point;

  public:
    Calculator(const H3POINT &p = {0, 0}, const H3Position &pos = {0}) noexcept;

  public:
    void operator+=(const H3POINT &other) noexcept;
    void align() noexcept;
};
class MapScroller : public IGamePatch
{

	static MapScroller* instance;
  private:
    const RECT scrollLimits;
    const UINT mapViewW;
    const UINT mapViewH;

    H3Position startMousePosition;
    H3POINT startMousePoint;
    H3POINT startScreenPosition;
    H3POINT startScreenOffset;
    DWORD sinceLastDrawTime = NULL;

    BOOL isMapView = FALSE;
    BOOL inMoveAction = FALSE;
    BOOL scrollingDone = FALSE;
    BOOL rmcAtMapScreen = FALSE;
    BOOL wheelButtonAtMapScreen = FALSE;

    const H3BaseDlg *lastAdddedDlg = nullptr;
    Patch *edgeScrollHook = nullptr;

  private:
    MapScroller() noexcept;

  protected:
    virtual void CreatePatches() noexcept final;

  private:
    void Stop() noexcept;
    void SetMapEdgeScrollStatus(const BOOL state) noexcept;

  private:
    static int __stdcall AdvMgr_MouseMove(HiHook *h, H3AdventureManager *adv, const int x, const int y) noexcept;
    static int __stdcall AdvMgr_MapScreenProcedure(HiHook *h, H3AdventureManager *adv, H3Msg *msg) noexcept;

    static void __stdcall AdvMgr_SetActiveHero(HiHook *h, H3AdventureManager *adv, const int heroIdx, const int a3,
                                               const char a4, const char a5) noexcept;
    static _LHF_(AdvMgr_MobilizeCurrentHero) noexcept;

    static _LHF_(BaseDlg_OnRightClickHold) noexcept;
    static void __stdcall WndMgr_AddNewDlg(HiHook *h, const H3WindowManager *wm, const H3BaseDlg *dlg, const int index,
                                           const int draw) noexcept;

  public:
    static MapScroller &Get() noexcept; // (PatcherInstance* _PI);
};

} // namespace scroll
