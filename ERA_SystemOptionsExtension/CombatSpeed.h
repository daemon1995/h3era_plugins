#pragma once
#include "framework.h"

namespace cmbspd
{
class CombatSpeed : public IGamePatch
{
    static CombatSpeed *instance;
	static float battleSpeedCoef[10];
  private:
    CombatSpeed();
    virtual void CreatePatches() noexcept override;
  public:
    // static void Init();
    static CombatSpeed &Get(); // (PatcherInstance* _PI);
};
} // namespace cmbspd
