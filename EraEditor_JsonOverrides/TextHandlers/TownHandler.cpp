#include "TownHandler.h"

void __stdcall Load_BuildingNames(HiHook *h)
{
    CDECL_0(void, h->GetDefaultFunc());

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    //  auto table = H3CreatureInformation::Get();
    const auto townBuilding = TownBuildingTextData::Get();

    const UINT dwellingsNum = 14; // ByteAt(0x05B995F + 2);
    const UINT townsNum = 10;     // DwordAt(0x05B9962 + 2) / dwellinsPerTown;
    const UINT neutralTownId = townsNum - 1;

    // dwellings
    for (size_t townType = 0; townType < townsNum; townType++)
    {
        const int jsonTownId =
            townType == neutralTownId ? -1 : townType; // Neutral town is 0 in JSON, others are 1-indexed
        for (size_t j = 0; j < dwellingsNum; j++)
        {
            sprintf(textBuffer, TownHandler::formats::DWELLING, jsonTownId, j);
            readResult = EraJS::read(textBuffer, readSuccess);
            const int dwellingId = j + 20;
            if (readSuccess)
                townBuilding[townType][dwellingId].name = readResult;

            sprintf(textBuffer, TownHandler::formats::DWELLING_DESCRIPTION, jsonTownId, j);
            readResult = EraJS::read(textBuffer, readSuccess);
            if (readSuccess)
                townBuilding[townType][dwellingId].description = readResult;
        }
    }
}
void TownHandler::Init()
{
    _PI->WriteHiHook(0x045C78B, CDECL_, Load_BuildingNames);
}

LPCSTR *TownHandler::GetTownDwellingNames() noexcept
{
    return *reinterpret_cast<LPCSTR **>(0x05B9923 + 2);
}
LPCSTR *TownHandler::GetTownDwellingDescriptions() noexcept
{
    return *reinterpret_cast<LPCSTR **>(0x05B9957 + 2);
}
