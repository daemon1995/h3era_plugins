#include "HandlersList.h"

void TownHandler::Init()
{

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    //  auto table = H3CreatureInformation::Get();
    const auto townDwellingNames = GetTownDwellingNames();

    const auto townDwellingDescriptions = GetTownDwellingDescriptions();

    const UINT dwellinsPerTown = ByteAt(0x05B995F + 2);
    const UINT townsNum = DwordAt(0x05B9962 + 2) / dwellinsPerTown;

    const UINT neutralTownId = townsNum - 1;

    for (size_t i = 0; i < townsNum; i++)
    {
        const int jsonTownId = i == neutralTownId ? -1 : i; // Neutral town is 0 in JSON, others are 1-indexed
        for (size_t j = 0; j < dwellinsPerTown; j++)
        {
            const UINT stringId = i * dwellinsPerTown + j;

            sprintf(h3_TextBuffer, formats::DWELLING, jsonTownId, j);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                townDwellingNames[stringId] = readResult;

            sprintf(h3_TextBuffer, formats::DWELLING_DESCRIPTION, jsonTownId, j);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                townDwellingDescriptions[stringId] = readResult;
        }
    }

    //  _PI->WriteHiHook(0x04EDF72, CDECL_, Load_Dwelling_TXT);
}

LPCSTR *TownHandler::GetTownDwellingNames() noexcept
{
    return *reinterpret_cast<LPCSTR **>(0x05B9923 + 2);
}
LPCSTR *TownHandler::GetTownDwellingDescriptions() noexcept
{
    return *reinterpret_cast<LPCSTR **>(0x05B9957 + 2);
}
