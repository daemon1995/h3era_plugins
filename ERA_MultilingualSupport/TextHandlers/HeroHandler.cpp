#include "HandlersList.h"

void HeroHandler::Init()
{
    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = (*reinterpret_cast<LPCSTR **>(0x005B9A18 + 2));

    const int heroCount = H3HeroCount::Get();

    // libc::sprintf(h3_TextBuffer, formats::BIOGRAPHY, 0);
    // MessageBoxA(NULL, EraJS::read(h3_TextBuffer), "ERA Multilingual Support", MB_OK);

    // MessageBoxA(NULL, EraJS::read(h3_TextBuffer), "ERA Multilingual Support", MB_OK);
    // MessageBoxA(NULL, EraJS::read(h3_TextBuffer), "ERA Multilingual Support", MB_OK);
    // MessageBoxA(NULL, EraJS::read(h3_TextBuffer), "ERA Multilingual Support", MB_OK);
    // MessageBoxA(NULL, EraJS::read(h3_TextBuffer), "ERA Multilingual Support", MB_OK);
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
