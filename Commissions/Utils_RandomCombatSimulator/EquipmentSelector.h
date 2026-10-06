#pragma once

#include "CombatParameters.h"

namespace randomcombat
{
// CanPlace sees the hero after each Equip call, so native slot restrictions
// remain authoritative. The pool contains each available artifact ID once.
template <class CanPlace, class Equip>
int EquipUniqueArtifacts(Generator &random, int requested, std::vector<int> pool, CanPlace canPlace, Equip equip)
{
    std::array<int, 14> slots{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 18}};
    int equipped = 0;
    for (int i = 0; i < static_cast<int>(slots.size()) && equipped < requested && !pool.empty(); ++i)
    {
        std::swap(slots[i], slots[random.Between(i, static_cast<int>(slots.size()) - 1)]);
        const int slot = slots[i];
        std::vector<int> candidates;
        for (int index = 0; index < static_cast<int>(pool.size()); ++index)
            if (canPlace(pool[index], slot))
                candidates.push_back(index);
        if (candidates.empty())
            continue;
        const int selected = candidates[random.Between(0, static_cast<int>(candidates.size()) - 1)];
        equip(pool[selected], slot);
        pool.erase(pool.begin() + selected);
        ++equipped;
    }
    return equipped;
}
} // namespace randomcombat
