#pragma once
#include "../headers/Objects/DwellingConfig.h"
#include <cstdint>

namespace dwellings
{
struct SavedDwelling
{
    std::uint32_t position;
    CreatureSlots creatures;
};
static_assert(sizeof(SavedDwelling) == 20, "Dwelling saves require four 32-bit creature IDs.");

inline bool HasCreatures(const CreatureSlots &creatures, int slotCount, int creatureCount) noexcept
{
    for (int slot = 0; slot < slotCount; ++slot)
        if (IsCreature(creatures[slot], creatureCount))
            return true;
    return false;
}

inline bool IsValidSavedDwelling(const SavedDwelling &record, std::uint32_t position, int creatureCount) noexcept
{
    if (record.position != position)
        return false;
    for (const int creature : record.creatures)
        if (creature != -1 && !IsCreature(creature, creatureCount))
            return false;
    return true;
}

// Missing/invalid fields preserve the original slot; -1 explicitly removes it.
template <typename Reader>
void ReadCreatures(CreatureSlots &creatures, int slotCount, int creatureCount, Reader read)
{
    for (int slot = 0; slot < slotCount; ++slot)
    {
        int creature = -1;
        if (read(slot, creature) && (creature == -1 || IsCreature(creature, creatureCount)))
            creatures[slot] = creature;
    }
}
} // namespace dwellings
