#pragma once
#include "ObjectGeometry.h"
#include <map>
#include <tuple>

namespace erm_functions { namespace geometry {
// Immutable shapes have stable addresses until Clear(). Failed builds are cached too.
class ShapeCache
{
    using Key = std::tuple<int, int, std::uint64_t, std::uint64_t, std::uint64_t>;
    struct Entry { Shape shape; bool valid = false; };
    std::map<Key, Entry> entries;
public:
    const Shape *Get(int width, int height, std::uint64_t passability,
                     std::uint64_t entrances, std::uint64_t colors)
    {
        constexpr std::uint64_t mask = 0xFFFFFFFFFFFFull;
        const Key key(width, height, passability & mask, entrances & mask, colors & mask);
        auto found = entries.lower_bound(key);
        if (found == entries.end() || entries.key_comp()(key, found->first))
        {
            Entry entry;
            entry.valid = BuildShape(width, height, passability, entrances, colors, entry.shape);
            found = entries.emplace_hint(found, key, std::move(entry));
        }
        return found->second.valid ? &found->second.shape : nullptr;
    }
    void Clear() { entries.clear(); }
    std::size_t Size() const { return entries.size(); }
};
} }
