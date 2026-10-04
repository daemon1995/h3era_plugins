#pragma once
#include "HandlersList.h"

#define HERO_INFO_FIELDS(X)                                                                                           \
    X(isFemale)                                                                                                       \
    X(race)                                                                                                           \
    X(heroClass)                                                                                                      \
    X(hasSpellbook)                                                                                                   \
    X(startingSpell)                                                                                                  \
    X(armyType)                                                                                                       \
    X(smallPortrait)                                                                                                  \
    X(largePortrait)                                                                                                  \
    X(roeHero)                                                                                                        \
    X(expansionHero)                                                                                                  \
    X(campaignHero)

class HeroHandler
{

    struct formats
    {
        static constexpr LPCSTR NAME = "era.heroes.%d.name";
        static constexpr LPCSTR SPECIALTY_SHORT = "era.heroes.%d.specialty.short";
        static constexpr LPCSTR SPECIALTY_FULL = "era.heroes.%d.specialty.full";
        static constexpr LPCSTR SPECIALTY_DESCRIPTION = "era.heroes.%d.specialty.description";
        static constexpr LPCSTR BIOGRAPHY = "era.heroes.%d.biography";

#define X(field) GENERATE_FORMAT_STR(H3HeroInfo, heroes, field)
        HERO_INFO_FIELDS(X)
#undef X
    };

  public:
    static void Init()
    {
        auto *biographies = EraJS::GameTables::HeroBiographies();
        const int heroCount = H3HeroCount::Get();
        for (int i = 0; i < heroCount; ++i)
        {
            EraJS::ReadField(biographies[i], formats::BIOGRAPHY, i);
            auto &info = P_HeroInfo[i];
            EraJS::ReadField(info.name, formats::NAME, i);
            EraJS::ReadNumberInRange(info.isFemale, formats::isFemale, 0, 1, i);
            EraJS::ReadNumberInRange(info.race, formats::race, 0, 13, i);
            EraJS::ReadNumberInRange(info.heroClass, formats::heroClass, 0, h3::limits::HERO_CLASSES - 1, i);
            EraJS::ReadNumberInRange(info.hasSpellbook, formats::hasSpellbook, 0, 1, i);
            EraJS::ReadNumberInRange(info.startingSpell, formats::startingSpell, -1, h3::limits::TOTAL_SPELLS - 1, i);
            const int creatureCount = IntAt(0x4A1657);
            for (int army = 0; army < 3; ++army)
                EraJS::ReadNumberInRange(info.armyType[army], formats::armyType, -1, creatureCount - 1, i, army);
            EraJS::ReadResourceField(info.smallPortrait, formats::smallPortrait, i);
            EraJS::ReadResourceField(info.largePortrait, formats::largePortrait, i);
            EraJS::ReadNumberInRange(info.roeHero, formats::roeHero, 0, 1, i);
            EraJS::ReadNumberInRange(info.expansionHero, formats::expansionHero, 0, 1, i);
            EraJS::ReadNumberInRange(info.campaignHero, formats::campaignHero, 0, 1, i);
            EraJS::ReadField(P_HeroSpecialty[i].spShort, formats::SPECIALTY_SHORT, i);
            EraJS::ReadField(P_HeroSpecialty[i].spFull, formats::SPECIALTY_FULL, i);
            EraJS::ReadField(P_HeroSpecialty[i].spDescr, formats::SPECIALTY_DESCRIPTION, i);
        }
    }
};
#undef HERO_INFO_FIELDS
