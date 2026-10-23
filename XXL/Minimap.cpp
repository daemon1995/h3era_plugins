#include "pch.h"
#include <cmath>

namespace
{
// Floor division also keeps the viewport's clipped top/left edges outside the radar.
int RadarCoordinate(int tile, int pixels, int mapSize)
{
    const int value = tile * pixels;
    return value >= 0 ? value / mapSize : -((-value + mapSize - 1) / mapSize);
}

bool IsLargeMap()
{
    return IntAt(0x6783C8) > 144 && IntAt(0x6783CC) > 144;
}

bool HasXxlRadarFrames(H3LoadedDef *radar)
{
    if (!radar || radar->groupsCount < 1 || !radar->groups || !radar->groups[0] || radar->groups[0]->count != 29)
        return false;
    // XXL orders S..G in ascending map size; retail orders XL..S instead.
    const H3DefFrame *small = radar->GetGroupFrame(0, 1);
    const H3DefFrame *xl = radar->GetGroupFrame(0, 4);
    return small && xl && small->frameWidth == 76 && small->frameHeight == 68 && xl->frameWidth == 19 &&
           xl->frameHeight == 17;
}

int XxlRadarFrame(int mapSize, bool viewWorld, DWORD zoom)
{
    if (mapSize < 36 || mapSize > 252 || mapSize % 36)
        return -1;
    const int sizeIndex = mapSize / 36;
    if (!viewWorld)
        return sizeIndex;
    switch (zoom)
    {
    case 0x41800000:
        return 7 + sizeIndex; // 16.0
    case 0x413D70A4:
        return 14 + sizeIndex; // 11.84
    case 0x40F5C28F:
        return 21 + sizeIndex; // 7.68
    default:
        return -1;
    }
}

float RadarMouseDivisor(int pixels)
{
    const int size = IntAt(0x6783CC);
    const float scale = float(pixels) / size;
    // A divisor rounded upwards would send exact pixel boundaries one tile back
    // when the native x87 conversion truncates (e.g. pixel 72 on an XH map).
    return double(scale) * size > pixels ? std::nextafterf(scale, 0.0f) : scale;
}

_LHF_(RadarMouseScale)
{
    if (IsLargeMap() && !GetModuleHandleA("HD_WOG.dll"))
    {
        H3AdventureManager *adv = reinterpret_cast<H3AdventureManager *>(c->ebx);
        *reinterpret_cast<float *>(c->ebp - 8) = RadarMouseDivisor(adv->dlg->minimapTransparentOverlay->GetWidth());
    }
    // The local divisor is shared by the initial click and the entire drag loop.
    return EXEC_DEFAULT;
}

_LHF_(ViewWorldRadarMouseScale)
{
    if (IsLargeMap() && !GetModuleHandleA("HD_WOG.dll"))
        *reinterpret_cast<float *>(c->ebp + 0x10) =
            RadarMouseDivisor(P_AdventureManager->dlg->minimapTransparentOverlay->GetWidth());
    return EXEC_DEFAULT;
}

_LHF_(DrawLargeMapPixel)
{
    if (!IsLargeMap())
        return EXEC_DEFAULT;

    // 0x412BA0 has already resolved visibility, terrain, heroes and object owners.
    // Its native switch has no pixel-writing case above XL. EDI is unused here.
    const int x = IntAt(c->ebp - 0x14) + RadarCoordinate(IntAt(c->ebp - 0x1C), IntAt(c->ebp - 0x28), IntAt(0x6783C8));
    const int y = IntAt(c->ebp - 0x20) + RadarCoordinate(IntAt(c->ebp - 0x10), IntAt(c->ebp - 0x24), IntAt(0x6783CC));
    H3LoadedPcx16 *screen = P_WindowManager->screenPcx16;
    if (x >= 0 && y >= 0 && x < screen->width && y < screen->height)
        *reinterpret_cast<UINT16 *>(screen->buffer + y * screen->scanlineSize + 2 * x) = UINT16(c->esi);

    c->return_address = 0x4131D7;
    return NO_EXEC_DEFAULT;
}

_LHF_(DrawRadarFrame)
{
    H3AdventureManager *adv = *reinterpret_cast<H3AdventureManager **>(c->ebp - 8);
    H3LoadedDef *radar = adv->radarDef;
    const bool xxlFrames = HasXxlRadarFrames(radar);
    const bool viewWorld = IntAt(0x6AACA4) != 0;
    const int xxlFrame = XxlRadarFrame(IntAt(0x6783CC), viewWorld, DwordAt(0x68C708));
    if (xxlFrames && xxlFrame >= 0)
    {
        IntAt(c->ebp + 0x14) = xxlFrame;
        if (IsLargeMap())
            *reinterpret_cast<float *>(c->ebp + 0x18) = float(IntAt(c->ebp - 0x28)) / IntAt(0x6783CC);
        const H3DefFrame *frame = radar->GetGroupFrame(0, xxlFrame);
        // Empty View World frames mean that the whole map already fits on screen.
        IntAt(c->ebp + 0x1C) = !frame->frameWidth || !frame->frameHeight;
        // Frames are already sized for H/XH/G. Native drawing preserves their
        // dashed border, margins and palette without scaling them a second time.
        return EXEC_DEFAULT;
    }
    if (!IsLargeMap())
        return EXEC_DEFAULT;

    // The native frame selection (including View World zoom) has finished, but
    // its subsequent coordinates assume scale 1 for every map larger than L.
    if (!IntAt(c->ebp + 0x1C))
    {
        const int frameIndex = xxlFrames ? XxlRadarFrame(144, viewWorld, DwordAt(0x68C708)) : IntAt(c->ebp + 0x14);
        if (radar && radar->groups && radar->groups[0] && frameIndex >= 0 && frameIndex < radar->groups[0]->count)
        {
            H3LoadedPcx16 *frame = H3LoadedPcx16::Create(radar->widthDEF, radar->heightDEF);
            if (frame)
            {
                // Two different backgrounds identify transparent pixels without reserving
                // a color that a replacement radar.def might itself use.
                H3LoadedPcx16 *mask = H3LoadedPcx16::Create(radar->widthDEF, radar->heightDEF);
                if (mask)
                {
                    frame->FillRectangle(0, 0, frame->width, frame->height, DWORD(0));
                    mask->FillRectangle(0, 0, mask->width, mask->height, DWORD(0xFFFF));
                    radar->DrawToPcx16(0, frameIndex, frame, 0, 0);
                    radar->DrawToPcx16(0, frameIndex, mask, 0, 0);

                    // Signed 10-bit coordinates: the normal viewport can start at -9/-8.
                    const int packed = IntAt(c->ebp + 8);
                    const int originX = (INT16(WORD(packed) << 6)) >> 6;
                    const int originY = (INT16(WORD(UINT32(packed) >> 16) << 6)) >> 6;
                    const int left = IntAt(c->ebp - 0x14);
                    const int top = IntAt(c->ebp - 0x20);
                    const int width = IntAt(c->ebp - 0x28);
                    const int height = IntAt(c->ebp - 0x24);
                    H3LoadedPcx16 *screen = P_WindowManager->screenPcx16;
                    // Forward mapping retains the DEF's thin borders when shrinking.
                    for (int sy = 0; sy < frame->height; ++sy)
                        for (int sx = 0; sx < frame->width; ++sx)
                        {
                            const UINT16 color = *reinterpret_cast<UINT16 *>(frame->GetRow(sy) + 2 * sx);
                            if (color != *reinterpret_cast<UINT16 *>(mask->GetRow(sy) + 2 * sx))
                                continue;
                            const int x = left + RadarCoordinate(originX + sx, width, IntAt(0x6783C8));
                            const int y = top + RadarCoordinate(originY + sy, height, IntAt(0x6783CC));
                            if (x >= left && y >= top && x < left + width && y < top + height && x >= 0 && y >= 0 &&
                                x < screen->width && y < screen->height)
                                *reinterpret_cast<UINT16 *>(screen->GetRow(y) + 2 * x) = color;
                        }
                    mask->Dereference();
                }
                frame->Dereference();
            }
        }
    }
    // Keep the native redraw flag and screen update.
    c->return_address = 0x413504;
    return NO_EXEC_DEFAULT;
}
} // namespace

void Minimap_Init()
{
    // HD_WOG replaces 0x412BA0 and does not execute these native instruction sites.
    _PI->WriteLoHook(0x4130DF, DrawLargeMapPixel);
    _PI->WriteLoHook(0x41336E, DrawRadarFrame);
    _PI->WriteLoHook(0x40A0AD, RadarMouseScale);
    _PI->WriteLoHook(0x5FCCA4, ViewWorldRadarMouseScale);
}
