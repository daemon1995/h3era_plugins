#pragma once
#include "HandlersList.h"
class HeroHandler
{

    struct formats
    {
        static constexpr LPCSTR NAME = "era.heroes.%d.name";
        static constexpr LPCSTR SPECIALTY_SHORT = "era.heroes.%d.specialty.short";
        static constexpr LPCSTR SPECIALTY_FULL = "era.heroes.%d.specialty.full";
        static constexpr LPCSTR SPECIALTY_DESCRIPTION = "era.heroes.%d.specialty.description";
        static constexpr LPCSTR BIOGRAPHY = "era.heroes.%d.biography";
    };

  public:
    static void Init()
    {
        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        LPCSTR *table = (*reinterpret_cast<LPCSTR **>(0x005B9A18 + 2));

        const int heroCount = H3HeroCount::Get();

        for (size_t i = 0; i < heroCount; i++)
        {

            libc::sprintf(h3_TextBuffer, formats::BIOGRAPHY, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                table[i] = readResult;
            }
            libc::sprintf(h3_TextBuffer, formats::NAME, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                P_HeroInfo[i].name = readResult;
            }

            libc::sprintf(h3_TextBuffer, formats::SPECIALTY_SHORT, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                P_HeroSpecialty[i].spShort = readResult;
            }

            libc::sprintf(h3_TextBuffer, formats::SPECIALTY_FULL, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                P_HeroSpecialty[i].spFull = readResult;
            }

            libc::sprintf(h3_TextBuffer, formats::SPECIALTY_DESCRIPTION, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                P_HeroSpecialty[i].spDescr = readResult;
            }
            // if (i == 203)
            //{
            //     auto &spec = P_HeroSpecialty[i];

            //    int test = 0;
            //    // H3Messagebox();
            //}
        }
    }
};
