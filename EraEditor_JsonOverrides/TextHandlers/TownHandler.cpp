#include "TownHandler.h"
#include "../../headers/Era/JsonOverridesSchema.hpp"

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
            const int dwellingId = static_cast<int>(j) + 20; // Editor layout differs from game building IDs.
            EraJS::ReadTownDwelling(townBuilding[townType][dwellingId].name, jsonTownId, static_cast<int>(j), false);
            EraJS::ReadTownDwelling(townBuilding[townType][dwellingId].description, jsonTownId, static_cast<int>(j), true);
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
