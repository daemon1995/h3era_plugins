#pragma once

#include <array>

namespace cbanks
{

class CreatureBanksExtender : public extender::ObjectsExtender
{
    static CreatureBanksExtender* instance;

    static constexpr UINT STATES_AMOUNT = 4;
    static constexpr UINT GUARDES_AMOUNT = 5;
    static constexpr UINT MITHRIL_ID = 7;
    static constexpr UINT SPELLS_AMOUNT = 4;
    static constexpr UINT SKILLS_AMOUNT = 4;
    static constexpr UINT ARTIFACTS_AMOUNT = 4;

    struct Current
    {
        INT type = -1;
        INT stateId = -1;
        H3CreatureBank *bank = nullptr;
        H3MapItem *mapItem = nullptr;
        H3Hero *hero = nullptr;
        Patch *positionsPatch = nullptr;
        LPCSTR customDefName = nullptr;

        std::array<eSpell, SPELLS_AMOUNT> spellsToLearn = {eSpell::NONE, eSpell::NONE, eSpell::NONE, eSpell::NONE};
        UINT mithrilToAdd = 0;
        H3String message = "";
        UINT spellPointsToAdd = 0;
        UINT experiencePointsToAdd = 0;
        INT MAX_ART_ID = limits::ARTIFACTS;
    };
    static Current currentCreatureBank;

  private:
    UINT defaultBanksNumber = 0;
    UINT addedBanksNumber = 0;

    struct CustomRewardSetupState
    {

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
        void Resize(const size_t size) noexcept;
        void Reserve(const size_t size) noexcept;
        void ShrinkToFit() noexcept;
        void CopyDefaultData(const size_t defaultSize, const std::array<int, 14> &defaultPositions);

      public:
        int LoadCreatureBanksFromJson(const INT16 defaultBanksNumber, const INT16 maxSubtype);

    } manager;

    /*    static BOOL GetArmyMessage(const H3CreatureBank *creatureBank, H3String &customDescription,
                                   const bool withoutBrackets = true) noexcept;
       */ // static BOOL ShowMultiplePicsArmyMessage(const char *message, const int messageType, const int x, const int y,
    // H3Army *army) noexcept;

  private:
    CreatureBanksExtender();
    virtual ~CreatureBanksExtender();

  protected:
    virtual void CreatePatches() override;
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes) override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept;

    // virtual void GetObjectPreperties() noexcept override  final;
  private:
    const CustomCreatureBank *GetCustomCreatureBank(const H3MapItem *mapItem) noexcept;

  private:
    static _LHF_(CrBank_BeforeSetupFromState);

    static int __stdcall WoG_PlaceObject(HiHook *h, const int x, const int y, const int z, const int objType,
                                         const int objSubtype, const int objType2, const int objSubtype2,
                                         const DWORD a8);
    static void __stdcall CrBank_AddToGameList(HiHook *h, H3Vector<H3CreatureBank> *creatureBanks,
                                               const H3CreatureBank *end, const UINT number,
                                               const H3CreatureBank *creatureBank);
    static _LHF_(Game_AfterInitMapItem);
    static _LHF_(Game_SetMapItemDef);

    static __int64 __stdcall AIHero_GetMapItemWeight(HiHook *h, H3Hero *hero, int *moveDistance, UINT mixedPos);

    static _LHF_(CrBank_DisplayPreCombatMessage);
    static _LHF_(SpecialCrBank_DisplayPreCombatMessage);
    static void __stdcall CrBank_AskForVisitMessage(HiHook *h, char *mes, const int messageType, const int x,
                                                    const int y, const int picType1, const int picSubtype1,
                                                    const int picType2, const int picSubtype2, const int par,
                                                    const int time, const int picType3, const int picSubtype3);

    static _LHF_(CrBank_BeforeCombatStart);
    static signed int __stdcall CrBank_CombatStart(HiHook *h, H3AdventureManager *advMan, UINT PisMixed,
                                                   const H3Hero *attHero, UINT attArmy, int PlayerIndex, UINT defTown,
                                                   UINT defHero, UINT defArmy, int seed, signed int a10, int isBank);
    static _LHF_(CrBank_AfterCombatWon);

    static _LHF_(CrBank_AfterDrawingResources);
    static _LHF_(CrBank_BeforeShowingRewardMessage);
    static int __stdcall CrBank_BeforeEndingText(HiHook *h, H3String *mes, const size_t len, const DWORD a3,
                                                 const DWORD a4) noexcept;

    static _LHF_(CrBank_BeforeGivingResources);

    static void __stdcall OnBeforeSaveGame(Era::TEvent *Event);
    static void __stdcall OnAfterSaveGame(Era::TEvent *Event);
    static void __stdcall OnAfterLoadGame(Era::TEvent *Event);
    static void __stdcall OnGameLeave(Era::TEvent *Event);

    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *event);
    //	static int __stdcall CretureBankSetups__Ctor(HiHook*h);
  public:
    UINT Size() const noexcept;

  public:
    static int GetCreatureBankType(const int objType, const int objSubtype) noexcept;
    static int GetCreatureBankType(const H3MapItem *mapItem) noexcept;
    static eObject GetCreatureBankObjectType(const int cbId) noexcept;

    static CreatureBanksExtender &Get();
};

} // namespace cbanks
