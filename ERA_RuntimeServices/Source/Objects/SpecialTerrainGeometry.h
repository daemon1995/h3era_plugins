#pragma once
#include <array>
#include <cstdint>
#include "ObjectGeometry.h"

namespace erm_functions { namespace geometry {
inline bool PassableUnderlay(int width, int height, bool flat, std::uint64_t passability,
                             std::uint64_t entrances)
{
    constexpr std::uint64_t mask = 0xFFFFFFFFFFFFull;
    return flat && width > 0 && width <= MASK_WIDTH && height > 0 && height <= MASK_HEIGHT &&
           (passability & mask) == mask && !(entrances & mask);
}

// An even-sized footprint has two central cells on that axis. Try both, without
// shifting an odd-sized footprint away from its true center to fit a map edge.
inline int CenteredTerrainOrigins(int x, int y, int width, int height, int mapSize,
                                  std::array<Cell, 4> &origins)
{
    if (mapSize < 1 || x < 0 || y < 0 || x >= mapSize || y >= mapSize ||
        width < 1 || width > MASK_WIDTH || height < 1 || height > MASK_HEIGHT) return 0;
    int count = 0;
    for (int dy = (height - 1) / 2; dy <= height / 2; ++dy)
        for (int dx = (width - 1) / 2; dx <= width / 2; ++dx)
        {
            const int right = x + dx, bottom = y + dy;
            if (right < width - 1 || bottom < height - 1 || right >= mapSize || bottom >= mapSize ||
                right > 255 || bottom > 255) continue;
            origins[count++] = {right, bottom};
        }
    return count;
}

inline bool SmallerTerrain(int width, int height, int otherWidth, int otherHeight)
{
    const int area = width * height, otherArea = otherWidth * otherHeight;
    if (area != otherArea) return area < otherArea;
    const int aspect = std::max(width, height) - std::min(width, height);
    const int otherAspect = std::max(otherWidth, otherHeight) - std::min(otherWidth, otherHeight);
    if (aspect != otherAspect) return aspect < otherAspect;
    return width != otherWidth ? width < otherWidth : height < otherHeight;
}
} }
