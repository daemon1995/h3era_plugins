#pragma once
#include "framework.h"

namespace wog { namespace native {
inline bool IsWarfareSkill(int skill) noexcept { return skill == 10 || skill == 20 || skill == 27; }
inline int WarfareRepresentative(const H3Hero &hero) noexcept
{
    int representative = -1;
    for (int skill : {20, 10, 27})
        if (hero.secSkill[skill] > 0)
        {
            if (representative < 0) representative = skill;
            // Warfare's visible native slot is authoritative; w48 is unnecessary.
            if (hero.secSkillPosition[skill] > 0) return skill;
        }
    return representative < 0 ? 20 : representative;
}
inline bool RemoveSecondarySkill(H3Hero &hero, int skill) noexcept
{
    if (skill < 0 || skill >= 28 || hero.secSkill[skill] <= 0) return false;
    const int position = hero.secSkillPosition[skill];
    hero.secSkill[skill] = 0;
    hero.secSkillPosition[skill] = 0;
    if (position > 0)
    {
        for (int other = 0; other < 28; ++other)
            if (hero.secSkillPosition[other] > position) --hero.secSkillPosition[other];
        if (hero.secSkillCount > 0) --hero.secSkillCount;
    }
    return true;
}
inline bool ForgetSkill(H3Hero &hero, int skill, bool warfare) noexcept
{
    if (skill < 0 || skill >= 28 || hero.secSkill[skill] <= 0) return false;
    if (warfare && IsWarfareSkill(skill))
    {
        for (int linked : {10, 20, 27}) RemoveSecondarySkill(hero, linked);
        return true;
    }
    return RemoveSecondarySkill(hero, skill);
}
} }
