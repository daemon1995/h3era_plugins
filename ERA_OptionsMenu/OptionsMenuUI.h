#pragma once
#include "pch.h"
#include "OptionMenuModel.h"

namespace era_options
{
class OptionsDialog : public H3Dlg
{
    void UpdateShadow()
    {
        constexpr int ShadowFlag = 0x10; // DF_SHADOW; keep DF_SCREENSHOT (0x02).
        if (DialogShadowFits(xDlg, yDlg, widthDlg, heightDlg, H3GameWidth::Get(), H3GameHeight::Get()))
            flags |= ShadowFlag;
        else flags &= ~ShadowFlag;
    }
  protected:
    INT vShow(INT zorder, BOOL8 draw) override
    {
        UpdateShadow(); // Use the final position, including native popup placement.
        return H3Dlg::vShow(zorder, draw);
    }
  public:
    OptionsDialog(int width, int height) : H3Dlg(width, height, -1, -1, FALSE, FALSE)
    {
        UpdateShadow();
    }
};

inline H3LoadedPcx16 *TiledBackground(int width, int height)
{
    auto *canvas = H3LoadedPcx16::Create(width, height);
    if (!canvas) return nullptr;
    canvas->FillRectangle(0, 0, width, height, 0, 0, 0);
    H3PcxLoader tile(NH3Dlg::Assets::DIBOXBACK);
    // Clip source rectangles; HD otherwise reads past the tile on the edges.
    if (tile.Get() && tile->width > 0 && tile->height > 0)
        for (int y = 0; y < height; y += tile->height)
            for (int x = 0; x < width; x += tile->width)
                tile->DrawToPcx16(0, 0, (std::min)(tile->width, width - x),
                    (std::min)(tile->height, height - y), canvas, x, y, FALSE);
    return canvas;
}
}
