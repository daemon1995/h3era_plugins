#pragma once
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

    static struct QuickCombatInfo
    {
        int quickCombat = 0;
        int autoSpells = 0;
        int isNeedRestore = 0;
        int lastSelection = 0;
    } quickCombatInfo;

  private:
    CombatSettings();
    virtual void CreatePatches() noexcept override;

  protected:
    static _ERH_(OnBeforeBattleUniversal_Quit);
    static _ERH_(OnAfterBattle);

  public:
    static CombatSettings &Get();
};
} // namespace cmbsttngs
