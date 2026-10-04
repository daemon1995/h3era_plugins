#pragma once

#include "MonolithSaveData.h"

#include <cassert>
#include <map>
#include <vector>

namespace monoliths
{
// Keep vector objects in stable map nodes, with direct subtype lookup on hot paths.
// Growing the pointer table cannot invalidate a vector passed to native code.
template <typename Positions> class GroupRegistry
{
    std::map<int, Positions> groups;
    std::vector<Positions *> lookup;

  public:
    GroupRegistry() = default;
    GroupRegistry(const GroupRegistry &) = delete;
    GroupRegistry &operator=(const GroupRegistry &) = delete;

    Positions *Find(int subtype) const noexcept
    {
        const auto index = static_cast<unsigned int>(subtype);
        return index < lookup.size() ? lookup[index] : nullptr;
    }

    Positions &GetOrCreate(int subtype)
    {
        assert(IsExtendedSubtype(subtype));
        const auto index = static_cast<std::size_t>(subtype);
        if (index >= lookup.size())
            lookup.resize(index + 1, nullptr);
        if (!lookup[index])
            lookup[index] = &groups[subtype];
        return *lookup[index];
    }

    const std::map<int, Positions> &Entries() const noexcept
    {
        return groups;
    }

    void Clear() noexcept
    {
        lookup.clear();
        groups.clear();
    }
};
} // namespace monoliths
