#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace SpellDescriptions
{
namespace Detail
{
struct PixelRect
{
    int x = 0, y = 0, width = 0, height = 0;
    bool Empty() const { return width <= 0 || height <= 0; }
    bool Intersects(const PixelRect &other) const
    {
        return !Empty() && !other.Empty() && std::int64_t(x) < std::int64_t(other.x) + other.width &&
               std::int64_t(other.x) < std::int64_t(x) + width &&
               std::int64_t(y) < std::int64_t(other.y) + other.height &&
               std::int64_t(other.y) < std::int64_t(y) + height;
    }
};

inline PixelRect ClipPixels(int x, int y, int width, int height, int canvasWidth, int canvasHeight)
{
    if (width <= 0 || height <= 0 || canvasWidth <= 0 || canvasHeight <= 0)
        return {};
    const auto right = std::int64_t(x) + width;
    const auto bottom = std::int64_t(y) + height;
    const int left = x > 0 ? x : 0;
    const int top = y > 0 ? y : 0;
    const int clippedRight = right < canvasWidth ? static_cast<int>(right) : canvasWidth;
    const int clippedBottom = bottom < canvasHeight ? static_cast<int>(bottom) : canvasHeight;
    if (left >= clippedRight || top >= clippedBottom)
        return {};
    return {left, top, clippedRight - left, clippedBottom - top};
}

// At most two 45x52 tiles per target. Capture every rectangle BEFORE painting,
// so overlapping hexes restore the same original pixels in either order.
class PixelBackup
{
    struct Patch
    {
        PixelRect rect;
        std::size_t offset;
    };
    std::array<Patch, 84> patches = {};
    std::vector<std::uint8_t> pixels;
    std::vector<std::uint8_t> paintedPixels;
    std::size_t count = 0;
    int bytesPerPixel = 0;
    bool painted = false;

    template <class Pixel>
    static void RestoreRow(std::uint8_t *destination, const std::uint8_t *original,
                           const std::uint8_t *overlay, int width)
    {
        const auto rowSize = std::size_t(width) * sizeof(Pixel);
        if (std::memcmp(destination, overlay, rowSize) == 0)
        {
            std::memcpy(destination, original, rowSize);
            return;
        }
        // Constant-size copies compile to unaligned loads/stores without
        // aliasing violations or a CRT call for every individual pixel.
        for (int column = 0; column < width; ++column)
        {
            const auto offset = std::size_t(column) * sizeof(Pixel);
            Pixel current, expected;
            std::memcpy(&current, destination + offset, sizeof(Pixel));
            std::memcpy(&expected, overlay + offset, sizeof(Pixel));
            if (current == expected)
                std::memcpy(destination + offset, original + offset, sizeof(Pixel));
        }
    }

  public:
    // Retain buffers while hovering, release them at battle/game boundaries.
    void Release() noexcept
    {
        count = 0;
        bytesPerPixel = 0;
        painted = false;
        std::vector<std::uint8_t>().swap(pixels);
        std::vector<std::uint8_t>().swap(paintedPixels);
    }

    bool Capture(const std::uint8_t *buffer, int width, int height, int stride, int pixelSize,
                 const PixelRect *rects, std::size_t rectCount)
    {
        count = 0;
        painted = false;
        if (!buffer || !rects || width <= 0 || height <= 0 || (pixelSize != 2 && pixelSize != 4) ||
            std::int64_t(stride) < std::int64_t(width) * pixelSize || rectCount > patches.size())
            return false;
        std::size_t size = 0;
        for (std::size_t i = 0; i < rectCount; ++i)
        {
            const auto &rect = rects[i];
            if (rect.Empty() || rect.x < 0 || rect.y < 0 || rect.width > 45 || rect.height > 52 ||
                std::int64_t(rect.x) + rect.width > width || std::int64_t(rect.y) + rect.height > height)
                return false;
            patches[i] = {rect, size};
            size += std::size_t(rect.width) * rect.height * pixelSize;
        }
        pixels.resize(size); // Reuse capacity; no per-frame allocation once warmed.
        paintedPixels.resize(size); // Allocate before touching the canvas.
        bytesPerPixel = pixelSize;
        for (std::size_t i = 0; i < rectCount; ++i)
        {
            const auto &patch = patches[i];
            const auto rowSize = std::size_t(patch.rect.width) * pixelSize;
            for (int row = 0; row < patch.rect.height; ++row)
                std::memcpy(pixels.data() + patch.offset + row * rowSize,
                            buffer + std::size_t(patch.rect.y + row) * stride + patch.rect.x * pixelSize, rowSize);
        }
        count = rectCount;
        return true;
    }

    // Keep the overlay in the actual screen buffer until native drawing resumes.
    // Both snapshots cover all rectangles, including overlaps, in the same order.
    void RecordPaint(const std::uint8_t *buffer, int stride)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto &patch = patches[i];
            const auto rowSize = std::size_t(patch.rect.width) * bytesPerPixel;
            for (int row = 0; row < patch.rect.height; ++row)
                std::memcpy(paintedPixels.data() + patch.offset + row * rowSize,
                            buffer + std::size_t(patch.rect.y + row) * stride + patch.rect.x * bytesPerPixel, rowSize);
        }
        painted = true;
    }

    void RestoreUnchanged(std::uint8_t *buffer, int stride) const
    {
        if (!painted)
            return;
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto &patch = patches[i];
            const auto rowSize = std::size_t(patch.rect.width) * bytesPerPixel;
            for (int row = 0; row < patch.rect.height; ++row)
            {
                auto destination = buffer + std::size_t(patch.rect.y + row) * stride +
                                   patch.rect.x * bytesPerPixel;
                const auto offset = patch.offset + row * rowSize;
                // Preserve entire native pixels when a sprite/dialog has
                // replaced even one byte since the recorded overlay.
                if (bytesPerPixel == 4)
                    RestoreRow<std::uint32_t>(destination, pixels.data() + offset,
                                              paintedPixels.data() + offset, patch.rect.width);
                else
                    RestoreRow<std::uint16_t>(destination, pixels.data() + offset,
                                              paintedPixels.data() + offset, patch.rect.width);
            }
        }
    }

    void Restore(std::uint8_t *buffer, int stride) const
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto &patch = patches[i];
            const auto rowSize = std::size_t(patch.rect.width) * bytesPerPixel;
            for (int row = 0; row < patch.rect.height; ++row)
                std::memcpy(buffer + std::size_t(patch.rect.y + row) * stride + patch.rect.x * bytesPerPixel,
                            pixels.data() + patch.offset + row * rowSize, rowSize);
        }
    }
};
} // namespace Detail
} // namespace SpellDescriptions
