#pragma once
struct MonsterHandler
{

    static std::vector<LPCSTR> originalSingleNames, originalPluralNames, originalDescriptions;
    H3CreatureInformation p;

  public:
    struct formats
    {
        static constexpr LPCSTR SINGULAR = "era.monsters.%d.name.singular";
        static constexpr LPCSTR PLURAL = "era.monsters.%d.name.plural";
        static constexpr LPCSTR DESCRIPTION = "era.monsters.%d.description";
    };
    static void Init();
};
