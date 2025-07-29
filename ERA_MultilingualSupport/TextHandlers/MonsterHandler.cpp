#include "HandlersList.h"

std::vector<LPCSTR> MonsterHandler::originalSingleNames;
std::vector<LPCSTR> MonsterHandler::originalPluralNames;
std::vector<LPCSTR> MonsterHandler::originalDescriptions;

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
        {
            singleNames[i] = readResult;
        }
        libc::sprintf(h3_TextBuffer, "era.monsters.%d.name.plural", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
        {
            // pluralNames[i] = readResult;
        }

        libc::sprintf(h3_TextBuffer, "era.monsters.%d.name.description", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
        {
            descriptions[i] = readResult;
        }
        if (refreshText)
        {
            // temp ? fix for missing broken name pointers
            P_CreatureInformation[i].namePlural = pluralNames[i];
            P_CreatureInformation[i].nameSingular = singleNames[i];
            P_CreatureInformation[i].description = descriptions[i];
        }
    }
}

static void CopyMonsterNames(const DWORD originalArrayPtr, std::vector<LPCSTR> &data, const BOOL isSet)
{
    const int MAX_MON_ID = IntAt(0x4A1657);
    //  auto table = H3CreatureInformation::Get();
    const auto originalArray = *reinterpret_cast<LPCSTR **>(originalArrayPtr);
    if (isSet)
    {
        data.resize(MAX_MON_ID);
        libc::memcpy(data.data(), originalArray, sizeof(LPCSTR) * MAX_MON_ID);
    }
    else
    {
        libc::memcpy(originalArray, data.data(), sizeof(LPCSTR) * MAX_MON_ID);
        data.clear();
    }
}

bool __stdcall Load_CrTraits_TXT(HiHook *h)
{
    // copy original names and descriptions
    {

        CopyMonsterNames(0x047B12C + 1, MonsterHandler::originalSingleNames, true);
        CopyMonsterNames(0x047B10C + 1, MonsterHandler::originalPluralNames, true);
        CopyMonsterNames(0x047B0EC + 1, MonsterHandler::originalDescriptions, true);
    }

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
void __stdcall CreatureNamesSingle_Destruct(HiHook *h)
{
    CopyMonsterNames(0x047B12C + 1, MonsterHandler::originalSingleNames, false);
    CDECL_0(void, h->GetDefaultFunc());
}
void __stdcall CreatureNamesMulti_Destruct(HiHook *h)
{
    CopyMonsterNames(0x047B10C + 1, MonsterHandler::originalPluralNames, false);
    CDECL_0(void, h->GetDefaultFunc());
}
void __stdcall CreatureDescriptions_Destruct(HiHook *h)
{
    CopyMonsterNames(0x047B0EC + 1, MonsterHandler::originalDescriptions, false);
    CDECL_0(void, h->GetDefaultFunc());
}

void MonsterHandler::Init()
{
    _PI->WriteHiHook(0x04EDE90, CDECL_, Load_CrTraits_TXT); //
    _PI->WriteHiHook(0x07117CA, CDECL_, WoG_OnMapReset);    //

    // restore original pointers

    _PI->WriteHiHook(0x047B120, CDECL_, CreatureNamesSingle_Destruct);  //
    _PI->WriteHiHook(0x047B100, CDECL_, CreatureNamesMulti_Destruct);   //
    _PI->WriteHiHook(0x047B0E0, CDECL_, CreatureDescriptions_Destruct); //

    // _PI->WriteHiHook(0x04EDF4B, CDECL_, LoadCranimTxt);  //
}
