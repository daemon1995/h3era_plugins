#pragma once
#include "HandlersList.h"

class TownHandler
{

  public:
    struct formats
    {
        static constexpr LPCSTR NAME = "era.towns.%d.name";
        static constexpr LPCSTR RANDOM_NAME = "era.towns.%d.names.%d";
        static constexpr LPCSTR DWELLING = "era.towns.%d.dwellings.%d.name";
        static constexpr LPCSTR DWELLING_DESCRIPTION = "era.towns.%d.dwellings.%d.description";
    };
    static constexpr int NEUTRAL_TOWN_ID = -1; // Neutral town is -1 in JSON, others are indexed
    static constexpr UINT TOWN_TYPES_COUNT = 9;
    static constexpr UINT RANDOM_NAMES_PER_TOWN = 16;

  protected:
    static inline LPCSTR *GetTownDwellingNames() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B9923 + 2);
    }
    static inline LPCSTR *GetTownDwellingDescriptions() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B9957 + 2);
    }

    static inline LPCSTR *GetTownNames() noexcept
    {
        // Town_GetName loads the table through the absolute operand of
        // "mov eax, TownNamesArr[eax * 4]". Read that operand instead of
        // duplicating the data address here, so the table may be relocated.
        return *reinterpret_cast<LPCSTR **>(0x005C1854 + 3);
    }

    static inline LPCSTR *GetRandomTownNames() noexcept
    {
        // The random-town generator uses the same table in this instruction.
        // Its disp32 operand is the current address of TOWNNAME.TXT.
        return *reinterpret_cast<LPCSTR **>(0x004CA9D0 + 3);
    }

  public:
    static void Init()
    {
        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        //  auto table = H3CreatureInformation::Get();
        const auto townDwellingNames = GetTownDwellingNames();

        const auto townDwellingDescriptions = GetTownDwellingDescriptions();

        // Names of town types (Castle, Rampart, ...), used by Town_GetName.
        const auto townNames = GetTownNames();
        for (UINT i = 0; i < TOWN_TYPES_COUNT; i++)
        {
            libc::sprintf(h3_TextBuffer, formats::NAME, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                townNames[i] = readResult;
        }

        // Random town names from TOWNNAME.TXT: 9 town-type groups, 16 names each.
        const auto randomTownNames = GetRandomTownNames();
        for (UINT i = 0; i < TOWN_TYPES_COUNT; i++)
        {
            for (UINT j = 0; j < RANDOM_NAMES_PER_TOWN; j++)
            {
                libc::sprintf(h3_TextBuffer, formats::RANDOM_NAME, i, j);
                readResult = EraJS::read(h3_TextBuffer, readSuccess);
                if (readSuccess)
                    randomTownNames[i * RANDOM_NAMES_PER_TOWN + j] = readResult;
            }
        }

        const UINT dwellinsPerTown = ByteAt(0x05B995F + 2);
        const UINT townsNum = DwordAt(0x05B9962 + 2) / dwellinsPerTown;

        const UINT neutralTownId = townsNum - 1;

        for (size_t i = 0; i < townsNum; i++)
        {
            const int jsonTownId = i == neutralTownId ? -1 : i; // Neutral town is 0 in JSON, others are 1-indexed
            for (size_t j = 0; j < dwellinsPerTown; j++)
            {
                const UINT stringId = i * dwellinsPerTown + j;

                libc::sprintf(h3_TextBuffer, formats::DWELLING, jsonTownId, j);
                readResult = EraJS::read(h3_TextBuffer, readSuccess);
                if (readSuccess)
                    townDwellingNames[stringId] = readResult;

                libc::sprintf(h3_TextBuffer, formats::DWELLING_DESCRIPTION, jsonTownId, j);
                readResult = EraJS::read(h3_TextBuffer, readSuccess);
                if (readSuccess)
                    townDwellingDescriptions[stringId] = readResult;
            }
        }

        //  _PI->WriteHiHook(0x04EDF72, CDECL_, Load_Dwelling_TXT);
    }
};
