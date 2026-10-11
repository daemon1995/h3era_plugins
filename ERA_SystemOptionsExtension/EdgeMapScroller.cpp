#include "MapScroller.h"

namespace scroll
{
namespace
{
constexpr ADDRESS MAP_VIEW_ACTIVE = 0x6AACA4;
constexpr ADDRESS MAP_WIDTH = 0x6783C8;
constexpr ADDRESS MAP_HEIGHT = 0x6783CC;
constexpr ADDRESS MAP_SCROLL_SPEEDS = 0x63A66C;
constexpr ADDRESS MAP_VIEW_SCALE = 0x68C70C;
constexpr ADDRESS MAP_VIEW_DRAW_OFFSET_X = 0x6AAC80;
constexpr ADDRESS MAP_VIEW_DRAW_OFFSET_Y = 0x6AAC84;
constexpr DWORD EDGE_SCROLL_INTERVAL = 70;
constexpr int MAP_VIEW_TOP = 8;
constexpr int MAP_VIEW_WIDTH_PIXELS = 608;
constexpr int MAP_VIEW_HEIGHT_PIXELS = 544;
constexpr int EDGE_MARGIN = 16;

DWORD lastScrollAt = 0;

int Clamp(const int value, const int minValue, const int maxValue) noexcept
{
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

class ViewWorldMapEdgeScroll final : public IGamePatch
{
    static ViewWorldMapEdgeScroll *instance;

    ViewWorldMapEdgeScroll() noexcept
        : IGamePatch(globalPatcher->CreateInstance("EraPlugin.ViewWorldMapEdgeScroll.daemon_n"))
    {
        _pi->WriteHiHook(0x5FCEC0, THISCALL_, ViewWorldDialogProcedure);
        SetEnabled(true);
    }

  public:
    static ViewWorldMapEdgeScroll &Get() noexcept
    {
        if (!instance)
            instance = new ViewWorldMapEdgeScroll();
        return *instance;
    }

    void CreatePatches() noexcept override
    {
    }

    static int __stdcall ViewWorldDialogProcedure(HiHook *hook, H3BaseDlg *dlg, H3Msg *msg) noexcept
    {
        const eMsgCommand command = msg->command;
        const int result = THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
        if (ByteAt(MAP_VIEW_ACTIVE) && (command == eMsgCommand::MOUSE_OVER || command == eMsgCommand::NONE))
            ScrollAtEdge(dlg, H3POINT::GetCursorPosition());
        return result;
    }

    static void ScrollAtEdge(H3BaseDlg *dlg, const H3POINT &cursor) noexcept
    {
        const int viewWidth = IntAt(reinterpret_cast<ADDRESS>(dlg) + 0x70);
        const int viewHeight = IntAt(reinterpret_cast<ADDRESS>(dlg) + 0x74);
        const int viewScale = IntAt(MAP_VIEW_SCALE);
        if (viewWidth <= 0 || viewHeight <= 0 || viewScale <= 0)
            return;

        // These offsets and the zoom determine where the renderer puts the actual tile grid.
        const int mapLeft = IntAt(MAP_VIEW_DRAW_OFFSET_X);
        const int mapTop = MAP_VIEW_TOP + IntAt(MAP_VIEW_DRAW_OFFSET_Y);
        const int rawRight = mapLeft + viewWidth * viewScale;
        const int rawBottom = mapTop + viewHeight * viewScale;
        const int mapRight = rawRight < MAP_VIEW_WIDTH_PIXELS ? rawRight : MAP_VIEW_WIDTH_PIXELS;
        const int mapBottom = rawBottom < MAP_VIEW_TOP + MAP_VIEW_HEIGHT_PIXELS ? rawBottom
                                                                               : MAP_VIEW_TOP + MAP_VIEW_HEIGHT_PIXELS;
        if (cursor.x < mapLeft || cursor.x >= mapRight || cursor.y < mapTop || cursor.y >= mapBottom)
            return;

        const int left = mapLeft + EDGE_MARGIN;
        const int right = mapRight - EDGE_MARGIN;
        const int top = mapTop + EDGE_MARGIN;
        const int bottom = mapBottom - EDGE_MARGIN;

        int dx = cursor.x < left ? -1 : (cursor.x >= right ? 1 : 0);
        int dy = cursor.y < top ? -1 : (cursor.y >= bottom ? 1 : 0);
        if (!dx && !dy)
            return;

        const DWORD now = h3::GetTime();
        if (now - lastScrollAt < EDGE_SCROLL_INTERVAL)
            return;
        lastScrollAt = now;

        int speed = IntAt(0x6987DC);
        if (speed < 0 || speed > 2)
            speed = IntAt(0x6987DC) = 0;
        const int step = IntAt(MAP_SCROLL_SPEEDS + speed * sizeof(int));

        const ADDRESS viewPositionAddress = reinterpret_cast<ADDRESS>(dlg) + 0x6C;
        H3Position viewPosition(DwordAt(viewPositionAddress));
        const int mapWidth = IntAt(MAP_WIDTH);
        const int mapHeight = IntAt(MAP_HEIGHT);

        const int x = Clamp(static_cast<int>(viewPosition.GetX()) + dx * step, 0, mapWidth - viewWidth);
        const int y = Clamp(static_cast<int>(viewPosition.GetY()) + dy * step, 0, mapHeight - viewHeight);
        if (x == static_cast<int>(viewPosition.GetX()) && y == static_cast<int>(viewPosition.GetY()))
            return;

        viewPosition.SetX(static_cast<INT16>(x));
        viewPosition.SetY(static_cast<INT16>(y));
        DwordAt(viewPositionAddress) = viewPosition.Mixed();

        const int z = viewPosition.GetZ();
        // The native view dialog gives MiniMap the top-left of its visible tile range.
        // Passing the center shifts the radar viewport rectangle by half the view size.
        THISCALL_7(void, 0x412BA0, P_AdventureManager->Get(), viewPosition.Mixed(), 1, 1, ByteAt(0x6AABD0),
                   ByteAt(0x6AAC98), ByteAt(0x6AAC7C));
        THISCALL_6(void, 0x5FC870, P_AdventureManager->Get(), x, y, z, viewWidth, viewHeight);
        // Redraw the whole view dialog, matching the native map-view redraw path.
        THISCALL_5(void, 0x603190, P_WindowManager->Get(), 0, 0, 800, 600);
    }
};

ViewWorldMapEdgeScroll *ViewWorldMapEdgeScroll::instance = nullptr;
} // namespace

void InitializeViewWorldMapEdgeScroll() noexcept
{
    (void)ViewWorldMapEdgeScroll::Get();
}
} // namespace scroll
