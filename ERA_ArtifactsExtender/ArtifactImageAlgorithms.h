#pragma once
#include <cstddef>
#include <climits>

namespace artifacts
{
template <class Group, class Frame, class Reallocate>
bool GrowArtifactFrames(Group &group, int required, Frame *placeholder, Reallocate reallocate)
{
    if (required < 0 || group.count < 0 || (group.count && !group.frames)) return false;
    if (group.count >= required) return true;
    auto **frames = static_cast<Frame **>(reallocate(group.frames, std::size_t(required) * sizeof(Frame *)));
    if (!frames) return false;
    for (int id = group.count; id < required; ++id) frames[id] = placeholder;
    group.frames = frames;
    group.spritesSize = required;
    group.count = required;
    return true;
}

template <class Pixel, class Palette>
unsigned char ArtifactPaletteIndex(const Pixel &pixel, const Palette &palette)
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
}
