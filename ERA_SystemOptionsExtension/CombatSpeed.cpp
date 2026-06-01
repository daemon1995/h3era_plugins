#include "CombatSpeed.h"
namespace cmbspd
{
CombatSpeed *CombatSpeed::instance = nullptr;

CombatSpeed::CombatSpeed() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.CombatSpeed.daemon_n"))
{
    CreatePatches();
}

void CombatSpeed::CreatePatches() noexcept
{
    if (m_isInited)
        return;

    m_isInited = true;

    const DWORD pBSpeed = (DWORD)&battleSpeedCoef;

    _pi->WriteDword(0x43F255 + 3, pBSpeed); // Battle_Stack_DrawShot_Bullet
    _pi->WriteDword(0x43F392 + 3, pBSpeed); // Battle_Stack_DrawShot_Bullet
    _pi->WriteDword(0x441B2A + 3, pBSpeed); // BattleStack_AttackMelle
    _pi->WriteDword(0x441BDC + 3, pBSpeed); // BattleStack_AttackMelle
    _pi->WriteDword(0x4466F4 + 3, pBSpeed); // BattleStack_DrawMoving
    _pi->WriteDword(0x44670B + 3, pBSpeed); // BattleStack_DrawMoving
    _pi->WriteDword(0x466CD4 + 3, pBSpeed); // BattleMgr_RemoveDead
    _pi->WriteDword(0x467758 + 3, pBSpeed); // BattleMgr_MaybeFlyingArrow
    _pi->WriteDword(0x467BCA + 3, pBSpeed); // BattleMgr_DrawMagicMissileMoving
    _pi->WriteDword(0x468093 + 3, pBSpeed); // BattleMgr_Unk
    _pi->WriteDword(0x473997 + 3, pBSpeed); // BattleMgr_Unk
    _pi->WriteDword(0x473A49 + 3, pBSpeed); // BattleMgr_Proc
    _pi->WriteDword(0x494662 + 3, pBSpeed); // BattleMgr_AnimationStep
    _pi->WriteDword(0x4B49AC + 3, pBSpeed); // BattleStack_0x4B47A0
    _pi->WriteDword(0x5A601C + 3, pBSpeed); // BattleStack_RayShooting
    _pi->WriteDword(0x5A6813 + 3, pBSpeed); // BattleStack_0x5A6670
    _pi->WriteDword(0x5A7FE2 + 3, pBSpeed); // BattleStack_CastSpellEarthquake
    _pi->WriteDword(0x5A8148 + 3, pBSpeed); // BattleStack_CastSpellEarthquake

    _pi->WriteDword(0x4023D3 + 6, speedsCount - 1); // set gosolo speed as 9

    _pi->WriteByte(0x50B556 + 2, speedsCount - 1); // NormalizeRegistry ( if ( BattleSpeed < 0 || BattleSpeed > 2->9 ))
}

CombatSpeed &CombatSpeed::Get()
{
    if (instance == nullptr)
        instance = new CombatSpeed();
    return *instance;
}
} // namespace cmbspd
