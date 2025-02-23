#include "MonsterHandler.h"
#include "pch.h"

bool __stdcall LoadCranimTxt(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    if (result)
    {
        const int MAX_MON_ID = IntAt(0x4A1657);
        H3CreatureAnimation *animTable = *reinterpret_cast<H3CreatureAnimation **>(0x67FF74);
        for (size_t i = 0; i < MAX_MON_ID; i++)
        {
            //   animTable[i].walkAnimationTime =1;
            // P_CreatureA::Get() = 12;
        }
    }

    return result;
}
bool __stdcall LoadCrtraitsTxt(HiHook *h, int a1)
{
    bool result = CDECL_1(bool, h->GetDefaultFunc(), a1);

    if (result)
    {
        static BOOL firstCall = true;

        if (!firstCall)
            return result;
        firstCall = false;

        const int MAX_MON_ID = IntAt(0x4A1657);

        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        //  auto table = H3CreatureInformation::Get();
        const auto singleNames = *reinterpret_cast<LPCSTR **>(0x047B12C + 1);
        const auto pluralNames = *reinterpret_cast<LPCSTR **>(0x047B10C + 1);
        const auto descriptions = *reinterpret_cast<LPCSTR **>(0x047B0EC + 1);

        for (size_t i = 0; i < MAX_MON_ID; i++)
        {
            sprintf(h3_TextBuffer, "era.monsters.%d.name.singular", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                singleNames[i] = readResult;

            sprintf(h3_TextBuffer, "era.monsters.%d.name.plural", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                pluralNames[i] = readResult;

            sprintf(h3_TextBuffer, "era.monsters.%d.name.description", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                descriptions[i] = readResult;
        }
    }

    return result;
}

void MonsterHandler::Init()
{
    _PI->WriteHiHook(0x07117CA, CDECL_, LoadCrtraitsTxt); //
                                                          //  _PI->WriteHiHook(0x04EDF4B, CDECL_, LoadCranimTxt);   //
}
