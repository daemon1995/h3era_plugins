#pragma once
#include <array>

namespace dwellings
{
constexpr int SINGLE_TYPE = 17;
constexpr int MULTIPLE_TYPE = 20;
constexpr int MIN_CAPACITY = 1024;
constexpr int MAX_CAPACITY = 65536;
using CreatureSlots = std::array<int, 4>;
constexpr CreatureSlots EMPTY_CREATURES{{-1, -1, -1, -1}};

inline bool IsCreature(int creature, int creatureCount) noexcept
{
    return creature >= 0 && creature < creatureCount;
}

} // namespace dwellings
