#include "framework.h"
#include <array>
#include <vector>

namespace resized
{
double BiCubicKernel(double x);
}

namespace mainmenu
{
namespace background
{
namespace
{
struct SamplingPoint
{
    int source[4];
    float weight[4];
};

std::vector<SamplingPoint> MakeSamplingPoints(int sourceSize, int targetSize, int bitMode)
{
    std::vector<SamplingPoint> points(targetSize);
    const double factor = static_cast<double>(sourceSize) / targetSize;
    for (int i = 0; i < targetSize; ++i)
    {
        // Retain the existing bicubic sampler's position convention.
        const double position = i * factor + (bitMode == 4 ? 0.5 : -0.5);
        const int center = static_cast<int>(position);
        const double fraction = position - center;
        for (int n = 0; n < 4; ++n)
        {
            points[i].source[n] = Clamp(0, center + n - 1, sourceSize - 1);
            points[i].weight[n] = static_cast<float>(resized::BiCubicKernel(n - 1 - fraction));
        }
    }
    return points;
}

void ResizeBackground(h3::H3LoadedPcx16 *source, h3::H3LoadedPcx16 *target, int bitMode)
{
    const auto horizontal = MakeSamplingPoints(source->width, target->width, bitMode);
    const auto vertical = MakeSamplingPoints(source->height, target->height, bitMode);
    // Four adjacent source rows suffice for a separable 4x4 bicubic kernel.
    std::array<std::vector<float>, 4> rows;
    int rowIndex[4] = {-1, -1, -1, -1};
    for (auto &row : rows)
        row.resize(target->width * 3);
    for (int y = 0; y < target->height; ++y)
    {
        const auto &yp = vertical[y];
        for (int n = 0; n < 4; ++n)
        {
            const int sourceY = yp.source[n];
            const int slot = sourceY % 4;
            if (rowIndex[slot] == sourceY)
                continue;
            rowIndex[slot] = sourceY;
            auto &row = rows[slot];
            const auto *line = source->buffer + sourceY * source->scanlineSize;
            for (int x = 0; x < target->width; ++x)
            {
                const auto &xp = horizontal[x];
                float b = 0, g = 0, r = 0;
                for (int m = 0; m < 4; ++m)
                {
                    if (bitMode == 4)
                    {
                        const auto *pixel = line + xp.source[m] * 4;
                        b += pixel[0] * xp.weight[m];
                        g += pixel[1] * xp.weight[m];
                        r += pixel[2] * xp.weight[m];
                    }
                    else
                    {
                        h3::H3RGB565 pixel;
                        pixel = *reinterpret_cast<const unsigned short *>(line + xp.source[m] * 2);
                        const auto rgb = pixel.GetRGB888();
                        b += (rgb & 255) * xp.weight[m];
                        g += ((rgb >> 8) & 255) * xp.weight[m];
                        r += ((rgb >> 16) & 255) * xp.weight[m];
                    }
                }
                row[x * 3] = b;
                row[x * 3 + 1] = g;
                row[x * 3 + 2] = r;
            }
        }
        auto *line = target->buffer + y * target->scanlineSize;
        for (int x = 0; x < target->width; ++x)
        {
            unsigned char channels[3];
            for (int c = 0; c < 3; ++c)
            {
                float value = 0;
                for (int n = 0; n < 4; ++n)
                    value += rows[yp.source[n] % 4][x * 3 + c] * yp.weight[n];
                channels[c] = static_cast<unsigned char>(Clamp(0, static_cast<int>(value + 0.5f), 255));
            }
            if (bitMode == 4)
            {
                auto *pixel = line + x * 4;
                pixel[0] = channels[0];
                pixel[1] = channels[1];
                pixel[2] = channels[2];
                pixel[3] = 255;
            }
            else
            {
                h3::H3ARGB888 rgb;
                rgb.b = channels[0];
                rgb.g = channels[1];
                rgb.r = channels[2];
                h3::H3RGB565 pixel;
                pixel = rgb;
                *reinterpret_cast<unsigned short *>(line + x * 2) = pixel;
            }
        }
    }
}

struct ResizedBackground
{
    h3::H3LoadedPcx16 *source = nullptr;
    h3::H3LoadedPcx16 *image = nullptr;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int bitMode = 0;
} cache;
}

void ReleaseCache()
{
    if (cache.image)
        cache.image->Destroy();
    cache = {};
}

bool Draw(h3::H3BaseDlg *dlg)
{
    const auto layout = GetLayout(dlg, Enabled());
    if (!layout.custom)
        return false;
    auto *screen = h3::H3WindowManager::Get()->GetDrawBuffer();
    auto *asset = GetAsset();
    if (!screen || !screen->buffer || !asset || screen == asset)
        return false;

    h3::H3LoadedPcx16 *image = asset;
    if (layout.assetWidth != asset->width || layout.assetHeight != asset->height)
    {
        const int bitMode = h3::H3BitMode::Get();
        if (!cache.image || cache.source != asset || cache.sourceWidth != asset->width ||
            cache.sourceHeight != asset->height || cache.bitMode != bitMode ||
            cache.image->width != layout.assetWidth || cache.image->height != layout.assetHeight)
        {
            ReleaseCache();
            // PNG-backed PCX resources can be empty stubs. Render through ERA
            // to materialize the source before reading its pixels for resizing.
            auto *source = h3::H3LoadedPcx16::Create(asset->width, asset->height);
            auto *fitted = h3::H3LoadedPcx16::Create(layout.assetWidth, layout.assetHeight);
            if (!source || !fitted)
            {
                if (source)
                    source->Destroy();
                if (fitted)
                    fitted->Destroy();
                return false;
            }
            libc::memset(source->buffer, 0, source->buffSize);
            libc::memset(fitted->buffer, 0, fitted->buffSize);
            LARGE_INTEGER frequency, begin, rendered, resizedAt;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&begin);
            asset->DrawToPcx16(0, 0, FALSE, source);
            QueryPerformanceCounter(&rendered);
            ResizeBackground(source, fitted, bitMode);
            QueryPerformanceCounter(&resizedAt);
            source->Destroy();
            Era::WriteLog("Interface_MainMenuAPI", "Background cache prepared",
                          H3String::Format("%dx%d -> %dx%d: PNG/PCX draw=%.2f ms, bicubic=%.2f ms",
                                           asset->width, asset->height, fitted->width, fitted->height,
                                           1000.0 * (rendered.QuadPart - begin.QuadPart) / frequency.QuadPart,
                                           1000.0 * (resizedAt.QuadPart - rendered.QuadPart) / frequency.QuadPart).String());
            cache.source = asset;
            cache.sourceWidth = asset->width;
            cache.sourceHeight = asset->height;
            cache.bitMode = bitMode;
            cache.image = fitted;
        }
        image = cache.image;
    }
    image->DrawToPcx16(layout.assetX, layout.assetY, FALSE, screen);
    if (layout.frame)
        screen->SimpleFrameRegion(layout.x - 4, layout.y - 4, layout.width + 8, layout.height + 8);
    return true;
}
} // namespace background
} // namespace mainmenu
