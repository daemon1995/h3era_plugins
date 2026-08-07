#pragma once

#include "HandlersList.h"

class SecondarySkillHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR NAME = "era.secondarySkills.%d.name";
        static constexpr LPCSTR DESCRIPTION = "era.secondarySkills.%d.description.%d";
    };

    static void Init()
    {
        bool readSuccess = false;
        LPCSTR readResult = nullptr;

        for (int i = 0; i < h3::limits::SECONDARY_SKILLS; i++)
        {
            auto &skill = P_SecondarySkillInfo[i];

            libc::sprintf(h3_TextBuffer, formats::NAME, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                skill.name = readResult;

            for (int level = 0; level < 3; level++)
            {
                libc::sprintf(h3_TextBuffer, formats::DESCRIPTION, i, level);
                readResult = EraJS::read(h3_TextBuffer, readSuccess);
                if (readSuccess)
                    skill.description[level] = readResult;
            }
        }
    }
};
