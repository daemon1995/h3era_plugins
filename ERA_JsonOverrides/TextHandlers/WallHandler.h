#pragma once

#include "HandlersList.h"

class WallHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR NAME = "era.walls.%d.name";
        static constexpr LPCSTR HIT_POINTS = "era.walls.%d.hp";
    };

    static void Init()
    {
        bool readSuccess = false;
        const auto walls = H3TownFortifications::Get()->fortifications;

        for (int i = 0; i < h3::limits::FORT_ELEMENTS; i++)
        {
            libc::sprintf(h3_TextBuffer, formats::NAME, i);
            const int name = EraJS::readInt(h3_TextBuffer, readSuccess);
            if (readSuccess)
                walls[i].name = name;

            libc::sprintf(h3_TextBuffer, formats::HIT_POINTS, i);
            const int hitPoints = EraJS::readInt(h3_TextBuffer, readSuccess);
            if (readSuccess)
                walls[i].hp = static_cast<INT16>(hitPoints);
        }
    }
};
