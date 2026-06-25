#pragma once
class HeroHandler
{

    struct formats
    {
        static constexpr LPCSTR NAME = "era.heroes.%d.name";
        static constexpr LPCSTR SPECIALTY_SHORT = "era.heroes.%d.specialty.short";
        static constexpr LPCSTR SPECIALTY_FULL = "era.heroes.%d.specialty.full";
        static constexpr LPCSTR SPECIALTY_DESCRIPTION = "era.heroes.%d.specialty.description";
        static constexpr LPCSTR BIOGRAPHY = "era.heroes.%d.biography";
    };

  public:
    static void Init();
};
