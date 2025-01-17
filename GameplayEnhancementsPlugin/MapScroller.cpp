#include "pch.h"

namespace scroll
{

void __stdcall MapScroller::AdvMgr_SetActiveHero(HiHook *h, H3AdventureManager *adv, int heroIdx, int a3, char a4,
                                                 char a5) noexcept
{
    P_AdventureManager->screenDrawOffset.x = 0;
    P_AdventureManager->screenDrawOffset.y = 0;
    THISCALL_5(void, h->GetDefaultFunc(), adv, heroIdx, a3, a4, a5);
}

MapScroller::MapScroller() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.MapScrolling.daemon_n"))
{
    CreatePatches();
}

MapScroller &MapScroller::Get() // (PatcherInstance* _PI)
{

    static MapScroller instacne; // = MapScroller();
    return instacne;
}

_LHF_(MapScroller::BaseDlg_OnRightClickHold)
{
    auto &mapScroller = Get();

    if (mapScroller.rmcAtMapScreen) // only if rmc was inited
    {

        // get current dlg from stack
        H3BaseDlg *currentDlg = *reinterpret_cast<H3BaseDlg **>(c->ebp + 0x8);
        // find previous dlg
        H3BaseDlg *lastDlg = *reinterpret_cast<H3BaseDlg **>(reinterpret_cast<int>(currentDlg) + 0xC);

        if (lastDlg == P_AdventureManager->dlg) // only if previous dlg is same as adventuremap
        {
            H3Msg *msg = reinterpret_cast<H3Msg *>(c->ebp - 0x34); // get mouse pos
            // check if more than 25 px move
            if (msg->GetX() && (Abs(msg->GetX() - mapScroller.startMousePoint.x) > 25 ||
                                Abs(msg->GetY() - mapScroller.startMousePoint.y) > 25))
            {
                mapScroller.SetMapEdgeScrollStatus(false); // disable map edge scroll
                c->eax = 16;                               // close dlg (send click)
            }
        }
        else
        {
            mapScroller.rmcAtMapScreen = false; // disable rmc flag to have some erm scripts compatibilityw
        }
    }

    return EXEC_DEFAULT;
}

int __stdcall MapScroller::AdvMgr_MapScreenProcedure(HiHook *h, H3AdventureManager *adv, H3Msg *msg) noexcept
{
    // char middleMouse = GetAsyncKeyState(VK_MBUTTON);// &0x1;
    int mapViewW = H3GameWidth::Get() - (800 - 592);  // right panel
    int mapViewH = H3GameHeight::Get() - (600 - 544); // bottom panel
    auto &mapScroller = Get();

    mapScroller.isMapView =
        msg->GetX() >= 8 && msg->GetX() <= 8 + mapViewW && msg->position.x >= 8 && msg->position.y <= 8 + mapViewH;

    if (mapScroller.isMapView &&
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

        mapScroller.startScreenPosition = p; // { adv->screenPosition.GetX(), adv->screenPosition.GetY() };
        mapScroller.startScreenOffset = adv->screenDrawOffset;

        mapScroller.startMousePosition = adv->mousePosition;
        mapScroller.startMousePoint.x = msg->GetX();
        mapScroller.startMousePoint.y = msg->GetY();

        // Init Click Status
        mapScroller.rmcAtMapScreen = true;
        if (msg->command == eMsgCommand::WHEEL_BUTTON)
        {
            mapScroller.wheelButtonAtMapScreen = true;
            mapScroller.scrollingDone = false;
            mapScroller.SetMapEdgeScrollStatus(false);
        }
    }

    // if no more rmc pressed
    if (msg->command == eMsgCommand::RBUTTON_UP || msg->command == eMsgCommand::LCLICK_OUTSIDE ||
        msg->command == eMsgCommand::LBUTTON_UP)
    {
        mapScroller.rmcAtMapScreen = false;
        mapScroller.SetMapEdgeScrollStatus(true);
    }

    if (msg->command == eMsgCommand::MOUSE_OVER || msg->command == eMsgCommand::MOUSE_BUTTON)
    {

        // if mapView Zone then send wrong coordinates for map items under mouse
        if (mapScroller.isMapView)
        {
            msg->position.x -= adv->screenDrawOffset.x;
            msg->position.y -= adv->screenDrawOffset.y;
        }
    }
    if (mapScroller.scrollingDone && msg->command == eMsgCommand::WHEEL_BUTTON &&
        msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_UP)
    {
        mapScroller.scrollingDone = false;
        msg->command = eMsgCommand(0);
        // return 1;
    }

    return THISCALL_2(int, h->GetDefaultFunc(), adv, msg);
}

void MapScroller::CreatePatches() noexcept
{

    if (!m_isInited)
    {

        // Init Map Draw Limit Borders;

        scrollLimits.left = IntAt(0x4195F2 + 1); //
        scrollLimits.top = IntAt(0x41960A + 1);

        scrollLimits.right = CharAt(0x4195FC + 2);
        scrollLimits.bottom = CharAt(0x419615 + 2);

        _pi->WriteHiHook(0x40E2C0, THISCALL_, AdvMgr_MouseMove);
        _pi->WriteHiHook(0x408710, THISCALL_, AdvMgr_MapScreenProcedure);
        _pi->WriteHiHook(0x417A80, THISCALL_, AdvMgr_SetActiveHero);

        _pi->WriteLoHook(0x60309A, BaseDlg_OnRightClickHold);
        _pi->WriteLoHook(0x417548, AdvMgr_MobilizeCurrentHero);

        edgeScrollHook = _pi->CreateCodePatch(0x40883A, (char *)"9090909090"); // nop for the edge scroll

        m_isInited = true;
    }
}

int __stdcall MapScroller::AdvMgr_MouseMove(HiHook *h, H3AdventureManager *adv, int x, int y) noexcept
{

    auto &mapScroller = Get();
    if (mapScroller.rmcAtMapScreen && STDCALL_1(SHORT, PtrAt(0x63A294), VK_RBUTTON) & 0x800 ||
        mapScroller.wheelButtonAtMapScreen && STDCALL_1(SHORT, PtrAt(0x63A294), VK_MBUTTON) & 0x800) // if rmcIs held
    {
        if (!adv->centeredHero)
        {

            if (CDECL_0(DWORD, 0x4F8970) - mapScroller.sinceLastDrawTime >= 8) // thanks to baratorch for idea
            {
                int mapX = adv->screenPosition.GetX();
                int mapY = adv->screenPosition.GetY();
                int mapZ = adv->screenPosition.GetZ();
                int mapSize = *P_MapSize;

                if (mapX >= mapSize)
                    mapX -= 1024; // draw outside map tiles
                if (mapY >= mapSize)
                    mapY -= 1024;

                H3POINT totalOffset(x - mapScroller.startMousePoint.x, y - mapScroller.startMousePoint.y);

                mapScroller.calculator = {
                    mapScroller.startScreenOffset,
                    H3Position(mapScroller.startScreenPosition.x, mapScroller.startScreenPosition.y, mapZ)};

                mapScroller.calculator += totalOffset;
                if (mapScroller.isMapView) // if Map View Zone hten return draw offset to properly calc
                    mapScroller.calculator += adv->screenDrawOffset;

                P_AdventureManager->screenDrawOffset.x = mapScroller.calculator.point.x;
                P_AdventureManager->screenDrawOffset.y = mapScroller.calculator.point.y;

                //	showCurrent(x, y);
                mapX = mapScroller.calculator.pos.x;
                mapY = mapScroller.calculator.pos.y;

                mapX = Clamp(mapScroller.scrollLimits.left, mapX, mapScroller.scrollLimits.right + *P_MapSize);
                mapY = Clamp(mapScroller.scrollLimits.top, mapY, mapScroller.scrollLimits.bottom + *P_MapSize);
                //	toEcho = H3String::Format(toEcho.String(), totalOffset.x, totalOffset.y, calc.pos.x, calc.pos.y,
                //startScreenPosition.x, startScreenPosition.y,mapX,mapY);

                // limit out of bounds for pixels
                if (mapX == mapScroller.scrollLimits.left || mapX == mapScroller.scrollLimits.right + *P_MapSize)
                    P_AdventureManager->screenDrawOffset.x = 0;

                if (mapY == mapScroller.scrollLimits.top || mapY == mapScroller.scrollLimits.bottom + *P_MapSize)
                    P_AdventureManager->screenDrawOffset.y = 0;

                adv->screenPosition.SetX(mapX);
                adv->screenPosition.SetY(mapY);

                // call "AdvMgr_Draw_MiniMap"
                THISCALL_7(void, 0x412BA0, adv, adv->screenPosition, 1, 1, 0, 0, 0);
                // call "AdvMgr_Draw_Map"
                THISCALL_6(char, 0x40F350, adv, mapX, mapY, mapZ, 0, 0);

                // redraw the whole map for the players
                P_WindowManager->H3Redraw(0, 8, IntAt(0x4196EA + 1), IntAt(0x4196E5 + 1));

                // update draw sinceLastDrawTime timer
                mapScroller.sinceLastDrawTime = CDECL_0(DWORD, 0x4F8970);
                mapScroller.scrollingDone = true;
            }
        }
    }
    else // if not holding then reset variable
    {
        mapScroller.rmcAtMapScreen = false;
        mapScroller.SetMapEdgeScrollStatus(true);
    }

    return THISCALL_3(int, h->GetDefaultFunc(), adv, x, y);
}

MapScroller::Calculator::Calculator(const H3POINT &p, const H3Position &pos) : point(p), pos(pos.GetX(), pos.GetY())
{
    int mapX = pos.GetX();
    int mapY = pos.GetY();
    int mapSize = *P_MapSize;

    if (mapX >= mapSize)
        mapX -= 1024; // draw outside map tiles
    if (mapY >= mapSize)
        mapY -= 1024;

    this->pos.x = mapX;
    this->pos.y = mapY;

    align();
}

void MapScroller::Calculator::operator+=(const H3POINT &other)
{
    point.x += other.x;
    point.y += other.y;
    align();
}

void MapScroller::Calculator::align() noexcept
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
void MapScroller::SetMapEdgeScrollStatus(const bool state)
{
    if (edgeScrollHook)
    {
        state ? edgeScrollHook->Undo() : edgeScrollHook->Apply();
    }
}

_LHF_(MapScroller::AdvMgr_MobilizeCurrentHero)
{
    // restore map draw offset when selet hero
    P_AdventureManager->screenDrawOffset.x = 0;
    P_AdventureManager->screenDrawOffset.y = 0;
    return EXEC_DEFAULT;
}

} // namespace scroll
