#pragma once
#include "framework.h"

namespace scroll
{

constexpr int DRAG_MOUSE_ACCESS = 25;
constexpr int MAP_MARGIN = 8;
constexpr int MAP_TILE_SIZE = 32;

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

    static MapScroller *instance;

  private:
    const RECT scrollLimits;
    const INT mapViewW;
    const INT mapViewH;

    H3POINT startMousePoint;
    H3POINT lastMousePoint;
    H3POINT scrollCursorPoint;
    H3MouseManager::Cursor scrollCursor;
    H3Position startScreenPosition;
    H3POINT startScreenOffset;
    H3Position scrollScreenPosition;
    H3POINT scrollScreenOffset;
    H3Position lastHintScreenPosition;
    H3POINT lastHintScreenOffset;
    DWORD lastDrawStartedAt = 0;

    BOOL hasHintCamera = FALSE;
    BOOL inMoveAction = FALSE;
    BOOL cursorPinned = FALSE;
    BOOL pendingMouseMove = FALSE;
    // Keep this until the matching release, even if a dialog or hero selection cancels the drag.
    BOOL suppressMiddleButtonUp = FALSE;
    BOOL rmcAtMapScreen = FALSE;
    BOOL wheelButtonAtMapScreen = FALSE;

    const H3BaseDlg *rightClickDlg = nullptr;
    Patch *edgeScrollHook = nullptr;

  private:
    MapScroller() noexcept;

  protected:
    virtual void CreatePatches() noexcept final;

  private:
    void StopMapScrolling() noexcept;
    void BeginMapDragging() noexcept;
    void PinScrollCursor() noexcept;
    void SetMapEdgeScrollStatus(const BOOL state) noexcept;
    BOOL IsMapPoint(const H3POINT &point) const noexcept;
    BOOL IsDragThresholdReached(const H3POINT &point) const noexcept;
    BOOL IsScrolling() const noexcept;
    BOOL IsScrollButtonHeld() const noexcept;
    void StartMapScrolling(H3AdventureManager *adv, const H3POINT &point, const BOOL middleButton) noexcept;
    void ScrollMap(H3AdventureManager *adv, const H3POINT &point, const BOOL forceDraw = FALSE) noexcept;
    void UpdateHintCamera(H3AdventureManager *adv) noexcept;

  private:
    static int __stdcall AdvMgr_MouseMove(HiHook *h, H3AdventureManager *adv, const int x, const int y) noexcept;
    static int __stdcall AdvMgr_MapScreenProcedure(HiHook *h, H3AdventureManager *adv, H3Msg *msg) noexcept;
    static _LHF_(AdvMgr_MouseMoveCoordinates) noexcept;
    static _LHF_(MouseMgr_DrawCursorCoordinates) noexcept;

    static void __stdcall AdvMgr_SetActiveHero(HiHook *h, H3AdventureManager *adv, const int heroIdx, const int a3,
                                               const char a4, const char a5) noexcept;
    static void __stdcall AdvMgr_SetActiveTown(HiHook *h, H3AdventureManager *adv, const int townIdx, const int a3,
                                               const char a4) noexcept;

    static _LHF_(BaseDlg_OnRightClickHold) noexcept;
    static void __stdcall WndMgr_ShowRightClickDlg(HiHook *h, H3WindowManager *wm, const H3BaseDlg *dlg) noexcept;
    static void __stdcall WndMgr_AddNewDlg(HiHook *h, const H3WindowManager *wm, const H3BaseDlg *dlg, const int index,
                                           const int draw) noexcept;

  public:
    static MapScroller &Get() noexcept; // (PatcherInstance* _PI);
    static void ApplySmoothScrollState(const BOOL state) noexcept;
};

} // namespace scroll
