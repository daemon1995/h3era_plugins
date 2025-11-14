#pragma once
#include "pch.h"

#include "map"
#include "set"
struct alignas(8) ArtifactsData
{
    struct ArtSimpleBonus
    {
        UINT artId;
        int bonus;
    };

    // General income data
    std::vector<ArtSimpleBonus> artifactsWhichAddGold;

    // General Combat procedure art
    std::vector<UINT> artifactsThatGiveAdditionalShot;
    std::vector<UINT> artifactsThatGiveFullTentHeal;
    std::vector<ArtSimpleBonus> artifactsWhichIncreaseTentHealing;

    struct ArtAutoCastSpellBonus
    {
        UINT artId;
        UINT spellID;
        UINT skillLevel;
        // -1 equal to heroe's
        INT power;
        BOOL affectOnlyEnemy;
    };

    // General Spells data
    std::vector<ArtAutoCastSpellBonus> autoCastedSpellsByArtifactId;
    std::vector<ArtSimpleBonus> artifactsWhichIncreaseSpellsDuration;
    std::vector<UINT> artifactsWhichSetExpertMagiclevel;

    // first arg is art id, second cost changer; +increase for enemy , - descrease for you id
    struct ArtFeature
    {

        // Movement Points cost

        // lands for settingPathfind on the all lands
        uint64_t ignorePenaltyLands;
        int landStepDiscount;
        int waterStepDiscount;
        int landMovepointsBonus;
        int waterMovepointsBonus;
        int buildingCostChanger;

        //  spell data
        float damage;
        float resurrection;
        float cure;

        BOOL levelsBanned[MAX_SPELL_LEVELS_AMOUNT];
        BOOL levelImmunities[MAX_SPELL_LEVELS_AMOUNT];

        BOOL hasSpells;
        std::vector<UINT> addedSpells;

        struct SpellsData
        {
            BOOL addsSpellWithLevel[MAX_SKILL_LEVELS_AMOUNT];
            BOOL free;
            BOOL ban;
        } spellsData[MAX_SPELLS_AMOUNT]; // should  I use limits::TOTAL_SPELLS ?
        int spellCostChanger;

        // NPC data
        BOOL isCommanderArt = false;
        struct NpcData
        {
            union {
                struct
                {
                    int attack;
                    int defence;
                    int hit_points;
                    int damage;
                    int spell_power;
                    int speed;
                    int resistance;
                };

                INT32 asArray[7];
            } parameters;
            struct
            {
                int attack;
                int defence;
                eSpell afterHitSpell = eSpell::NONE;

            } melee;
            struct
            {
                int attack;
                int defence;
                eSpell afterHitSpell = eSpell::NONE;

            } range;
            eSpell buffSpell = eSpell::NONE;
        } *npcData = nullptr;

    } artFeatures[MAX_ARTIFACTS_AMOUNT];

    std::vector<ArtSimpleBonus> secondarySkillBonuses[h3::limits::SECONDARY_SKILLS];

  public:
    int LoadJsonData();
    // create external function to reffresh combo pieces data based on the combo set id

    void AffectComboArts();

    void Clear();
};

struct alignas(sizeof(H3String)) PluginText
{

    struct Buffer
    {
        const char *spellIsForbiddenBy;
        const char *spellLevelForbiddenBy;

    } buffer;
    int LoadText();
};
