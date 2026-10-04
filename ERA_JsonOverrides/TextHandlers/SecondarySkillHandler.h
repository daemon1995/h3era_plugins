#pragma once
#include "HandlersList.h"
class SecondarySkillHandler
{
  public:
    static void Init()
    {
        for (int i = 0; i < h3::limits::SECONDARY_SKILLS; ++i)
        {
            auto &skill = P_SecondarySkillInfo[i];
            EraJS::ReadField(skill.name, "era.secondarySkills.%d.name", i);
            for (int level = 0; level < 3; ++level)
                EraJS::ReadFormatted(skill.description[level], "era.secondarySkills.%d.description.%d", i, level);
        }
    }
};
