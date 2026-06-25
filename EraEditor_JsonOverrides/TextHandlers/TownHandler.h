#pragma once

class TownHandler
{

  public:
    struct formats
    {
        static constexpr LPCSTR DWELLING = "era.towns.%d.dwellings.%d.name";
        static constexpr LPCSTR DWELLING_DESCRIPTION = "era.towns.%d.dwellings.%d.description";
    };
    static constexpr int NEUTRAL_TOWN_ID = -1; // Neutral town is -1 in JSON, others are indexed
    static LPCSTR *GetTownDwellingNames() noexcept;
    static LPCSTR *GetTownDwellingDescriptions() noexcept;

  public:
    static void Init();
};
