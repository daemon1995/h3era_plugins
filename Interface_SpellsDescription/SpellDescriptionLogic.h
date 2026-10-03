#pragma once

#include <cstdint>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace SpellDescriptions
{
namespace Detail
{
// Restore temporary engine/ERM context on every return and C++ exception.
template <class T, std::size_t N>
class ScopedValues
{
    std::array<T *, N> values;
    std::array<T, N> saved;

public:
    explicit ScopedValues(std::array<T *, N> locations) : values(locations)
    {
        for (std::size_t i = 0; i < N; ++i)
            saved[i] = *values[i];
    }
    ~ScopedValues()
    {
        for (std::size_t i = 0; i < N; ++i)
            *values[i] = saved[i];
    }
    ScopedValues(const ScopedValues &) = delete;
    ScopedValues &operator=(const ScopedValues &) = delete;
};

// Contiguous ERA argument buffers need only one reference, not N pointers.
template <class T, std::size_t N>
class ScopedArray
{
    T (&values)[N];
    std::array<T, N> saved;

public:
    explicit ScopedArray(T (&locations)[N]) : values(locations)
    {
        for (std::size_t i = 0; i < N; ++i)
            saved[i] = values[i];
    }
    ~ScopedArray()
    {
        for (std::size_t i = 0; i < N; ++i)
            values[i] = saved[i];
    }
    ScopedArray(const ScopedArray &) = delete;
    ScopedArray &operator=(const ScopedArray &) = delete;
};

enum class Kind
{
    None = 0,
    Damage = 1,
    Recovery = 2,
    Cure = 3,
    Sacrifice = 4,
    Hypnotize = 5,
    Summon = 6,
    Teleport = 7,
    RemoveObstacle = 8,
    LandMine = 9,
    FireWall = 10,
    Duration = 11
};

// Only mechanics that cannot be distinguished using the native flags.
enum class Identity
{
    Generic,
    Sacrifice,
    Teleport,
    LandMine,
    FireWall,
    DisruptingRay,
    Clone
};

enum class DamageShape { Single, Area, Global, Chain };

inline DamageShape GetDamageShape(std::uint32_t flags)
{
    if (flags & 0x10) // singleTarget
        return flags & 0x10000 ? DamageShape::Chain : DamageShape::Single;
    return flags & 0x80 ? DamageShape::Area : DamageShape::Global;
}

struct ChainCandidate
{
    int x;
    int y;
    bool eligible;
};

// Native 0x5A6500 truncates pixel distance BEFORE comparing it and keeps
// the first stack in side/index order on a tie, even across opposing sides.
inline int NextChainTarget(int x, int y, const ChainCandidate *candidates, std::size_t count)
{
    int result = -1;
    int nearest = 999999;
    for (std::size_t i = 0; i < count; ++i)
    {
        if (!candidates[i].eligible)
            continue;
        const double dx = double(candidates[i].x) - x;
        const double dy = double(candidates[i].y) - y;
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance < nearest)
        {
            // Comparing before conversion keeps extreme mod coordinates safe.
            // For nonnegative distances this preserves native truncation/ties.
            nearest = static_cast<int>(distance);
            result = static_cast<int>(i);
        }
    }
    return result;
}

struct DamageTotals
{
    std::uintptr_t targets[42] = {};
    std::size_t count = 0;
    std::int64_t damage = 0;
    std::int64_t killed = 0;

    bool Contains(std::uintptr_t target) const
    {
        for (std::size_t i = 0; i < count; ++i)
            if (targets[i] == target)
                return true;
        return false;
    }

    bool Add(std::uintptr_t target, int amount, int deaths)
    {
        if (!target || count >= 42 || Contains(target))
            return false;
        return AddUnique(target, amount, deaths);
    }

    // Caller already checked Contains before invoking the target's MR hooks.
    bool AddUnique(std::uintptr_t target, int amount, int deaths)
    {
        if (!target || count >= 42)
            return false;
        targets[count++] = target;
        damage += amount > 0 ? amount : 0;
        killed += deaths > 0 ? deaths : 0;
        return true;
    }
};

// Preserve a positive hit chance below 1%; truncating to zero would treat a
// resistant stack as immune and also alter the route of Chain Lightning.
inline int ChancePercent(double chance)
{
    if (!(chance > 0.0))
        return 0;
    if (chance >= 1.0)
        return 100;
    const int percent = static_cast<int>(chance * 100.0);
    return percent > 0 ? percent : 1;
}

// H3Spell's current flags, never a cached classification by spell ID.
inline Kind Classify(Identity identity, std::uint32_t flags, int targetType, int spEffect)
{
    constexpr std::uint32_t TIME_SCALE = 0x4;
    constexpr std::uint32_t SINGLE_TARGET = 0x10;
    constexpr std::uint32_t TARGET_ANYWHERE = 0x80;
    constexpr std::uint32_t REMOVE_OBSTACLE = 0x100;
    constexpr std::uint32_t DAMAGE = 0x200;
    constexpr std::uint32_t MIND = 0x400;
    constexpr std::uint32_t FRIENDLY_MASS = 0x800;
    constexpr std::uint32_t AI_CREATURES = 0x80000;

    if (flags & DAMAGE)
        return Kind::Damage;
    if (flags & REMOVE_OBSTACLE)
        return Kind::RemoveObstacle;
    if (flags & AI_CREATURES)
    {
        if ((flags & MIND) && targetType == -1)
            return Kind::Hypnotize;
        if ((flags & SINGLE_TARGET) && targetType == 1 && !(flags & TIME_SCALE))
            return Kind::Recovery;
        if (targetType == 0 && !(flags & (SINGLE_TARGET | TARGET_ANYWHERE | TIME_SCALE)))
            return Kind::Summon;
    }
    if (targetType == 1 && !(flags & TIME_SCALE))
    {
        if (flags & FRIENDLY_MASS)
            return Kind::Cure;
        // No distinct H3 flags exist for these two-stage targeting mechanics.
        if ((flags & SINGLE_TARGET) && identity == Identity::Sacrifice)
            return Kind::Sacrifice;
        if ((flags & SINGLE_TARGET) && identity == Identity::Teleport)
            return Kind::Teleport;
    }
    // Vanilla mine/wall spells lack DAMAGE; require their current area flags too.
    if (targetType == 0 && (flags & TARGET_ANYWHERE) && !(flags & TIME_SCALE) && spEffect > 0)
    {
        if (identity == Identity::LandMine)
            return Kind::LandMine;
        if (identity == Identity::FireWall)
            return Kind::FireWall;
    }
    if ((flags & TIME_SCALE) && (flags & 1) && !(flags & 2))
        return Kind::Duration;
    // These native lasting effects lack TIME_SCALE; geometry alone cannot
    // distinguish them from teleport. Damage/other flags above still win.
    if ((flags & 0x40000) && (flags & SINGLE_TARGET) &&
        (identity == Identity::DisruptingRay || identity == Identity::Clone))
        return Kind::Duration;
    return Kind::None;
}

inline int NonnegativeInt(std::int64_t value)
{
    if (value <= 0)
        return 0;
    const auto maximum = (std::numeric_limits<int>::max)();
    return value > maximum ? maximum : static_cast<int>(value);
}

inline std::int64_t SacrificeHealth(int creatureHitPoints, int power, int masteryBase, int count)
{
    const auto perCreature = std::int64_t(creatureHitPoints) + power + masteryBase;
    if (perCreature <= 0 || count <= 0)
        return 0;
    const auto maximum = (std::numeric_limits<std::int64_t>::max)();
    return perCreature > maximum / count ? maximum : perCreature * count;
}

inline int Killed(int alive, int hitPoints, int healthLost, int damage, bool clone = false)
{
    if (alive <= 0 || hitPoints <= 0 || damage <= 0)
        return 0;
    if (clone)
        return alive;
    const auto remaining = std::int64_t(alive) * hitPoints - healthLost - damage;
    if (remaining <= 0)
        return alive;
    return NonnegativeInt(alive - (remaining + hitPoints - 1) / hitPoints);
}

inline int Resurrected(int atStart, int alive, int hitPoints, int healthLost, int restored)
{
    if (hitPoints <= 0 || restored <= 0 || atStart <= alive)
        return 0;
    // A corpse retains the old top creature's healthLost in H3.
    const auto surplus = std::int64_t(restored) - (alive > 0 ? healthLost : 0);
    if (surplus <= 0)
        return 0;
    const auto count = (surplus + hitPoints - 1) / hitPoints;
    return NonnegativeInt(count < atStart - alive ? count : atStart - alive);
}
}
}
