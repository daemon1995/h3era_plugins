#pragma once

#include <array>

namespace cbanks
{

class CreatureBanksExtender : public extender::ObjectsExtender
{

    static constexpr LPCSTR creatureBankStateFormat = "RMG_CreatureBankStateById_%d";
    static constexpr UINT STATES_AMOUNT = 4;
    static constexpr UINT GUARDES_AMOUNT = 5;
    static constexpr UINT MITHRIL_ID = 7;

    static INT currentCreatureBankId;
    static H3MapItem *currentMapItem;
    static H3CreatureBank *currentCreatureBank;
    static std::array<eSpell, STATES_AMOUNT> spellsToLearn;
    static INT creatureBankStateId;
    static UINT mithrilToAdd;

  private:
    UINT16 defaultBanksNumber = 0;
    UINT16 addedBanksNumber = 0;

    struct CustomReward
    {
        static constexpr UINT SPELLS_AMOUNT = 4;
        static constexpr LPCSTR creatureBankSpellsFormat = "RMG_CreatureBankId_%d_SpellId_%d";
        static constexpr LPCSTR hasCustomSetupFormat = "RMG_CreatureBankId_%d_hasCustomSetup";

        BOOL enabled = false;

        UINT experience;
        UINT8 primarySkills[SPELLS_AMOUNT];

        UINT spellPoints;

        INT luck;
        INT morale;
        // resources
        // artifacts
        struct SpellsReward
        {
            eSpell spellId = eSpell::NONE;
            eSpellchool spellSchool;
            DWORD spellLevels = 0;
            DWORD spellFlags = 0;
            BOOL generate = false;

        } spellsRewards[SPELLS_AMOUNT];

      public:
        void ReadStateFromJson(const INT16 creatureBankId, const UINT stateId) noexcept;
    };

    struct CreatureBank
    {
        UINT m_size;

        std::vector<int> monsterAwards;
        std::vector<std::array<int, GUARDES_AMOUNT>> monsterGuards;
        std::vector<H3CreatureBankSetup> setups;
        std::vector<std::array<UINT, STATES_AMOUNT>> mithrilAmount;
        std::vector<int> isNotBank;
        std::vector<std::array<int, 14>> customPositions;
        std::vector<std::array<CustomReward, STATES_AMOUNT>> customRewards;

      public:
        void CopyDefaultData(const size_t defaultSize);

    } creatureBanks;

    /*    static BOOL GetArmyMessage(const H3CreatureBank *creatureBank, H3String &customDescription,
                                   const bool withoutBrackets = true) noexcept;
       */ // static BOOL ShowMultiplePicsArmyMessage(const char *message, const int messageType, const int x, const int y,
    // H3Army *army) noexcept;

  private:
    CreatureBanksExtender();
    virtual ~CreatureBanksExtender();

  private:
    virtual void CreatePatches() override;
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes) override final;

  protected:
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept;

    // virtual void GetObjectPreperties() noexcept override  final;
  private:
    int GetBankSetupsNumberFromJson(const INT16 maxSubtype);
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;
    void Resize(UINT16 m_size) noexcept;
    void Reserve(UINT16 m_size) noexcept;
    void ShrinkToFit() noexcept;

  private:
    static _LHF_(CrBank_BeforeAddingToGameList);
    static _LHF_(CrBank_BeforeSetupFromState);

    static _LHF_(CrBank_DisplayPreCombatMessage);
    static _LHF_(CrBank_BeforeCombatStart);
    static _LHF_(SpecialCrBank_DisplayPreCombatMessage);

    static signed int __stdcall CrBank_CombatStart(HiHook *h, UINT AdvMan, UINT PisMixed, const H3Hero* attHero, UINT attArmy,
                                                   int PlayerIndex, UINT defTown, UINT defHero, UINT defArmy, int seed,
                                                   signed int a10, int isBank);
    static void __stdcall CrBank_AskForVisitMessage(HiHook *h, char *mes, const int messageType, const int x,
                                                    const int y, const int picType1, const int picSubtype1,
                                                    const int picType2, const int picSubtype2, const int par,
                                                    const int time, const int picType3, const int picSubtype3);

    static _LHF_(CrBank_AfterCombatWon);

    static _LHF_(CrBank_AfterDrawingResources);
    static _LHF_(CrBank_BeforeShowingRewardMessage);
    static int __stdcall CrBank_BeforeEndingText(HiHook *h, H3String *mes, const size_t len, const DWORD a3,
                                                 const DWORD a4) noexcept;

    static _LHF_(CrBank_BeforeGivingResources);
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *event);
    //	static int __stdcall CretureBankSetups__Ctor(HiHook*h);
  public:
    UINT Size() const noexcept;

  public:
    static int GetCreatureBankId(const int objType, const int objSubtype) noexcept;
    static eObject GetCreatureBankObjectType(const int cbId) noexcept;

    static CreatureBanksExtender &Get();
};

} // namespace cbanks
