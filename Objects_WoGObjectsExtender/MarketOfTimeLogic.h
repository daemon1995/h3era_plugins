#pragma once
#include <array>

namespace wog
{
namespace marketOfTime
{
constexpr int OBJECT_SUBTYPE = 51;
constexpr int WOG_OPTION = 109;
constexpr int GOLD_COST = 2000;
constexpr int MOVEMENT_COST = 500;
constexpr int SKILL_COUNT = 28;
constexpr int PAGE_SIZE = 8;

struct SkillList
{
    std::array<int, SKILL_COUNT> ids{};
    int count = 0;

    int SkillOnPage(int first, int choice) const noexcept
    {
        return first >= 0 && first < count && choice >= 0 && choice < PAGE_SIZE && choice < count - first
                   ? ids[first + choice]
                   : -1;
    }
};

template <typename SkillLevels>
SkillList CollectSkills(const SkillLevels &levels, bool warfare, int warfareSkill) noexcept
{
    // These are the three native IDs used by script 14 for the shared Warfare skill.
    constexpr int BALLISTICS = 10;
    constexpr int ARTILLERY = 20;
    constexpr int FIRST_AID = 27;
    if (warfareSkill != BALLISTICS && warfareSkill != FIRST_AID)
        warfareSkill = ARTILLERY;

    SkillList result;
    for (int skill = 0; skill < SKILL_COUNT; ++skill)
    {
        if (levels[skill] <= 0)
            continue;
        if (warfare && (skill == BALLISTICS || skill == ARTILLERY || skill == FIRST_AID) && skill != warfareSkill)
            continue;
        result.ids[result.count++] = skill;
    }
    return result;
}
} // namespace marketOfTime
} // namespace wog
