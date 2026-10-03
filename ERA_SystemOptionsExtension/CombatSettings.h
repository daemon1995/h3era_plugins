#pragma once
#include "QuickCombatState.h"
#include "framework.h"

namespace cmbsttngs
{
class CombatSettings : public IGamePatch
{
    static CombatSettings *instance;
    static constexpr DWORD speedsCount = 10;
    // * new battle speed coefficients from SOD_SP
    static constexpr float battleSpeedCoef[speedsCount] = {
        1.000f, 0.630f, 0.400f, // original speed coefficients
        0.300f,                 // new speed coefficient
        0.200f, 0.100f,         // old SoD_SP turbo speed coefficients
        0.075f,                 // new speed coefficient
        0.050f,                 // old SoD_SP turbo speed coefficient
        0.025f, 0.010f          // new speed coefficients
    };

    static struct QuickCombatInfo : QuickCombatState
    {
        int lastSelection = 0;
    } quickCombatInfo;

  private:
    CombatSettings();
    virtual void CreatePatches() noexcept override;

  protected:
    static _ERH_(OnBeforeBattleUniversal_Quit);
    static _ERH_(OnAfterBattleOrFastQuit);
    static int __stdcall CombatManager_ProcessMessage(HiHook *hook, H3CombatManager *combatManager, H3Msg *msg);
    static _LHF_(CombatManager_AutoCombatButton);
    static _LHF_(CombatManager_EndBattle);
    static void FinishBattleInstantly() noexcept;
    static void RestoreQuickCombatOptions() noexcept;

  public:
    static CombatSettings &Get();
    static int PersistentAutoSpells() noexcept;
    static void SetPersistentAutoSpells(int value) noexcept;
    static void ApplyQuickCombatType(const AdditionalConfig::ConfigEntry &entry,
                                     AdditionalConfig::EOptionChangeSource source) noexcept;
};
} // namespace cmbsttngs
