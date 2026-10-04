#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace editor
{
struct TypeGenerationLimits
{
    int map;
    int zone;
};

inline TypeGenerationLimits NormalizeTypeLimits(int map, int zone) noexcept
{
    map = std::max(0, map);
    return {map, std::max(0, std::min(zone, map))};
}

// The game resets these tables in the RMG constructor. Apply after that reset,
// and restore the exact previous values when generation finishes.
template <std::size_t Count>
class NativeTypeLimitsOverride
{
    std::array<int, Count> previousMap{};
    std::array<int, Count> previousZone{};
    int *nativeMap = nullptr;
    int *nativeZone = nullptr;

  public:
    NativeTypeLimitsOverride() = default;
    NativeTypeLimitsOverride(const NativeTypeLimitsOverride &) = delete;
    NativeTypeLimitsOverride &operator=(const NativeTypeLimitsOverride &) = delete;
    ~NativeTypeLimitsOverride() { Restore(); }

    void Apply(int *mapTable, int *zoneTable, const int *mapLimits, const int *zoneLimits) noexcept
    {
        Restore();
        std::copy_n(mapTable, Count, previousMap.begin());
        std::copy_n(zoneTable, Count, previousZone.begin());
        nativeMap = mapTable;
        nativeZone = zoneTable;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const auto limits = NormalizeTypeLimits(mapLimits[i], zoneLimits[i]);
            nativeMap[i] = limits.map;
            nativeZone[i] = limits.zone;
        }
    }

    void Restore() noexcept
    {
        if (nativeMap)
        {
            std::copy(previousMap.begin(), previousMap.end(), nativeMap);
            std::copy(previousZone.begin(), previousZone.end(), nativeZone);
            nativeMap = nullptr;
            nativeZone = nullptr;
        }
    }
};
} // namespace editor
