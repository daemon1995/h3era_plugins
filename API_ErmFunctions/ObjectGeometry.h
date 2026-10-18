#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
#include <utility>

namespace erm_functions
{
namespace geometry
{
constexpr int MASK_WIDTH = 8;
constexpr int MASK_HEIGHT = 6;

struct Cell
{
    int x, y;
    bool operator==(const Cell &other) const { return x == other.x && y == other.y; }
    bool operator<(const Cell &other) const { return y != other.y ? y < other.y : x < other.x; }
};

// Native H3ObjectAttributes stores the bottom-right bit at position 47, not 0.
// Verified against GetEntrance (4FF280) and WoG FindEnterShift (712CEB).
inline bool NativeMaskCell(std::uint64_t bits, int column, int row)
{
    return column >= 0 && column < MASK_WIDTH && row >= 0 && row < MASK_HEIGHT &&
           ((bits >> (47 - row * MASK_WIDTH - column)) & 1) != 0;
}

struct Shape
{
    int width = 0, height = 0;
    int entranceColumn = -1, entranceRow = -1;
    std::vector<Cell> blocked, entrances, visible;
};

inline bool BuildShape(int width, int height, std::uint64_t passability,
                       std::uint64_t entrances, std::uint64_t colors, Shape &result)
{
    result = {};
    if (width < 1 || width > MASK_WIDTH || height < 1 || height > MASK_HEIGHT)
        return false;
    result.width = width;
    result.height = height;
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
        {
            if (!NativeMaskCell(entrances, column, row))
                continue;
            if (result.entranceColumn < 0)
            {
                result.entranceColumn = column;
                result.entranceRow = row;
            }
        }
    if (result.entranceColumn < 0)
        return false; // This API replaces visitable objects with an unambiguous primary entrance.
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
        {
            const Cell cell{result.entranceColumn - column, result.entranceRow - row};
            if (!NativeMaskCell(passability, column, row)) result.blocked.push_back(cell);
            if (NativeMaskCell(entrances, column, row)) result.entrances.push_back(cell);
            if (NativeMaskCell(colors, column, row)) result.visible.push_back(cell);
        }
    std::sort(result.blocked.begin(), result.blocked.end());
    std::sort(result.entrances.begin(), result.entrances.end());
    return true;
}

inline bool SameTopology(const Shape &source, const Shape &target)
{
    return &source == &target ||
           (source.blocked == target.blocked && source.entrances == target.entrances);
}

struct Placement
{
    int originX = 0, originY = 0;
};

// Keep the selected prototype visible to the native selector, without changing its masks.
// Collect before mutating: allocation failure cannot leave a modified catalog.
// Typical catalogs need no allocation; large variant sets spill into the vector.
template <class List>
class ScopedPrototypeSelection
{
    List &list;
    using Entry = std::pair<int, int>;
    std::array<Entry, 16> local;
    std::size_t localCount = 0;
    std::vector<Entry> overflow;
public:
    ScopedPrototypeSelection(List &attributes, int index) : list(attributes)
    {
        const int subtype = list[index].subtype;
        for (unsigned i = 0; i < list.Count(); ++i)
            if (static_cast<int>(i) != index && list[i].subtype == subtype)
            {
                if (localCount < local.size()) local[localCount++] = Entry(i, subtype);
                else overflow.emplace_back(i, subtype);
            }
        for (std::size_t i = 0; i < localCount; ++i) list[local[i].first].subtype = -1;
        for (const auto &entry : overflow) list[entry.first].subtype = -1;
    }
    ~ScopedPrototypeSelection()
    {
        for (std::size_t i = 0; i < localCount; ++i) list[local[i].first].subtype = local[i].second;
        for (const auto &entry : overflow) list[entry.first].subtype = entry.second;
    }
    ScopedPrototypeSelection(const ScopedPrototypeSelection &) = delete;
    ScopedPrototypeSelection &operator=(const ScopedPrototypeSelection &) = delete;
};

// World supplies bounds, foreign entrances and valid approach cells. It never changes the map.
template <class World>
bool CheckPlacement(const Shape &source, const Shape &target, int x, int y,
                    unsigned approachMask, const World &world, Placement &placement)
{
    if (!SameTopology(source, target) || !approachMask || approachMask > 255)
        return false;
    placement.originX = x + target.entranceColumn;
    placement.originY = y + target.entranceRow;
    if (!world.InBounds(placement.originX, placement.originY)) return false;
    for (const auto &cell : target.blocked)
        if (!world.InBounds(x + cell.x, y + cell.y)) return false;
    for (const auto &cell : target.entrances)
        if (!world.InBounds(x + cell.x, y + cell.y)) return false;
    for (const auto &cell : target.visible)
        if (!world.InBounds(x + cell.x, y + cell.y) ||
            world.ForeignEntrance(x + cell.x, y + cell.y)) return false;
    // Clockwise: N, NE, E, SE, S, SW, W, NW. Preserve the actual approach to the entrance.
    static const Cell approaches[] = {{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
    for (unsigned index = 0; index < 8; ++index)
        if ((approachMask & (1u << index)) &&
            world.CanApproach(x + approaches[index].x, y + approaches[index].y)) return true;
    return false;
}
}
}
