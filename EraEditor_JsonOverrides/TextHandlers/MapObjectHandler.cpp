#include "MapObjectHandler.h"

#include <unordered_set>

bool __stdcall Load_Objnames_TXT(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    // LPCSTR *table = H3DwellingNames1::Get();

    // const int dwellings1Num = GetSubtypesAmount(eObject::CREATURE_GENERATOR1);
    //  const int dwellings4Num = GetSubtypesAmount(eObject::CREATURE_GENERATOR4);

    //// load dwelling names
    // for (size_t i = 0; i < dwellings1Num; i++)
    //{

    //    sprintf(textBuffer, formats::DWELLING1, i);
    //    readResult = EraJS::read(textBuffer, readSuccess);
    //    if (readSuccess)
    //        table[i] = readResult;
    //}
    // table = H3DwellingNames4::Get();
    //// load dwelling 4 names
    // for (size_t i = 0; i < dwellings4Num; i++)
    //{
    //     sprintf(textBuffer, formats::DWELLING4, i);
    //     readResult = EraJS::read(textBuffer, readSuccess);
    //     if (readSuccess)
    //         table[i] = readResult;
    // }

    // load native object names
    readSuccess = false;
    readResult = nullptr;
    auto mapObjectsTable = MapObjectData::Get();
    for (size_t i = 0; i < limits::OBJECTS; i++)
    {
        sprintf(textBuffer, MapObjectHandler::formats::OBJECTS, i);
        readResult = EraJS::read(textBuffer, readSuccess);
        if (readSuccess)
            mapObjectsTable[i].objectName = readResult; // + 1 because first object is empty
    }

    return result;
}
bool __stdcall Load_CrBanks_TXT(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    bool readSuccess = false;
    LPCSTR readResult = nullptr;

    auto creatureBankNames = *reinterpret_cast<LPCSTR **>(0x048D496 + 1);
    for (size_t i = 0; i < limits::OBJECTS; i++)
    {
        sprintf(textBuffer, MapObjectHandler::formats::CREATURE_BANKS, eObject::CREATURE_BANK, i);
        readResult = EraJS::read(textBuffer, readSuccess);
        if (readSuccess)
            creatureBankNames[i] = readResult; // + 1 because first object is empty
    }

    return result;
}

void MapObjectHandler::Init()
{
    _PI->WriteHiHook(0x045C713, CDECL_, Load_Objnames_TXT);
    _PI->WriteHiHook(0x048D3A2, CDECL_, Load_CrBanks_TXT);
}

int MapObjectHandler::GetSubtypesAmount(const eObject type) noexcept
{
    std::unordered_set<int> objectSubtypes;
    for (auto &i : P_Game->mainSetup.objectLists[type])
    {
        objectSubtypes.insert(i.subtype);
    }
    return objectSubtypes.size();
}
