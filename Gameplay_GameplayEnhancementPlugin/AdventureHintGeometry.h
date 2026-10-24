#pragma once
#include <algorithm>
#include <cstdint>

namespace advMapHints
{
namespace geometry
{
// Weight known tiles by their visible area. DEF animation never enters the calculation.
template <class KnownTile>
bool VisibleTileCentre(int width, int height, int left, int top, const RECT &view, KnownTile known, POINT &centre)
{
    std::int64_t sumX = 0, sumY = 0, count = 0;
    for (int tileY = 0; tileY < height; ++tileY)
        for (int tileX = 0; tileX < width; ++tileX)
        {
            const int x0 = (std::max)(tileX * 32, int(view.left) - left);
            const int y0 = (std::max)(tileY * 32, int(view.top) - top);
            const int x1 = (std::min)(tileX * 32 + 32, int(view.right) - left);
            const int y1 = (std::min)(tileY * 32 + 32, int(view.bottom) - top);
            if (x0 >= x1 || y0 >= y1 || !known(tileX, tileY))
                continue;
            const std::int64_t area = (x1 - x0) * (y1 - y0);
            sumX += area * (2 * left + x0 + x1 - 1);
            sumY += area * (2 * top + y0 + y1 - 1);
            count += area;
        }
    if (!count)
        return false;
    centre.x = static_cast<LONG>(sumX / (2 * count));
    centre.y = static_cast<LONG>(sumY / (2 * count));
    return true;
}
} // namespace geometry
} // namespace advMapHints
