#pragma once

#include <array>
#include <cstdint>

namespace townupgrades
{
using Cost = std::array<int, 7>;

inline bool ValidSelection(int armyMask, int slot) noexcept
{
    return armyMask >= 1 && armyMask <= 3 && slot >= -1 && slot < 7 &&
           (slot == -1 || armyMask != 3);
}

struct StackRules
{
    int type;
    int upgrade;
    int count;
    int level;
    int faction;
    bool dwellingBuilt;
};

inline bool Eligible(const StackRules &stack, int townFaction, bool universal, bool individual) noexcept
{
    return stack.type >= 0 && stack.upgrade >= 0 && stack.upgrade != stack.type && stack.count > 0 &&
           stack.level >= 0 && stack.level < 7 && stack.dwellingBuilt &&
           (universal || stack.faction == townFaction) && (individual || stack.type != 56);
}

inline bool TryPay(Cost &budget, const Cost &cost) noexcept
{
    for (unsigned i = 0; i < cost.size(); ++i)
        if (cost[i] < 0 || budget[i] < cost[i])
            return false;
    for (unsigned i = 0; i < cost.size(); ++i)
        budget[i] -= cost[i];
    return true;
}

inline int UpgradedExperience(int experience) noexcept
{
    return experience > 0 ? static_cast<int>(static_cast<std::int64_t>(experience) * 3 / 4) : 0;
}

struct Upgrade
{
    int army = 0;
    int slot = 0;
    int from = -1;
    int to = -1;
    int count = 0;
    Cost cost{};
};

struct Plan
{
    std::array<Upgrade, 14> stacks{};
    int size = 0;
    Cost cost{};
};

// Garrison first, then visitor; an unaffordable stack does not prevent later upgrades.
template <class Describe> Plan MakePlan(int armyMask, Cost budget, Describe describe)
{
    Plan plan;
    for (int army = 0; army < 2; ++army)
        if (armyMask & (1 << army))
            for (int slot = 0; slot < 7; ++slot)
            {
                Upgrade upgrade;
                upgrade.army = army;
                upgrade.slot = slot;
                if (describe(upgrade) && TryPay(budget, upgrade.cost))
                {
                    plan.stacks[plan.size++] = upgrade;
                    for (unsigned i = 0; i < plan.cost.size(); ++i)
                        plan.cost[i] += upgrade.cost[i];
                }
            }
    return plan;
}
} // namespace townupgrades
