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
        unsigned int generation = 0;
    } quickCombatInfo;

  private:
    CombatSettings();
    virtual void CreatePatches() noexcept override;

  protected:
    static _ERH_(OnBeforeBattleUniversal);
    static _ERH_(OnAfterBattle);
    static _ERH_(OnBattleReplay);
    static _ERH_(OnGameContextChanged);
    static void ConfigureBattle(OriginalConfig &config, const H3Main *main, int attackerPlayerId, int defenderPlayerId,
                                bool autoSolo, bool networkGame);
    static void ResetGameContext(OriginalConfig &config) noexcept;
    static int AskQuickCombatType();
    static int __stdcall AdventureManager_Battle(HiHook *hook, H3AdventureManager *manager, int point, H3Hero *attacker,
                                                 H3Army *attackerArmy, int defenderOwner, H3Town *town,
                                                 H3Hero *defender, H3Army *defenderArmy, int seed, int combatIsLocal,
                                                 int isBank);
    static LONG __stdcall WriteGameSettings(HiHook *hook);
    static int __stdcall CombatManager_ProcessMessage(HiHook *hook, H3CombatManager *combatManager, H3Msg *msg);
    static int __stdcall CombatManager_DrawFizzleAtStackSummon(HiHook *hook, H3WindowManager *windowManager, int x,
                                                               int y, int width, int height, int drawTime);
    static _LHF_(CombatManager_AutoCombatButton);
    static _LHF_(CombatManager_EndBattle);
    static void FinishBattleInstantly() noexcept;
    static bool CanFinishBattleInstantly(const H3CombatManager *combat, const H3Main *main) noexcept;
    static void RestoreQuickCombatOptions() noexcept;
    static void RestoreQuickCombatOptions(OriginalConfig &config) noexcept;

  public:
    static CombatSettings &Get();
    static int PersistentQuickCombat() noexcept;
    static int PersistentAutoSpells() noexcept;
    static void SetPersistentAutoSpells(int value) noexcept;
    static void ApplyQuickCombatType(const AdditionalConfig::ConfigEntry &entry,
                                     AdditionalConfig::EOptionChangeSource source) noexcept;
};
} // namespace cmbsttngs
