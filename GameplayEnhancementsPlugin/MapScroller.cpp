#include "pch.h"

namespace scroll
{

MapScroller::MapScroller() noexcept
    : IGamePatch(globalPatcher->CreateInstance("EraPlugin.MapScrolling.daemon_n")),
      scrollLimits{IntAt(0x4195F2 + 1), IntAt(0x41960A + 1), CharAt(0x4195FC + 2), // init screen size
                   CharAt(0x419615 + 2)},
      mapViewW(H3GameWidth::Get() - 208), mapViewH(H3GameHeight::Get() - 56) // bottom panel
{

    CreatePatches();
}
MapScroller &MapScroller::Get() noexcept
{

    static MapScroller instacne; // = MapScroller();
    return instacne;
}

void __stdcall MapScroller::WndMgr_AddNewDlg(HiHook *h, const H3WindowManager *wm, const H3BaseDlg *dlg,
                                             const int index, const int draw) noexcept
{

    THISCALL_4(void, h->GetDefaultFunc(), wm, dlg, index, draw);
    auto &mapScroller = Get();

    if (mapScroller.rmcAtMapScreen)
    {
        if (mapScroller.lastAdddedDlg)
        {
            mapScroller.Stop();
        }
        else
        {
            mapScroller.lastAdddedDlg = dlg;
        }
    }
}

_LHF_(MapScroller::BaseDlg_OnRightClickHold) noexcept
{
    auto &mapScroller = Get();

    if (mapScroller.rmcAtMapScreen) // only if rmc was inited
    {
        // get current dlg from stack
        H3BaseDlg *currentDlg = *reinterpret_cast<H3BaseDlg **>(c->ebp + 0x8);
        // find previous dlg
        H3BaseDlg *lastDlg = *reinterpret_cast<H3BaseDlg **>(reinterpret_cast<int>(currentDlg) + 0xC);

        if (lastDlg == P_AdventureManager->dlg &&
            mapScroller.lastAdddedDlg == currentDlg) // only if previous dlg is same as adventuremap
        {
            H3Msg *msg = reinterpret_cast<H3Msg *>(c->ebp - 0x34); // get mouse pos
            // check if more than 25 px move
            if (msg->GetX() && (Abs(msg->GetX() - mapScroller.startMousePoint.x) > DRAG_MOUSE_ACCESS ||
                                Abs(msg->GetY() - mapScroller.startMousePoint.y) > DRAG_MOUSE_ACCESS))
            {
                mapScroller.SetMapEdgeScrollStatus(false); // disable map edge scroll
                c->eax = eMsgCommand::LBUTTON_UP;          // close dlg (send click)
            }
        }
        else
        {
            mapScroller.Stop(); // disable rmc flag to have some erm scripts compatibilityw
        }
    }

    return EXEC_DEFAULT;
}

int __stdcall MapScroller::AdvMgr_MapScreenProcedure(HiHook *h, H3AdventureManager *adv, H3Msg *msg) noexcept
{
    // char middleMouse = GetAsyncKeyState(VK_MBUTTON);// &0x1;

    auto &scroller = Get();

    scroller.isMapView = msg->GetX() >= MAP_MARGIN && msg->GetX() <= MAP_MARGIN + scroller.mapViewW &&
                         msg->position.x >= MAP_MARGIN && msg->position.y <= MAP_MARGIN + scroller.mapViewH;

    if (scroller.isMapView &&
        (msg->command == eMsgCommand::MOUSE_BUTTON && msg->subtype == eMsgSubtype::RBUTTON_DOWN ||
         msg->command == eMsgCommand::WHEEL_BUTTON && msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_DOWN)

    )
    {
        // detach current hero from drawing at screen center
        P_AdventureManager->DemobilizeHero();

        // store start screed data
        int mapX = adv->screenPosition.GetX();
        int mapY = adv->screenPosition.GetY();
        H3POINT p(mapX, mapY);

        scroller.startScreenPosition = p; // { adv->screenPosition.GetX(), adv->screenPosition.GetY() };
        scroller.startScreenOffset = adv->screenDrawOffset;

        scroller.startMousePosition = adv->mousePosition;
        scroller.startMousePoint.x = msg->GetX();
        scroller.startMousePoint.y = msg->GetY();

        // Init Click Status
        scroller.rmcAtMapScreen = true;
        // lastWindowAnswer = P_WindowManager->resultItemID;
        if (msg->command == eMsgCommand::WHEEL_BUTTON)
        {
            scroller.wheelButtonAtMapScreen = true;
            scroller.scrollingDone = false;
            scroller.SetMapEdgeScrollStatus(false);
        }
    }

    // if no more rmc pressed
    if (msg->command == eMsgCommand::RBUTTON_UP || msg->command == eMsgCommand::LCLICK_OUTSIDE ||
        msg->command == eMsgCommand::LBUTTON_UP)
    {
        scroller.Stop();
    }

    if (msg->command == eMsgCommand::MOUSE_OVER || msg->command == eMsgCommand::MOUSE_BUTTON)
    {

        // if mapView Zone then send wrong coordinates for map items under mouse
        if (scroller.isMapView)
        {
            msg->position.x -= adv->screenDrawOffset.x;
            msg->position.y -= adv->screenDrawOffset.y;
        }
    }
    if (scroller.scrollingDone && msg->command == eMsgCommand::WHEEL_BUTTON &&
        msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_UP)
    {
        scroller.scrollingDone = false;
        msg->command = eMsgCommand(0);
    }

    return THISCALL_2(int, h->GetDefaultFunc(), adv, msg);
}

int __stdcall MapScroller::AdvMgr_MouseMove(HiHook *h, H3AdventureManager *adv, const int x, const int y) noexcept
{

    auto &mapScroller = Get();
    if (mapScroller.rmcAtMapScreen && STDCALL_1(SHORT, PtrAt(0x63A294), VK_RBUTTON) & 0x800 ||
        mapScroller.wheelButtonAtMapScreen && STDCALL_1(SHORT, PtrAt(0x63A294), VK_MBUTTON) & 0x800) // if rmcIs held
    {
        if (!adv->centeredHero)
        {
            if (h3::GetTime() - mapScroller.sinceLastDrawTime >= 8) // thanks to baratorch for idea
            {
                int mapX = adv->screenPosition.GetX();
                int mapY = adv->screenPosition.GetY();
                const int mapZ = adv->screenPosition.GetZ();
                const size_t mapSize = *P_MapSize;

                if (mapX >= mapSize)
                    mapX -= 1024; // draw outside map tiles
                if (mapY >= mapSize)
                    mapY -= 1024;

                H3POINT totalOffset(x - mapScroller.startMousePoint.x, y - mapScroller.startMousePoint.y);

                static Calculator calculator;
                calculator = {mapScroller.startScreenOffset,
                              H3Position(mapScroller.startScreenPosition.x, mapScroller.startScreenPosition.y, mapZ)};

                calculator += totalOffset;
                if (mapScroller.isMapView) // if Map View Zone hten return draw offset to properly calc
                    calculator += adv->screenDrawOffset;

                P_AdventureManager->screenDrawOffset.x = calculator.point.x;
                P_AdventureManager->screenDrawOffset.y = calculator.point.y;

                //	showCurrent(x, y);
                mapX = calculator.pos.x;
                mapY = calculator.pos.y;

                mapX = Clamp(mapScroller.scrollLimits.left, mapX, mapScroller.scrollLimits.right + mapSize);
                mapY = Clamp(mapScroller.scrollLimits.top, mapY, mapScroller.scrollLimits.bottom + mapSize);
                //	toEcho = H3String::Format(toEcho.String(), totalOffset.x, totalOffset.y, calc.pos.x, calc.pos.y,

                // limit out of bounds for pixels
                if (mapX == mapScroller.scrollLimits.left || mapX == mapScroller.scrollLimits.right + mapSize)
                    P_AdventureManager->screenDrawOffset.x = 0;

                if (mapY == mapScroller.scrollLimits.top || mapY == mapScroller.scrollLimits.bottom + mapSize)
                    P_AdventureManager->screenDrawOffset.y = 0;

                adv->screenPosition.SetXYZ(mapX, mapY, mapZ);

                // call "AdvMgr_Draw_MiniMap"
                THISCALL_7(void, 0x412BA0, adv, adv->screenPosition, 1, 1, 0, 0, 0);
                // call "AdvMgr_Draw_Map"
                THISCALL_6(char, 0x40F350, adv, mapX, mapY, mapZ, 0, 0);

                // redraw the whole map for the players
                P_WindowManager->H3Redraw(MAP_MARGIN, MAP_MARGIN, IntAt(0x4196EA + 1), IntAt(0x4196E5 + 1));

                // update draw sinceLastDrawTime timer
                mapScroller.sinceLastDrawTime = h3::GetTime();
                mapScroller.scrollingDone = true;
            }
        }
    }
    else // if not holding then reset variable
    {
        mapScroller.Stop();
    }

    return THISCALL_3(int, h->GetDefaultFunc(), adv, x, y);
}

void MapScroller::Stop() noexcept
{
    rmcAtMapScreen = false;
    SetMapEdgeScrollStatus(true);
    lastAdddedDlg = nullptr;
}
Calculator::Calculator(const H3POINT &p, const H3Position &pos) noexcept : point(p), pos(pos.GetX(), pos.GetY())
{
    UINT mapX = pos.GetX();
    UINT mapY = pos.GetY();
    int mapSize = *P_MapSize;

    if (mapX >= mapSize)
        mapX -= 1024; // draw outside map tiles
    if (mapY >= mapSize)
        mapY -= 1024;

    this->pos.x = mapX;
    this->pos.y = mapY;

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
    if (point.x > 31 || point.x < -31)
    {
        pos.x -= point.x / 31;
        point.x %= 31;
    }

    if (point.y > 31 || point.y < -31)
    {
        pos.y -= point.y / 31;
        point.y %= 31;
    }
}
void MapScroller::SetMapEdgeScrollStatus(const BOOL state) noexcept
{
    if (edgeScrollHook)
    {
        state ? edgeScrollHook->Undo() : edgeScrollHook->Apply();
    }
}

_LHF_(MapScroller::AdvMgr_MobilizeCurrentHero) noexcept
{
    // restore map draw offset when selet hero
    P_AdventureManager->screenDrawOffset.x = 0;
    P_AdventureManager->screenDrawOffset.y = 0;
    return EXEC_DEFAULT;
}
void __stdcall MapScroller::AdvMgr_SetActiveHero(HiHook *h, H3AdventureManager *adv, const int heroIdx, const int a3,
                                                 const char a4, const char a5) noexcept
{
    P_AdventureManager->screenDrawOffset.x = 0;
    P_AdventureManager->screenDrawOffset.y = 0;
    THISCALL_5(void, h->GetDefaultFunc(), adv, heroIdx, a3, a4, a5);
}

void MapScroller::CreatePatches() noexcept
{

    if (!m_isInited)
    {

        // Init Map Draw Limit Borders;

        // handle scrolling process
        _pi->WriteHiHook(0x40E2C0, THISCALL_, AdvMgr_MouseMove);
        // handle scrolling data initialization
        _pi->WriteHiHook(0x408710, THISCALL_, AdvMgr_MapScreenProcedure);

        // handle proper hero drawing when scrolling
        _pi->WriteHiHook(0x417A80, THISCALL_, AdvMgr_SetActiveHero);
        _pi->WriteLoHook(0x417548, AdvMgr_MobilizeCurrentHero);

        // handle extra dlg creation before popup dlg
        _pi->WriteHiHook(0x0602970, THISCALL_, WndMgr_AddNewDlg);
        // handle popup dlg closing
        _pi->WriteLoHook(0x60309A, BaseDlg_OnRightClickHold);

        // init patch to disable map edge scroll
        edgeScrollHook = _pi->CreateCodePatch(0x40883A, const_cast<LPSTR>("9090909090")); // nop for the edge scroll

        m_isInited = true;
    }
}

} // namespace scroll
