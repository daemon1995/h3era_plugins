#pragma once
#include "ObjectLimits.h"
#include <array>

namespace dwellings
{
constexpr int SINGLE_TYPE = 17;
constexpr int MULTIPLE_TYPE = 20;
constexpr int MAX_CAPACITY = object_config::SUBTYPE_COUNT_LIMIT;
using CreatureSlots = std::array<int, 4>;
constexpr CreatureSlots EMPTY_CREATURES{{-1, -1, -1, -1}};

inline bool IsCreature(int creature, int creatureCount) noexcept
{
    return creature >= 0 && creature < creatureCount;
}

} // namespace dwellings
