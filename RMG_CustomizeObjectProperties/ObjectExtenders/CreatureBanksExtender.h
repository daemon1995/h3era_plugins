#pragma once
#include "..\pch.h"
#include "CreatureBankManager.h"

#include <array>
struct ObjectExtenderDesc;
namespace cbanks
{

class CreatureBanksExtender : public extender::ObjectExtender
{
    struct Current
    {
        INT type = -1;
        INT stateId = -1;
        H3CreatureBank *bank = nullptr;
        const H3MapItem *mapItem = nullptr;
        H3Hero *hero = nullptr;
        Patch *positionsPatch = nullptr;
        LPCSTR customDefName = nullptr;

        std::array<eSpell, SPELLS_AMOUNT> spellsToLearn = {eSpell::NONE, eSpell::NONE, eSpell::NONE, eSpell::NONE};
        UINT mithrilToAdd = 0;
        UINT spellPointsToAdd = 0;
        UINT experiencePointsToAdd = 0;
        H3String message;
    };

    static CreatureBanksExtender *instance;
    static Current currentCreatureBank;

  private:
    UINT defaultBanksNumber = 0;
    UINT addedBanksNumber = 0;

    CreatureBankManager manager;
    ObjectExtenderDesc api;

  private:
    CreatureBanksExtender();
    virtual ~CreatureBanksExtender();

  protected:
    virtual void CreatePatches() override;
    virtual void AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept override;

    // virtual void GetObjectPreperties() noexcept override  final;
  private:
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

    static _LHF_(AIHero_GetCreatureBankItemWeight);
    static _LHF_(CrBank_DisplayPreCombatMessage);
    static _LHF_(SpecialCrBank_DisplayPreCombatMessage);
    static void __stdcall CrBank_AskForVisitMessage(HiHook *h, char *mes, const int messageType, const int x,
                                                    const int y, const int picType1, const int picSubtype1,
                                                    const int picType2, const int picSubtype2, const int par,
                                                    const int time, const int picType3, const int picSubtype3);

    static signed int __stdcall CrBank_CombatAndRewardProc(HiHook *h, H3AdventureManager *advMan, const H3Hero *attHero,
                                                           const H3MapItem *mapItem, LPCSTR text, const UINT PisMixed,
                                                           const BOOL isPlayer);
    static signed int __stdcall CrBank_CombatStart(HiHook *h, H3AdventureManager *advMan, UINT PisMixed,
                                                   const H3Hero *attHero, UINT attArmy, int PlayerIndex, UINT defTown,
                                                   UINT defHero, UINT defArmy, int seed, signed int a10, int isBank);
    static _LHF_(CrBank_AfterCombatWon);
    static _LHF_(CrBank_AfterDrawingResources);
    static _LHF_(CrBank_BeforeShowingRewardMessage);
    static H3String *__cdecl CrBank_AwardMessageFormatReadingFromTxt(HiHook *h, char *buffer, const char *textFormat,
                                                                     const char *creatureNames,
                                                                     const char *rewardText) noexcept;
    static _LHF_(CrBank_BeforeGivingResources);

    // save/load data management
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

    static CreatureBanksExtender &Get();
};

} // namespace cbanks
