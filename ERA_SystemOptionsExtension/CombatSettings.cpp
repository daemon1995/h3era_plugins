#include "CombatSettings.h"
namespace cmbsttngs
{
CombatSettings *CombatSettings::instance = nullptr;
CombatSettings::QuickCombatInfo CombatSettings::quickCombatInfo = {};

CombatSettings::CombatSettings() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.CombatSettings.daemon_n"))
{
    CreatePatches();
}

_ERH_(CombatSettings::OnBeforeBattleUniversal_Quit)
{
    auto cmb = P_CombatManager->Get();
    // if (!cmb || cmb->isHuman[0] == cmb->isHuman[1] || P_Game->inTutorial)
    //    return;

    LPCSTR varNames[] = {"battle_humanOnly", "battle_isNetwork", "battle_aiOnly"};

    for (auto &varName : varNames)
    {
        if (Era::GetAssocVarIntValue(varName))
        {
            return;
        }
    }

    auto &config = OriginalConfig::Get();
    quickCombatInfo.quickCombat = config.quickCombat;
    quickCombatInfo.autoSpells = config.autoSpells;
    quickCombatInfo.isNeedRestore = false;

    if (P_AutoSolo)
    {
        config.quickCombat = true;
        quickCombatInfo.isNeedRestore = true;
        return;
    }

    auto &extraConfig = AdditionalConfig::Get();
    int quickCombatType = extraConfig.quickCombatType.value;

    BOOL hdModConflict = false;
    if (auto hdWog = globalPatcher->GetInstance("HD.WoG"))
    {
        BOOL hdModQuick = globalPatcher->VarGetValue<int>("HD.QuickCombat", 0);
        if (!hdModQuick && quickCombatType)
        {
            quickCombatType = 0;
        }
        else if (hdModQuick && !quickCombatType)
        {
            quickCombatType = 3;
        }
    }
    if (quickCombatType == 3)
    {
        LPCSTR keys[] = {"era.opt.map.quickCombat.menu", "era.opt.map.quickCombatManual.menu",
                         "era.opt.map.quickCombatMana.menu", "era.opt.map.quickCombatManaFree.menu"};
        for (size_t i = 1; i <= 4; i++)
        {
            libc::sprintf(Era::z[i], "%s", EraJS::read(keys[i - 1]));
        }
        const int storedY1 = Era::y[1];
        Era::y[1] = quickCombatInfo.lastSelection + 1;
        Era::ExecErmCmd("IF:G1/1/y1/1/2/3/4");
        Era::y[1] = storedY1;
        quickCombatType = Clamp(0, Era::v[1] - 1, 2); // get quick combat type from registry (0..2)
        quickCombatInfo.lastSelection = quickCombatType;
    }
    else if (quickCombatType == 0 && globalPatcher->VarValue<int>("HD.QuickCombat"))
    {
        quickCombatType = 2 - (config.autoSpells);
    }

    switch (quickCombatType)
    {

    case 0:
        config.quickCombat = false;
        break;
    case 1:
        config.quickCombat = true;
        config.autoSpells = true;
        break;
    case 2:
        config.quickCombat = true;
        config.autoSpells = false;
        break;
    default:
        break;
    }

    quickCombatInfo.isNeedRestore =
        config.quickCombat != quickCombatInfo.quickCombat || config.autoSpells != quickCombatInfo.autoSpells;
}

// restore quick combat settings after battle
_ERH_(CombatSettings::OnAfterBattle)
{
    if (!quickCombatInfo.isNeedRestore)
        return;

    auto &config = OriginalConfig::Get();
    config.quickCombat = quickCombatInfo.quickCombat;
    config.autoSpells = quickCombatInfo.autoSpells;
    // quickCombatInfo = {};
}
void CombatSettings::CreatePatches() noexcept
{
    if (m_isInited)
        return;

    m_isInited = true;

    // additional combat speed
    // idea @MoP, code @igrik
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

    // combat setype selection
    _REH_(OnBeforeBattleUniversal_Quit);
    _REH_(OnAfterBattle);
}

CombatSettings &CombatSettings::Get()
{
    if (instance == nullptr)
        instance = new CombatSettings();
    return *instance;
}
} // namespace cmbsttngs
