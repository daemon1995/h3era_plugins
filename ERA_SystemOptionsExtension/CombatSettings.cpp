#include "CombatSettings.h"
#include <cstring>
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

    enum eQuickCombatType : int
    {
        eQuickCombatType_Off = 0,
        eQuickCombatType_QuickCombatWithAutoSpells = 1,
        eQuickCombatType_QuickCombatWithoutAutoSpells = 2,
        eQuickCombatType_Ask = 3
    };
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
    quickCombatInfo.restoreQuickCombat = FALSE;
    quickCombatInfo.restoreAutoSpells = FALSE;

    if (P_AutoSolo)
    {
        config.quickCombat = true;
        quickCombatInfo.restoreQuickCombat = config.quickCombat != quickCombatInfo.quickCombat;
        return;
    }

    auto &extraConfig = AdditionalConfig::Get().quickCombatType;
    int quickCombatType = Clamp(0, extraConfig.value, extraConfig.maxValue);

    if (config.quickCombat && !quickCombatType)
    {
        quickCombatType = eQuickCombatType_QuickCombatWithoutAutoSpells - bool(config.autoSpells);
    }
    else if (!config.quickCombat && quickCombatType)
    {
        quickCombatType = eQuickCombatType_Off;
    }

    if (quickCombatType == eQuickCombatType_Ask)
    {
        LPCSTR keys[] = {"era.opt.map.quickCombat.menu", "era.opt.map.quickCombatManual.menu",
                         "era.opt.map.quickCombatMana.menu", "era.opt.map.quickCombatManaFree.menu"};
        Era::TErmZVar storedZ[4];
        const int storedV1 = Era::v[1];
        const int storedY1 = Era::y[1];
        for (size_t i = 0; i < 4; i++)
        {
            std::memcpy(storedZ[i], Era::z[i + 1], sizeof(storedZ[i]));
            std::strncpy(Era::z[i + 1], EraJS::read(keys[i]), sizeof(Era::z[i + 1]) - 1);
            Era::z[i + 1][sizeof(Era::z[i + 1]) - 1] = '\0';
        }
        Era::y[1] = 1 << quickCombatInfo.lastSelection;
        Era::ExecErmCmd("IF:G1/1/y1/1/2/3/4");
        quickCombatType = Clamp(1, Era::v[1], 3) - 1;
        Era::v[1] = storedV1;
        Era::y[1] = storedY1;
        for (size_t i = 0; i < 4; i++)
            std::memcpy(Era::z[i + 1], storedZ[i], sizeof(storedZ[i]));
        quickCombatInfo.lastSelection = quickCombatType;
    }

    switch (quickCombatType)
    {

    case eQuickCombatType_Off:
        config.quickCombat = false;
        break;
    case eQuickCombatType_QuickCombatWithAutoSpells:
        config.quickCombat = true;
        config.autoSpells = true;
        break;
    case eQuickCombatType_QuickCombatWithoutAutoSpells:
        config.quickCombat = true;
        config.autoSpells = false;
        break;
    default:
        break;
    }

    quickCombatInfo.restoreQuickCombat = config.quickCombat != quickCombatInfo.quickCombat;
    quickCombatInfo.restoreAutoSpells = config.autoSpells != quickCombatInfo.autoSpells;
}

// restore quick combat settings after battle or before FastQuit discards the game
_ERH_(CombatSettings::OnAfterBattleOrFastQuit)
{
    if (!quickCombatInfo.restoreQuickCombat && !quickCombatInfo.restoreAutoSpells)
        return;

    auto &config = OriginalConfig::Get();
    // Restore only fields temporarily overridden for this battle. A user can
    // change an untouched option in the manual battle's settings dialog.
    if (quickCombatInfo.restoreQuickCombat)
        config.quickCombat = quickCombatInfo.quickCombat;
    if (quickCombatInfo.restoreAutoSpells)
        config.autoSpells = quickCombatInfo.autoSpells;
    quickCombatInfo.restoreQuickCombat = FALSE;
    quickCombatInfo.restoreAutoSpells = FALSE;
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
    Era::RegisterHandler(OnAfterBattleOrFastQuit, "OnAfterBattle");
    Era::RegisterHandler(OnAfterBattleOrFastQuit, "OnBeforeFastQuitToGameMenu");
}

CombatSettings &CombatSettings::Get()
{
    if (instance == nullptr)
        instance = new CombatSettings();
    return *instance;
}

void CombatSettings::ApplyQuickCombatType(const AdditionalConfig::ConfigEntry &entry,
                                          const AdditionalConfig::EOptionChangeSource source) noexcept
{
    // A zero value from the initial load preserves the game's original quick-combat state.
    // A zero value received from the API or the dialog explicitly disables it.
    if (source == AdditionalConfig::EOptionChangeSource::InitialLoad && entry.value == 0)
        return;

    OriginalConfig::Get().quickCombat = entry.value != 0;
}
} // namespace cmbsttngs
