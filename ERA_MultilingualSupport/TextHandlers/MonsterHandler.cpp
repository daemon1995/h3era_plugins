#include "HandlersList.h"

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

void LoadJsonData(const BOOL refreshText)
{

    const int MAX_MON_ID = IntAt(0x4A1657);

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    //  auto table = H3CreatureInformation::Get();
    const auto singleNames = *reinterpret_cast<LPCSTR **>(0x047B12C + 1);
    const auto pluralNames = *reinterpret_cast<LPCSTR **>(0x047B10C + 1);
    const auto descriptions = *reinterpret_cast<LPCSTR **>(0x047B0EC + 1);

    for (size_t i = 0; i < MAX_MON_ID; i++)
    {
        libc::sprintf(h3_TextBuffer, "era.monsters.%d.name.singular", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            singleNames[i] = readResult;

        libc::sprintf(h3_TextBuffer, "era.monsters.%d.name.plural", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            pluralNames[i] = readResult;

        libc::sprintf(h3_TextBuffer, "era.monsters.%d.name.description", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            descriptions[i] = readResult;

        if (refreshText)
        {
            // temp ? fix for missing broken name pointers
            P_CreatureInformation[i].namePlural = pluralNames[i];
            P_CreatureInformation[i].nameSingular = singleNames[i];
            P_CreatureInformation[i].description = descriptions[i];
        }
    }
}

bool __stdcall Load_CrTraits_TXT(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    if (result)
    {
        LoadJsonData(true);
    }
    return result;
}

bool __stdcall WoG_OnMapReset(HiHook *h, int a1)
{
    bool result = CDECL_1(bool, h->GetDefaultFunc(), a1);

    if (result)
    {
        LoadJsonData(true);
    }

    return result;
}

void MonsterHandler::Init()
{
    _PI->WriteHiHook(0x04EDE90, CDECL_, Load_CrTraits_TXT); //
    _PI->WriteHiHook(0x07117CA, CDECL_, WoG_OnMapReset);    //
    // _PI->WriteHiHook(0x04EDF4B, CDECL_, LoadCranimTxt);  //
}
