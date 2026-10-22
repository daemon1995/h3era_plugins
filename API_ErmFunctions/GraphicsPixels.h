#pragma once
#include <cstdint>
#include <cstddef>
#include <climits>
#include <vector>

namespace graphics_resources
{
struct Pixel { unsigned char b, g, r, a; };

// DEF compression 1: row offsets followed by transparent and literal runs.
inline std::vector<std::uint8_t> EncodeFrame(const std::vector<std::uint8_t> &pixels, int width, int height)
{
    if (width <= 0 || height <= 0 || std::size_t(width) * height != pixels.size()) return {};
    std::vector<std::uint8_t> encoded(std::size_t(height) * 4, 0);
    for (int y = 0; y < height; ++y)
    {
        const std::uint32_t offset = static_cast<std::uint32_t>(encoded.size());
        for (int byte = 0; byte < 4; ++byte)
            encoded[std::size_t(y) * 4 + byte] = static_cast<std::uint8_t>(offset >> (byte * 8));
        const auto row = pixels.begin() + std::size_t(y) * width;
        for (int x = 0; x < width;)
        {
            const bool transparent = row[x] == 0;
            int length = 1;
            while (length < 256 && x + length < width && (row[x + length] == 0) == transparent) ++length;
            encoded.push_back(transparent ? 0 : 255);
            encoded.push_back(static_cast<std::uint8_t>(length - 1));
            if (!transparent) encoded.insert(encoded.end(), row + x, row + x + length);
            x += length;
        }
    }
    return encoded;
}

template <class Palette>
unsigned char PaletteIndex(const Pixel &pixel, const Palette &palette)
{
    if (pixel.a < 128 || (pixel.r < 8 && pixel.g >= 248 && pixel.b >= 248)) return 0;
    int best = 8, distance = INT_MAX;
    for (int index = 8; index < 256; ++index)
    {
        const auto &c = palette.color[index];
        const int r = int(c.r) - pixel.r, g = int(c.g) - pixel.g, b = int(c.b) - pixel.b;
        const int d = r * r + g * g + b * b;
        if (d < distance) { best = index; distance = d; if (!d) break; }
    }
    return static_cast<unsigned char>(best);
}

// Resolve every source before calling this: allocation failure leaves the atlas intact.
template <class Group, class Frame, class Reallocate>
bool UpdateFrameTable(Group &group, const std::vector<Frame *> &frames, Reallocate reallocate)
{
    if (group.count < 0 || (group.count && !group.frames) || frames.size() < std::size_t(group.count)) return false;
    for (auto *frame : frames) if (!frame) return false;
    if (frames.empty()) return true;
    auto **buffer = static_cast<Frame **>(reallocate(group.frames, frames.size() * sizeof(Frame *)));
    if (!buffer) return false;
    for (std::size_t i = 0; i < frames.size(); ++i) buffer[i] = frames[i];
    group.frames = buffer;
    group.count = static_cast<int>(frames.size());
    group.spritesSize = static_cast<unsigned>(frames.size());
    return true;
}
} // namespace graphics_resources
