#pragma once
#include "../headers/DrawPcx16ResizedBicubic.h"

// Shared geometry for the menu background and widget panel.
// Include after header.h; coordinates here are absolute draw-buffer pixels.
namespace mainmenu
{
namespace background
{

struct Layout
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int assetX = 0;
    int assetY = 0;
    int assetWidth = 0;
    int assetHeight = 0;
    bool frame = false;
    bool custom = false;
};

inline Layout Calculate(int assetWidth, int assetHeight, int screenWidth, int screenHeight,
                        int requestedWidth = 0, int requestedHeight = 0, bool frame = false)
{
    Layout result;
    if (assetWidth <= 0 || assetHeight <= 0 || screenWidth <= 0 || screenHeight <= 0)
        return result;

    // The frame is outside the image area. Reserve its four-pixel edges so
    // neither the image nor the frame is cropped at the window boundary.
    result.frame = frame && screenWidth > 8 && screenHeight > 8;
    const int border = result.frame ? 4 : 0;
    result.width = std::min(requestedWidth > 0 ? requestedWidth : assetWidth, screenWidth - border * 2);
    result.height = std::min(requestedHeight > 0 ? requestedHeight : assetHeight, screenHeight - border * 2);
    result.x = (screenWidth - result.width) / 2;
    result.y = (screenHeight - result.height) / 2;

    // Fit inside the configured canvas without distorting the image. With
    // automatic dimensions, native-size images are never enlarged.
    result.assetWidth = result.width;
    const auto scaledHeight = static_cast<long long>(assetHeight) * result.width / assetWidth;
    if (scaledHeight <= result.height)
        result.assetHeight = std::max(1, static_cast<int>(scaledHeight));
    else
    {
        result.assetHeight = result.height;
        result.assetWidth = std::max(1, static_cast<int>(static_cast<long long>(assetWidth) * result.height / assetHeight));
    }
    result.assetX = (screenWidth - result.assetWidth) / 2;
    result.assetY = (screenHeight - result.assetHeight) / 2;
    result.custom = true;
    return result;
}

inline h3::H3LoadedPcx16 *GetAsset()
{
    // MainLoop loads Gamselbk.pcx here and releases it when leaving the menu.
    // Mainmenu.pcx at 0x699564 is the separate overlay used by credits.
    return *reinterpret_cast<h3::H3LoadedPcx16 **>(0x699568);
}

inline bool Enabled()
{
    return config::Enabled("era.main_menu.background.enabled", true);
}

inline Layout GetLayout(h3::H3BaseDlg *dlg, bool configured)
{
    auto *screen = h3::H3WindowManager::Get()->GetDrawBuffer();
    auto *asset = GetAsset();
    if (!configured || !screen || !asset || !asset->buffer || asset->width <= 0 || asset->height <= 0)
    {
        Layout fallback;
        fallback.x = dlg->GetX();
        fallback.y = dlg->GetY();
        fallback.width = dlg->GetWidth();
        fallback.height = dlg->GetHeight();
        return fallback;
    }
    return Calculate(asset->width, asset->height, screen->width, screen->height,
                     config::Int("era.main_menu.background.width", 0),
                     config::Int("era.main_menu.background.height", 0),
                     config::Enabled("era.main_menu.background.frame.enabled", false));
}

inline bool Draw(h3::H3BaseDlg *dlg)
{
    const auto layout = GetLayout(dlg, Enabled());
    if (!layout.custom)
        return false;
    auto *screen = h3::H3WindowManager::Get()->GetDrawBuffer();
    auto *asset = GetAsset();
    if (!screen || !screen->buffer || !asset || screen == asset)
        return false;

    // Preserve HD's background texture; draw only the image and optional frame
    // before the dialog items, including the API's panel.
    if (layout.assetWidth == asset->width && layout.assetHeight == asset->height)
        asset->DrawToPcx16(layout.assetX, layout.assetY, FALSE, screen);
    else
        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(
            screen, asset, asset->width, asset->height, layout.assetX, layout.assetY,
            layout.assetWidth, layout.assetHeight);
    if (layout.frame)
        screen->SimpleFrameRegion(layout.x - 4, layout.y - 4, layout.width + 8, layout.height + 8);
    return true;
}
} // namespace background
} // namespace mainmenu
