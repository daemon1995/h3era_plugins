#pragma once
#include "..\pch.h"

constexpr UINT STATES_AMOUNT = 4;
constexpr UINT GUARDES_AMOUNT = 5;
constexpr UINT MITHRIL_ID = 7;
constexpr UINT SPELLS_AMOUNT = 4;
constexpr UINT SKILLS_AMOUNT = 4;
constexpr UINT ARTIFACTS_AMOUNT = 4;

struct CustomRewardSetupState
{
    static INT maxArtId;

    BOOL enabled = true;
    UINT stateId = 0;
    UINT mithrilAmount = 0;
    UINT experience = 0;
    UINT spellPoints = 0;
    INT luck = 0;
    INT morale = 0;
    eArtifact artifactIds[ARTIFACTS_AMOUNT] = {eArtifact::NONE, eArtifact::NONE, eArtifact::NONE, eArtifact::NONE};

    UINT8 primarySkills[SKILLS_AMOUNT] = {0, 0, 0, 0};
    struct SpellsReward
    {
        eSpell spellId = eSpell::NONE;
        eSpellchool spellSchool = eSpellchool::ALL;
        DWORD spellLevels = 0x3E;
        DWORD spellFlags = -1;
        BOOL generate = false;

    } spellsRewards[SPELLS_AMOUNT];
    LPCSTR customDefName = nullptr;

  public:
    CustomRewardSetupState(const INT creatureBankType = 0, const UINT stateId = 0) noexcept;
};

struct CustomCreatureBank
{

#define DECLARE_FIELD(type, name, defaultValue)                                                                        \
    type name = defaultValue;                                                                                          \
    static constexpr LPCSTR name##Format = "RMG_CreatureBankId_%d_" #name;

#define DECLARE_INDEXED_FIELD(type, name, size, defaultValue)                                                          \
    type name[size] = {defaultValue, defaultValue, defaultValue, defaultValue};                                        \
    static constexpr LPCSTR name##Format = "RMG_CreatureBankId_%d_" #name "_%d";

    DECLARE_FIELD(UINT, id, -1)
    DECLARE_FIELD(UINT, stateIndex, -1)
    DECLARE_FIELD(UINT, mithrilAmount, 0)
    DECLARE_FIELD(UINT, experience, 0)
    DECLARE_FIELD(UINT, spellPoints, 0)
    DECLARE_FIELD(INT, luck, 0)
    DECLARE_FIELD(INT, morale, 0)

    DECLARE_INDEXED_FIELD(UINT8, primarySkills, SKILLS_AMOUNT, 0)
    DECLARE_INDEXED_FIELD(eSpell, spells, SPELLS_AMOUNT, eSpell::NONE)

  public:
    // used at initialization of the creature bank
    CustomCreatureBank(const CustomRewardSetupState &setupState, const UINT index);
    // used at loading of the creature bank from save
    CustomCreatureBank(const int craetureBankId);

  private:
    void GenerateSpells(const CustomRewardSetupState &setup);

  public:
    void WriteSaveData() const noexcept;
    void ClearAssocVariables() const noexcept;

    //  static BOOL Create(const H3CreatureBank &bank);
};

struct CreatureBankManager
{

    UINT m_size = 0;

    // default data
    std::vector<int> monsterAwards;
    std::vector<std::array<int, GUARDES_AMOUNT>> monsterGuards;
    std::vector<H3CreatureBankSetup> setups;

    // custom data
    std::vector<int> isBankSettings;
    std::vector<std::array<int, 14>> customPositions;
    std::vector<std::array<CustomRewardSetupState, STATES_AMOUNT>> customRewardSetups;

    // used during the Game same way as P_Game->creatureBanks;
    std::vector<CustomCreatureBank> customCreatureBanksVector;
    // std::unordered_map<H3CreatureBank *, CustomCreatureBank> customCreatureBanks;

  private:
    void CopyDefaultData(const size_t defaultSize, const std::array<int, 14> &defaultPositions);

    void Resize(const size_t size) noexcept;
    void Reserve(const size_t size) noexcept;
    void ShrinkToFit() noexcept;

  public:
    int LoadCreatureBanksFromJson(const INT16 defaultBanksNumber, const INT16 maxSubtype);
    const CustomCreatureBank *GetCustomCreatureBank(const H3MapItem *mapItem) const noexcept;
    const CustomCreatureBank *GetCustomCreatureBank(const H3CreatureBank *creatureBank) const noexcept;

  public:
    static eObject GetCreatureBankObjectType(const int cbId) noexcept;
};
