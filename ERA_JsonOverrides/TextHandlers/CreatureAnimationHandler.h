#pragma once

#include "HandlersList.h"

class CreatureAnimationHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR MISSILE_FRAME_ANGLES = "era.monsters.%d.animation.missileFrameAngles.%d";
        static constexpr LPCSTR TROOP_COUNT_LOCATION_OFFSET = "era.monsters.%d.animation.troopCountLocationOffset";
        static constexpr LPCSTR ATTACK_CLIMAX_FRAME = "era.monsters.%d.animation.attackClimaxFrame";
        static constexpr LPCSTR TIME_BETWEEN_FIDGETS = "era.monsters.%d.animation.timeBetweenFidgets";
        static constexpr LPCSTR WALK_ANIMATION_TIME = "era.monsters.%d.animation.walkAnimationTime";
        static constexpr LPCSTR ATTACK_ANIMATION_TIME = "era.monsters.%d.animation.attackAnimationTime";
        static constexpr LPCSTR FLIGHT_ANIMATION_TIME = "era.monsters.%d.animation.flightAnimationTime";
        static constexpr LPCSTR troopCountLocationOffset = TROOP_COUNT_LOCATION_OFFSET;
        static constexpr LPCSTR attackClimaxFrame = ATTACK_CLIMAX_FRAME;
        static constexpr LPCSTR timeBetweenFidgets = TIME_BETWEEN_FIDGETS;
        static constexpr LPCSTR walkAnimationTime = WALK_ANIMATION_TIME;
        static constexpr LPCSTR attackAnimationTime = ATTACK_ANIMATION_TIME;
        static constexpr LPCSTR flightAnimationTime = FLIGHT_ANIMATION_TIME;
    };

    static void Init()
    {
        bool readSuccess = false;
        const int creatureCount = IntAt(0x4A1657);

        for (int i = 0; i < creatureCount; i++)
        {
            auto &animation = H3CreatureAnimation::Get()[i];

            for (int angle = 0; angle < 12; angle++)
            {
                libc::sprintf(h3_TextBuffer, formats::MISSILE_FRAME_ANGLES, i, angle);
                const int value = EraJS::readInt(h3_TextBuffer, readSuccess);
                if (readSuccess)
                    animation.missileFrameAngles[angle] = value;
            }

#define READ_INT(field)                                                                                               \
    libc::sprintf(h3_TextBuffer, formats::field, i);                                                                  \
    {                                                                                                                  \
        const int value = EraJS::readInt(h3_TextBuffer, readSuccess);                                                  \
        if (readSuccess)                                                                                              \
            animation.field = value;                                                                                  \
    }
            READ_INT(troopCountLocationOffset)
            READ_INT(attackClimaxFrame)
            READ_INT(timeBetweenFidgets)
            READ_INT(walkAnimationTime)
            READ_INT(attackAnimationTime)
            READ_INT(flightAnimationTime)
#undef READ_INT
        }
    }
};
