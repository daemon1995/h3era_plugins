#include "HandlersList.h"

void HeroHandler::Init()
{
    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = (*reinterpret_cast<LPCSTR **>(0x005B9A18 + 2));

    const int heroCount = H3HeroCount::Get();

    for (size_t i = 0; i < heroCount; i++)
    {

        sprintf(h3_TextBuffer, formats::BIOGRAPHY, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;

        sprintf(h3_TextBuffer, formats::NAME, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            P_HeroInfo[i].name = readResult;

        sprintf(h3_TextBuffer, formats::SPECIALTY_SHORT, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            P_HeroSpecialty[i].spShort = readResult;

        sprintf(h3_TextBuffer, formats::SPECIALTY_FULL, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            P_HeroSpecialty[i].spFull = readResult;

        sprintf(h3_TextBuffer, formats::SPECIALTY_DESCRIPTION, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            P_HeroSpecialty[i].spDescr = readResult;
    }
}
