#include "MapScroller.h"
#include <climits>

namespace scroll
{
namespace
{
constexpr DWORD MIN_DRAW_INTERVAL = 8;
constexpr DWORD MAP_ANIMATION_INTERVAL = 180;
constexpr ADDRESS MAP_NEXT_DRAW_TIME = 0x6989E8;

int SignedMapCoordinate(const UINT coordinate) noexcept
{
    return coordinate & 0x200 ? static_cast<int>(coordinate) - 1024 : static_cast<int>(coordinate);
}

void ClampCameraAxis(int &tile, int &offset, const int minTile, const int maxTile) noexcept
{
    const int pixelPosition = MAP_TILE_SIZE * tile - offset;
    if (pixelPosition < MAP_TILE_SIZE * minTile)
    {
        tile = minTile;
        offset = 0;
    }
    else if (pixelPosition > MAP_TILE_SIZE * maxTile)
    {
        tile = maxTile;
        offset = 0;
    }
}

void AdvanceMapDrawTime(H3AdventureManager *adv, const DWORD now) noexcept
{
    // Same scheduling as 0x40F1D0, without pumping messages recursively from MouseMove.
    auto &nextDraw = DwordAt(MAP_NEXT_DRAW_TIME);
    if (static_cast<INT32>(now - nextDraw) >= 0 && !ByteAt(reinterpret_cast<ADDRESS>(adv) + 0x104))
    {
        ++adv->refreshCounter;
        const DWORD elapsed = now - nextDraw;
        nextDraw += elapsed < MAP_ANIMATION_INTERVAL ? MAP_ANIMATION_INTERVAL : elapsed;
    }
}

void RefreshMapMouse(H3AdventureManager *adv, const H3POINT &point) noexcept
{
    adv->previousMousePosition = {INT_MIN, INT_MIN};
    THISCALL_3(int, 0x40E2C0, adv, point.x, point.y);
}
} // namespace

MapScroller *MapScroller::instance = nullptr;

MapScroller::MapScroller() noexcept
    : IGamePatch(globalPatcher->CreateInstance("EraPlugin.MapScrolling.daemon_n")),
      scrollLimits{IntAt(0x4195F2 + 1), IntAt(0x41960A + 1), CharAt(0x4195FC + 2), CharAt(0x419615 + 2)},
      mapViewW(H3GameWidth::Get() - 208), mapViewH(H3GameHeight::Get() - 56)
{
    CreatePatches();
}

MapScroller &MapScroller::Get() noexcept
{
    if (instance == nullptr)
        instance = new MapScroller();
    return *instance;
}

void MapScroller::ApplySmoothScrollState(const BOOL state) noexcept
{
    auto &scroller = Get();
    scroller.StopMapScrolling();
    scroller.suppressMiddleButtonUp = FALSE;
    scroller.SetEnabled(state);
    // ApplyAll also applies the edge-scroll patch; an enabled but idle scroller must undo it.
    if (state)
        scroller.SetMapEdgeScrollStatus(TRUE);
    else if (P_AdventureManager && P_AdventureManager->dlg)
    {
        P_AdventureManager->screenDrawOffset = {};
        P_AdventureManager->previousMousePosition = {INT_MIN, INT_MIN};
    }
}

BOOL MapScroller::IsMapPoint(const H3POINT &point) const noexcept
{
    return point.x >= MAP_MARGIN && point.x < MAP_MARGIN + mapViewW && point.y >= MAP_MARGIN &&
           point.y < MAP_MARGIN + mapViewH;
}

BOOL MapScroller::IsDragThresholdReached(const H3POINT &point) const noexcept
{
    return Abs(point.x - startMousePoint.x) > DRAG_MOUSE_ACCESS ||
           Abs(point.y - startMousePoint.y) > DRAG_MOUSE_ACCESS;
}

BOOL MapScroller::IsScrolling() const noexcept
{
    return rmcAtMapScreen || wheelButtonAtMapScreen;
}

BOOL MapScroller::IsScrollButtonHeld() const noexcept
{
    const int button = wheelButtonAtMapScreen ? VK_MBUTTON : VK_RBUTTON;
    return IsScrolling() && (STDCALL_1(SHORT, PtrAt(0x63A294), button) & 0x8000) != 0;
}

void MapScroller::StartMapScrolling(H3AdventureManager *adv, const H3POINT &point, const BOOL middleButton) noexcept
{
    StopMapScrolling();
    H3MouseManager *mouse = P_MouseManager;
    // Capture the displayed hotspot, including the renderer's even-X alignment.
    // These fields and the critical section are verified in 0x50CF90.
    const ADDRESS mouseAddress = reinterpret_cast<ADDRESS>(mouse);
    auto *criticalSection = reinterpret_cast<LPCRITICAL_SECTION>(mouseAddress + 0x78);
    EnterCriticalSection(criticalSection);
    scrollCursor = mouse->CurrentCursor();
    scrollCursorPoint = H3POINT(IntAt(mouseAddress + 0x6C), IntAt(mouseAddress + 0x70));
    LeaveCriticalSection(criticalSection);
    adv->DemobilizeHero();
    startMousePoint = lastMousePoint = point;
    startScreenPosition = adv->screenPosition;
    scrollScreenPosition = adv->screenPosition;
    startScreenOffset = scrollScreenOffset = adv->screenDrawOffset;
    rmcAtMapScreen = !middleButton;
    wheelButtonAtMapScreen = middleButton;
    lastDrawStartedAt = h3::GetTime() - MIN_DRAW_INTERVAL;
    if (middleButton)
        BeginMapDragging();
}

void MapScroller::BeginMapDragging() noexcept
{
    inMoveAction = TRUE;
    SetMapEdgeScrollStatus(FALSE);
    PinScrollCursor();
}

void MapScroller::PinScrollCursor() noexcept
{
    if (cursorPinned)
        return;
    cursorPinned = TRUE;
    H3MouseManager *mouse = P_MouseManager;
    mouse->SetCursor(scrollCursor.frame, scrollCursor.type);
    THISCALL_2(void, 0x50CF90, mouse, 1);
}

void MapScroller::ScrollMap(H3AdventureManager *adv, const H3POINT &point, const BOOL forceDraw) noexcept
{
    if (!IsScrolling())
        return;

    // A script, hero selection or level change must not restore an obsolete drag anchor.
    if (adv->centeredHero || adv->screenPosition.Mixed() != scrollScreenPosition.Mixed() ||
        adv->screenDrawOffset.x != scrollScreenOffset.x || adv->screenDrawOffset.y != scrollScreenOffset.y ||
        P_WindowManager->lastDlg != adv->dlg)
    {
        StopMapScrolling();
        return;
    }

    pendingMouseMove |= point.x != lastMousePoint.x || point.y != lastMousePoint.y;
    lastMousePoint = point;
    if (!inMoveAction)
    {
        if (!IsDragThresholdReached(point))
            return;
        BeginMapDragging();
    }
    // Still validate the camera above when idle, but do not recalculate a processed
    // mouse position. A throttled position remains pending until the next frame.
    if (!pendingMouseMove)
        return;
    const DWORD drawStartedAt = h3::GetTime();
    if (!forceDraw && drawStartedAt - lastDrawStartedAt < MIN_DRAW_INTERVAL)
        return;
    pendingMouseMove = FALSE;

    const H3POINT totalOffset(point.x - startMousePoint.x, point.y - startMousePoint.y);
    Calculator calculator(H3POINT(startScreenOffset.x + totalOffset.x, startScreenOffset.y + totalOffset.y),
                          startScreenPosition);
    const int mapSize = static_cast<int>(*P_MapSize);
    ClampCameraAxis(calculator.pos.x, calculator.point.x, scrollLimits.left, scrollLimits.right + mapSize);
    ClampCameraAxis(calculator.pos.y, calculator.point.y, scrollLimits.top, scrollLimits.bottom + mapSize);

    const H3Position position(calculator.pos.x, calculator.pos.y, startScreenPosition.GetZ());
    const BOOL tilesChanged = position.Mixed() != adv->screenPosition.Mixed();
    if (!tilesChanged && calculator.point.x == adv->screenDrawOffset.x && calculator.point.y == adv->screenDrawOffset.y)
        return;

    adv->screenPosition = position;
    scrollScreenPosition = position;
    adv->screenDrawOffset = {calculator.point.x, calculator.point.y};
    scrollScreenOffset = calculator.point;
    adv->previousMousePosition = {INT_MIN, INT_MIN};
    // Limit frame start times; rendering time must not add another 8 ms pause.
    lastDrawStartedAt = drawStartedAt;
    if (tilesChanged)
        THISCALL_7(void, 0x412BA0, adv, position, 1, 1, 0, 0, 0);
    THISCALL_6(void, 0x40F350, adv, calculator.pos.x, calculator.pos.y, position.GetZ(), 0, 0);
    P_WindowManager->H3Redraw(MAP_MARGIN, MAP_MARGIN, mapViewW, mapViewH);
    AdvanceMapDrawTime(adv, h3::GetTime());
    if (wheelButtonAtMapScreen)
        suppressMiddleButtonUp = TRUE;
}

void MapScroller::UpdateHintCamera(H3AdventureManager *adv) noexcept
{
    if (!hasHintCamera || lastHintScreenPosition.Mixed() != adv->screenPosition.Mixed() ||
        lastHintScreenOffset.x != adv->screenDrawOffset.x || lastHintScreenOffset.y != adv->screenDrawOffset.y)
    {
        adv->previousMousePosition = {INT_MIN, INT_MIN};
        lastHintScreenPosition = adv->screenPosition;
        lastHintScreenOffset = adv->screenDrawOffset;
        hasHintCamera = TRUE;
    }
}

int __stdcall MapScroller::AdvMgr_MapScreenProcedure(HiHook *h, H3AdventureManager *adv, H3Msg *msg) noexcept
{
    auto &scroller = Get();
    const H3POINT point(msg->GetX(), msg->GetY());
    const BOOL pointAtMap = scroller.IsMapPoint(point);
    const BOOL rightDown = msg->command == eMsgCommand::MOUSE_BUTTON && msg->subtype == eMsgSubtype::RBUTTON_DOWN;
    const BOOL middleDown =
        msg->command == eMsgCommand::WHEEL_BUTTON && msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_DOWN;
    const BOOL middleUp =
        msg->command == eMsgCommand::WHEEL_BUTTON && msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_UP;

    if (rightDown || middleDown)
        scroller.StopMapScrolling();
    if (middleDown)
        scroller.suppressMiddleButtonUp = FALSE;

    if (msg->command == eMsgCommand::LCLICK_OUTSIDE || msg->command == eMsgCommand::LBUTTON_UP ||
        (msg->command == eMsgCommand::MOUSE_BUTTON && msg->subtype == eMsgSubtype::LBUTTON_DOWN))
        scroller.StopMapScrolling();

    // MapClick reads cached mousePosition, even if no MouseMove preceded the click.
    if (pointAtMap && (msg->command == eMsgCommand::MOUSE_BUTTON || middleDown))
        RefreshMapMouse(adv, point);

    if (pointAtMap && (rightDown || middleDown))
        scroller.StartMapScrolling(adv, point, middleDown);

    if ((middleUp && scroller.wheelButtonAtMapScreen) ||
        (msg->command == eMsgCommand::RBUTTON_UP && scroller.rmcAtMapScreen))
    {
        // Flush the final mouse position even when the preceding move was throttled.
        scroller.ScrollMap(adv, point, TRUE);
        scroller.StopMapScrolling();
        RefreshMapMouse(adv, point);
    }
    else if (msg->command == eMsgCommand::NONE && scroller.IsScrolling() && scroller.inMoveAction)
    {
        // Complete a throttled move without requiring another physical mouse movement.
        // Poll only when the H3 input queue is empty: GetKeyState can already reflect a
        // release while older MouseMove events are still waiting in that queue.
        const BOOL buttonHeld = scroller.IsScrollButtonHeld();
        scroller.ScrollMap(adv, scroller.lastMousePoint, !buttonHeld);
        if (!buttonHeld)
            scroller.StopMapScrolling();
        if (!scroller.IsScrolling())
            RefreshMapMouse(adv, scroller.lastMousePoint);
    }
    if (middleUp && scroller.suppressMiddleButtonUp)
    {
        scroller.suppressMiddleButtonUp = FALSE;
        return 1;
    }

    return THISCALL_2(int, h->GetDefaultFunc(), adv, msg);
}

int __stdcall MapScroller::AdvMgr_MouseMove(HiHook *h, H3AdventureManager *adv, const int x, const int y) noexcept
{
    auto &scroller = Get();
    if (scroller.IsScrolling())
    {
        scroller.ScrollMap(adv, H3POINT(x, y));
        // Dragging needs only the raw position. Native MouseMove would also rebuild
        // hints and the hero path and change the cursor on every crossed object.
        if (scroller.inMoveAction)
            return 1;
    }
    scroller.UpdateHintCamera(adv);
    // Keep raw coordinates for the native viewport and interface hit tests.
    return THISCALL_3(int, h->GetDefaultFunc(), adv, x, y);
}

_LHF_(MapScroller::AdvMgr_MouseMoveCoordinates) noexcept
{
    const auto *adv = reinterpret_cast<H3AdventureManager *>(c->esi);
    c->edi -= adv->screenDrawOffset.x;
    c->ebx -= adv->screenDrawOffset.y;
    // The native division truncates; negative map pixels require floor division.
    if (c->edi < 0)
        c->edi -= MAP_TILE_SIZE - 1;
    if (c->ebx < 0)
        c->ebx -= MAP_TILE_SIZE - 1;
    return EXEC_DEFAULT;
}

_LHF_(MapScroller::MouseMgr_DrawCursorCoordinates) noexcept
{
    const auto &scroller = Get();
    if (scroller.cursorPinned)
    {
        // Shared by forced and periodic cursor redraws; the native lock is held.
        // Keep OS/input coordinates untouched so dragging can continue normally.
        IntAt(c->esi + 0x6C) = scroller.scrollCursorPoint.x;
        IntAt(c->esi + 0x70) = scroller.scrollCursorPoint.y;
    }
    return EXEC_DEFAULT;
}

void __stdcall MapScroller::WndMgr_AddNewDlg(HiHook *h, const H3WindowManager *wm, const H3BaseDlg *dlg,
                                             const int index, const int draw) noexcept
{
    THISCALL_4(void, h->GetDefaultFunc(), wm, dlg, index, draw);
    auto &scroller = Get();
    if (scroller.IsScrolling() && dlg != scroller.rightClickDlg)
        scroller.StopMapScrolling();
}

void __stdcall MapScroller::WndMgr_ShowRightClickDlg(HiHook *h, H3WindowManager *wm, const H3BaseDlg *dlg) noexcept
{
    auto &scroller = Get();
    H3AdventureManager *adv = P_AdventureManager;
    const BOOL mapPopup = dlg && adv && scroller.rmcAtMapScreen && !scroller.inMoveAction && wm->lastDlg == adv->dlg;
    if (mapPopup)
        scroller.rightClickDlg = dlg;
    else
        scroller.StopMapScrolling();

    THISCALL_2(void, h->GetDefaultFunc(), wm, dlg);

    if (mapPopup && scroller.rightClickDlg == dlg)
    {
        scroller.rightClickDlg = nullptr;
        if (scroller.rmcAtMapScreen && scroller.inMoveAction)
        {
            scroller.ScrollMap(adv, scroller.lastMousePoint, TRUE);
            if (!scroller.IsScrolling())
                RefreshMapMouse(adv, scroller.lastMousePoint);
        }
        else
            scroller.StopMapScrolling();
    }
}

_LHF_(MapScroller::BaseDlg_OnRightClickHold) noexcept
{
    auto &scroller = Get();
    const auto *dlg = *reinterpret_cast<H3BaseDlg **>(c->ebp + 0x8);
    if (scroller.rmcAtMapScreen && dlg && dlg == scroller.rightClickDlg)
    {
        auto *msg = reinterpret_cast<H3Msg *>(c->ebp - 0x34);
        if (msg->command == eMsgCommand::RBUTTON_UP || msg->command == eMsgCommand::LCLICK_OUTSIDE ||
            msg->command == eMsgCommand::LBUTTON_UP)
            scroller.StopMapScrolling();
        else if (msg->command == eMsgCommand::MOUSE_OVER &&
                 scroller.IsDragThresholdReached(H3POINT(msg->GetX(), msg->GetY())))
        {
            scroller.lastMousePoint = H3POINT(msg->GetX(), msg->GetY());
            scroller.pendingMouseMove = TRUE;
            scroller.BeginMapDragging();
            // This is the popup's local message, loaded into EAX at 0x60308D.
            msg->command = eMsgCommand::LBUTTON_UP;
        }
    }
    return EXEC_DEFAULT;
}

void MapScroller::StopMapScrolling() noexcept
{
    const BOOL restoreCursor = cursorPinned;
    cursorPinned = FALSE;
    rmcAtMapScreen = FALSE;
    wheelButtonAtMapScreen = FALSE;
    inMoveAction = FALSE;
    pendingMouseMove = FALSE;
    hasHintCamera = FALSE;
    rightClickDlg = nullptr;
    SetMapEdgeScrollStatus(TRUE);
    if (restoreCursor)
    {
        H3MouseManager *mouse = P_MouseManager;
        THISCALL_2(void, 0x50CF90, mouse, 0);
    }
}

Calculator::Calculator(const H3POINT &p, const H3Position &position) noexcept
    : pos(SignedMapCoordinate(position.GetX()), SignedMapCoordinate(position.GetY())), point(p)
{
    align();
}

void Calculator::operator+=(const H3POINT &other) noexcept
{
    point.x += other.x;
    point.y += other.y;
    align();
}

void Calculator::align() noexcept
{
    pos.x -= point.x / MAP_TILE_SIZE;
    point.x %= MAP_TILE_SIZE;
    pos.y -= point.y / MAP_TILE_SIZE;
    point.y %= MAP_TILE_SIZE;
}

void MapScroller::SetMapEdgeScrollStatus(const BOOL state) noexcept
{
    if (edgeScrollHook && edgeScrollHook->IsApplied() == !!state)
        state ? edgeScrollHook->Undo() : edgeScrollHook->Apply();
}

void __stdcall MapScroller::AdvMgr_SetActiveHero(HiHook *h, H3AdventureManager *adv, const int heroIdx, const int a3,
                                                 const char a4, const char a5) noexcept
{
    if (heroIdx != -1)
    {
        Get().StopMapScrolling();
        adv->screenDrawOffset = {};
        adv->previousMousePosition = {INT_MIN, INT_MIN};
    }
    THISCALL_5(void, h->GetDefaultFunc(), adv, heroIdx, a3, a4, a5);
}

void __stdcall MapScroller::AdvMgr_SetActiveTown(HiHook *h, H3AdventureManager *adv, const int townIdx, const int a3,
                                                 const char a4) noexcept
{
    Get().StopMapScrolling();
    adv->screenDrawOffset = {};
    adv->previousMousePosition = {INT_MIN, INT_MIN};
    THISCALL_4(void, h->GetDefaultFunc(), adv, townIdx, a3, a4);
}

void MapScroller::CreatePatches() noexcept
{
    if (!m_isInited)
    {
        _pi->WriteHiHook(0x40E2C0, THISCALL_, AdvMgr_MouseMove);
        _pi->WriteHiHook(0x408710, THISCALL_, AdvMgr_MapScreenProcedure);
        // Both points are after the raw viewport test and before native pixel-to-tile conversion.
        _pi->WriteLoHook(0x40E332, AdvMgr_MouseMoveCoordinates);
        _pi->WriteLoHook(0x40DE5B, AdvMgr_MouseMoveCoordinates);
        // After GetCursorPos (if requested), before clipping/blitting the cursor.
        _pi->WriteLoHook(0x50D032, MouseMgr_DrawCursorCoordinates);
        _pi->WriteHiHook(0x417A80, THISCALL_, AdvMgr_SetActiveHero);
        _pi->WriteHiHook(0x417790, THISCALL_, AdvMgr_SetActiveTown);
        _pi->WriteHiHook(0x602970, THISCALL_, WndMgr_AddNewDlg);
        _pi->WriteHiHook(0x603000, THISCALL_, WndMgr_ShowRightClickDlg);
        // Observe release messages before all three popup termination comparisons.
        _pi->WriteLoHook(0x60308D, BaseDlg_OnRightClickHold);
        edgeScrollHook = _pi->CreateCodePatch(0x40883A, const_cast<LPSTR>("9090909090"));
        m_isInited = true;
    }
}
} // namespace scroll
