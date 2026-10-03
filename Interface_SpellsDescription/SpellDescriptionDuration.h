#pragma once
#include "SpellDescriptionLogic.h"

namespace SpellDescriptions { namespace Detail {
enum class DurationMode
{
    None = 0, Rounds = 1, Battle = 2, NextAttack = 3, NextTurn = 4,
    RoundsOrAttack = 5, RoundsOrDamage = 6
};
enum class DurationRule { Generic, DisruptingRay, Berserk, Frenzy, Blind, Stone, Paralyze, Clone };
struct Duration { DurationMode mode = DurationMode::None; int rounds = 0; };
inline bool NumericDuration(DurationMode mode)
{
    return mode == DurationMode::Rounds || mode == DurationMode::RoundsOrAttack ||
           mode == DurationMode::RoundsOrDamage;
}
inline bool ValidDuration(int mode, int rounds)
{
    return mode >= int(DurationMode::None) && mode <= int(DurationMode::RoundsOrDamage) &&
           (!NumericDuration(static_cast<DurationMode>(mode)) || rounds > 0);
}
inline Duration NativeDuration(DurationRule rule, std::uint32_t flags, int power, int bonus)
{
    // ApplySpell 0x444610 uses special expiry rules for ray/berserk/frenzy.
    // CastSpell 0x5A0140 adds Hero_GetSpellDurationBonus only for TIME_SCALE;
    // SummonClone 0x5A6F80 adds it independently of that flag.
    if (rule == DurationRule::DisruptingRay) return {DurationMode::Battle, 0};
    if (rule == DurationRule::Berserk) return {DurationMode::NextAttack, 0};
    if (rule == DurationRule::Frenzy) return {DurationMode::NextTurn, 0};
    if (!(flags & 4) && rule != DurationRule::Clone) return {};
    const int rounds = NonnegativeInt(std::int64_t(power) + bonus);
    if (rounds <= 0) return {};
    const auto mode = rule == DurationRule::Clone ? DurationMode::RoundsOrDamage :
                      rule == DurationRule::Blind || rule == DurationRule::Stone || rule == DurationRule::Paralyze
                          ? DurationMode::RoundsOrAttack : DurationMode::Rounds;
    return {mode, rounds};
}
} }
