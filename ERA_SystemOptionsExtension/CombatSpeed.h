#pragma once
#include "framework.h"

namespace cmbspd
{
class CombatSpeed : public IGamePatch
{
    static CombatSpeed *instance;
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

  private:
    CombatSpeed();
    virtual void CreatePatches() noexcept override;

  public:
    static CombatSpeed &Get();
};
} // namespace cmbspd
