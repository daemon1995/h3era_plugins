#pragma once

#include <cstddef>

namespace SpellDescriptions
{
namespace Detail
{
// A spell cursor's frame is animation rather than spell/target identity.
inline bool SameTargetCursor(int type, int frame, int savedType, int savedFrame, bool animated)
{
    return type == savedType && (animated || frame == savedFrame);
}

struct ChainRoute
{
    struct Target
    {
        int hex;
        int secondHex;
        int stack;
    };
    Target targets[42] = {};
    std::size_t count = 0;

    bool Add(int hex, int secondHex, int stack)
    {
        if (count >= 42 || hex < 0 || hex >= 187 || stack < 0 || stack >= 42)
            return false;
        for (std::size_t i = 0; i < count; ++i)
            if (targets[i].stack == stack)
                return false;
        // A two-hex creature occupies adjacent columns in the same row.
        if (secondHex == hex || secondHex < 0 || secondHex >= 187 || secondHex / 17 != hex / 17 ||
            (secondHex != hex - 1 && secondHex != hex + 1))
            secondHex = -1;
        targets[count++] = {hex, secondHex, stack};
        return true;
    }

    bool SameTargets(const ChainRoute &other) const
    {
        if (count != other.count || count > 42)
            return false;
        for (std::size_t i = 0; i < count; ++i)
            if (targets[i].hex != other.targets[i].hex || targets[i].secondHex != other.targets[i].secondHex ||
                targets[i].stack != other.targets[i].stack)
                return false;
        return true;
    }
};

// Native battlefield cell: 45 x 52 pixels, point at the top/bottom.
inline int HexInset(int row)
{
    if (row < 0 || row >= 52)
        return -1;
    const int edge = row < 26 ? row : 51 - row;
    return edge < 13 ? (22 * (13 - edge) + 12) / 13 : 0;
}
} // namespace Detail
} // namespace SpellDescriptions
