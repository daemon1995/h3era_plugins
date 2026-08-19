#pragma once
#include "HandlersList.h"

class TownHandler
{

  public:
    struct formats
    {
        static constexpr LPCSTR COMMON_BUILDING_NAME = "era.towns.buildings.%d.name";
        static constexpr LPCSTR COMMON_BUILDING_DESCRIPTION = "era.towns.buildings.%d.description";
        static constexpr LPCSTR TOWN_TYPE_NAME = "era.towns.%d.name";
        static constexpr LPCSTR TOWN_RANDOM_NAME = "era.towns.%d.names.%d";
        static constexpr LPCSTR BUILDING_NAME = "era.towns.%d.buildings.%d.name";
        static constexpr LPCSTR BUILDING_DESCRIPTION = "era.towns.%d.buildings.%d.description";
    };
    static constexpr int NEUTRAL_TOWN_ID = -1; // Neutral town is -1 in JSON, others are indexed
    static constexpr UINT TOWN_TYPES_COUNT = 9;
    static constexpr UINT RANDOM_NAMES_PER_TOWN = 16;
    static constexpr UINT DWELLINGS_PER_TOWN = 14;
    static constexpr UINT SPEC_BUILDINGS_PER_TOWN = 11;

  protected:
    static inline LPCSTR *GetTownDwellingNames() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B9923 + 2);
    }
    static inline LPCSTR *GetTownDwellingDescriptions() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B9957 + 2);
    }
    static inline LPCSTR *GetTownSpecBuildingNames() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B98BC + 2);
    }
    static inline LPCSTR *GetTownSpecBuildingDescriptions() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B98C7 + 2);
    }
    static inline LPCSTR *GetBlackSmithNames() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05D215A + 1);
    }
    static inline LPCSTR *GetBlackSmithDescriptions() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05D2E64 + 3);
    }
    static inline LPCSTR *GetCommonBuildingNames() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B97FC + 2);
    }
    static inline LPCSTR *GetCommonBuildingDescriptions() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x05B9833 + 2);
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
    static void OverrideCommonBuildingsText()
    {
        auto commonBuildingNames = GetCommonBuildingNames();
        auto commonBuildingDescriptions = GetCommonBuildingDescriptions();
        for (size_t i = 0; i < eBuildings::SPEC17; i++)
        {
            libc::sprintf(h3_TextBuffer, formats::COMMON_BUILDING_NAME, i);
            EraJS::ReadSingleValue<LPCSTR>(commonBuildingNames[i], h3_TextBuffer);
            libc::sprintf(h3_TextBuffer, formats::COMMON_BUILDING_DESCRIPTION, i);
            EraJS::ReadSingleValue<LPCSTR>(commonBuildingDescriptions[i], h3_TextBuffer);
        }
    }
    static void OverrideTownText(const int townTypeId, const int jsonIndex)
    {

        // const int jsonIndex = townTypeId; // == NEUTRAL_TOWN_ID;// ? 0 : townTypeId + 1; // Neutral town is 0 in
        // JSON, others are 1-indexed

        bool readSuccess = false;
        LPCSTR readResult = nullptr;

        // auto textArrayPtr

        // handle town type name
        // neutral town has -1 offset in the array adderss
        libc::sprintf(h3_TextBuffer, formats::TOWN_TYPE_NAME, jsonIndex);
        EraJS::ReadSingleValue<LPCSTR>(GetTownNames()[jsonIndex], h3_TextBuffer);

        // handle random town names
        if (jsonIndex >= 0) // only for non-neutral towns
        {
            auto randomTownNames = GetRandomTownNames();
            for (UINT j = 0; j < RANDOM_NAMES_PER_TOWN; j++)
            {
                libc::sprintf(h3_TextBuffer, formats::TOWN_RANDOM_NAME, jsonIndex, j);
                readResult = EraJS::read(h3_TextBuffer, readSuccess);
                if (readSuccess)
                    randomTownNames[townTypeId * RANDOM_NAMES_PER_TOWN + j] = readResult;
            }
        }

        // handle town building names and descriptions

        const auto townSpecBuildingNames = GetTownSpecBuildingNames();
        const auto townSpecBuildingDescriptions = GetTownSpecBuildingDescriptions();

        UINT stringId = townTypeId * SPEC_BUILDINGS_PER_TOWN + 10; // silo has weird offset, so we handle it separately

        libc::sprintf(h3_TextBuffer, formats::BUILDING_NAME, jsonIndex, eBuildings::RESOURCE_SILO);
        EraJS::ReadSingleValue<LPCSTR>(townSpecBuildingNames[stringId], h3_TextBuffer, readSuccess);

        libc::sprintf(h3_TextBuffer, formats::BUILDING_DESCRIPTION, jsonIndex, eBuildings::RESOURCE_SILO);
        EraJS::ReadSingleValue<LPCSTR>(townSpecBuildingDescriptions[stringId], h3_TextBuffer, readSuccess);

        // blacksmith is a special case, and it stored int the bldgneut.txt
        // libc::sprintf(h3_TextBuffer, formats::BUILDING_NAME, jsonIndex, eBuildings::BLACKSMITH);
        // EraJS::ReadSingleValue<LPCSTR>(GetBlackSmithNames()[townTypeId], h3_TextBuffer, readSuccess);

        libc::sprintf(h3_TextBuffer, formats::BUILDING_DESCRIPTION, jsonIndex, eBuildings::BLACKSMITH);
        EraJS::ReadSingleValue<LPCSTR>(GetBlackSmithDescriptions()[townTypeId], h3_TextBuffer, readSuccess);

        UINT buildingId = eBuildings::SPEC17;
        stringId = townTypeId * SPEC_BUILDINGS_PER_TOWN;
        constexpr UINT LAST_INDEXED_SPEC_BUILDING = 10;
        for (size_t i = 0; i < LAST_INDEXED_SPEC_BUILDING; i++)
        {
            libc::sprintf(h3_TextBuffer, formats::BUILDING_NAME, jsonIndex, buildingId);
            EraJS::ReadSingleValue<LPCSTR>(townSpecBuildingNames[stringId], h3_TextBuffer, readSuccess);

            libc::sprintf(h3_TextBuffer, formats::BUILDING_DESCRIPTION, jsonIndex, buildingId);
            EraJS::ReadSingleValue<LPCSTR>(townSpecBuildingDescriptions[stringId], h3_TextBuffer, readSuccess);

            buildingId++;
            stringId++;
        }

        const auto townDwellingNames = GetTownDwellingNames();
        const auto townDwellingDescriptions = GetTownDwellingDescriptions();

        // handle town dwelling names and descriptions
        buildingId = eBuildings::DWELL1;
        stringId = townTypeId * DWELLINGS_PER_TOWN;

        for (size_t i = 0; i < DWELLINGS_PER_TOWN; i++)
        {
            libc::sprintf(h3_TextBuffer, formats::BUILDING_NAME, jsonIndex, buildingId);
            EraJS::ReadSingleValue<LPCSTR>(townDwellingNames[stringId], h3_TextBuffer);

            libc::sprintf(h3_TextBuffer, formats::BUILDING_DESCRIPTION, jsonIndex, buildingId);
            EraJS::ReadSingleValue<LPCSTR>(townDwellingDescriptions[stringId], h3_TextBuffer);

            buildingId++;
            stringId++;
        }
    }
    static void Init()
    {

        OverrideCommonBuildingsText();
        const size_t townsCount = H3TownCount::Get();
        // original town types;
        for (size_t i = 0; i < townsCount; i++)
        {
            OverrideTownText(i, i);
        }
        OverrideTownText(townsCount, -1); // Neutral town is -1 in JSON, others are indexed
    }
};
