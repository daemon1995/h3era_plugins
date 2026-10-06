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
    if (P_AutoSolo)
    {
        quickCombatInfo.SetQuickCombat(config.quickCombat, TRUE);
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
        quickCombatInfo.SetQuickCombat(config.quickCombat, FALSE);
        break;
    case eQuickCombatType_QuickCombatWithAutoSpells:
        quickCombatInfo.SetQuickCombat(config.quickCombat, TRUE);
        quickCombatInfo.SetAutoSpells(config.autoSpells, TRUE);
        break;
    case eQuickCombatType_QuickCombatWithoutAutoSpells:
        quickCombatInfo.SetQuickCombat(config.quickCombat, TRUE);
        quickCombatInfo.SetAutoSpells(config.autoSpells, FALSE);
        break;
    default:
        break;
    }
}

// restore quick combat settings after battle or before FastQuit discards the game
_ERH_(CombatSettings::OnAfterBattleOrFastQuit)
{
    RestoreQuickCombatOptions();
}

void CombatSettings::RestoreQuickCombatOptions() noexcept
{
    auto &config = OriginalConfig::Get();
    // Restore only fields temporarily overridden for this battle. A user can
    // change an untouched option in the manual battle's settings dialog.
    quickCombatInfo.Restore(config.quickCombat, config.autoSpells);
}

void CombatSettings::FinishBattleInstantly() noexcept
{
    // Clear the cursor's combat shadows before switching to hidden combat.
    CDECL_0(int, 0x493EF0);
    auto &config = OriginalConfig::Get();
    quickCombatInfo.FinishInstantly(config.quickCombat, config.autoSpells);
}

int __stdcall CombatSettings::CombatManager_ProcessMessage(HiHook *hook, H3CombatManager *combatManager, H3Msg *msg)
{
    if (msg && msg->IsKeyDown() && msg->GetKey() == eVKey::H3VK_Q &&
        bool(combatManager->isHuman[0]) != bool(combatManager->isHuman[1]))
    {
        bool translated = false;
        LPCSTR question = EraJS::read("era.opt.combat.autoQuick.question", translated);
        if (!translated)
            question = EraJS::read("wnd.combat.finish_question", translated);
        if (!translated)
            question = "Finish with Quick Combat?";

        if (H3Messagebox::Choice(question))
        {
            FinishBattleInstantly();
            return TRUE;
        }
    }
    return THISCALL_2(int, hook->GetDefaultFunc(), combatManager, msg);
}

int __stdcall CombatSettings::CombatManager_DrawFizzleAtStackSummon(HiHook *hook, H3WindowManager *windowManager, int x,
                                                                    int y, int width, int height, int drawTime)
{
    auto &config = OriginalConfig::Get();
    constexpr int maxSpeedIndex = static_cast<int>(std::size(battleSpeedCoef)) - 1;
    const int speedIndex = Clamp(0, config.animationSpeed, maxSpeedIndex);
    if (battleSpeedCoef[speedIndex] < 1.0f)
    {
        drawTime = static_cast<int>(static_cast<float>(drawTime) * 1.5f * battleSpeedCoef[speedIndex]);
    }

    return THISCALL_6(int, hook->GetDefaultFunc(), windowManager, x, y, width, height, drawTime);
}

_LHF_(CombatSettings::CombatManager_AutoCombatButton)
{
    const auto combatManager = reinterpret_cast<H3CombatManager *>(c->ebx);
    if (AdditionalConfig::Get().quickAutoResolve.value && !combatManager->autoCombat &&
        bool(combatManager->isHuman[0]) != bool(combatManager->isHuman[1]))
    {
        FinishBattleInstantly();
        // The game has accepted button 2004 (or its A hotkey). Return TRUE
        // without starting the visible auto-combat action for the current stack.
        c->return_address = 0x47480F;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(CombatSettings::CombatManager_EndBattle)
{
    // WND restored its instant-finish override before the result dialog. Keep
    // regular battle-type restoration at OnAfterBattle, as before.
    auto &config = OriginalConfig::Get();
    quickCombatInfo.RestoreInstantFinish(config.quickCombat, config.autoSpells);
    return EXEC_DEFAULT;
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

    _pi->WriteHiHook(0x479BED, CALL_, EXTENDED_, THISCALL_, CombatManager_DrawFizzleAtStackSummon);

    _pi->WriteDword(0x4023D3 + 6, speedsCount - 1); // set gosolo speed as 9

    _pi->WriteByte(0x50B556 + 2, speedsCount - 1); // NormalizeRegistry ( if ( BattleSpeed < 0 || BattleSpeed > 2->9 ))

    // Q and the optional instant auto-combat button share WND's quick finish.
    _pi->WriteHiHook(0x473F55, CALL_, EXTENDED_, THISCALL_, CombatManager_ProcessMessage);
    _pi->WriteLoHook(0x47478A, CombatManager_AutoCombatButton);
    _pi->WriteLoHook(0x476DA5, CombatManager_EndBattle);

    // combat type selection
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
    OriginalConfig::Get().quickCombat = entry.value != 0;
}

int CombatSettings::PersistentAutoSpells() noexcept
{
    return quickCombatInfo.PersistentAutoSpells(OriginalConfig::Get().autoSpells);
}
void CombatSettings::SetPersistentAutoSpells(const int value) noexcept
{
    quickCombatInfo.SetPersistentAutoSpells(OriginalConfig::Get().autoSpells, value);
}
} // namespace cmbsttngs
