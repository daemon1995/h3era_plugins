#include "CombatSettings.h"
#include <algorithm>
#include <cstring>
namespace cmbsttngs
{
CombatSettings *CombatSettings::instance = nullptr;
CombatSettings::QuickCombatInfo CombatSettings::quickCombatInfo = {};

CombatSettings::CombatSettings() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.CombatSettings.daemon_n"))
{
    CreatePatches();
}

_ERH_(CombatSettings::OnBeforeBattleUniversal)
{
    const auto attacker = ValueAt<H3Hero *>(0x2860248);
    ConfigureBattle(OriginalConfig::Get(), P_Game->Get(), attacker ? attacker->owner : -1,
                    IntAt(0x2846BC0), P_AutoSolo, IntAt(0x69959C) != 0);
}

void CombatSettings::ConfigureBattle(OriginalConfig &config, const H3Main *main, const int attackerPlayerId,
                                     const int defenderPlayerId, const bool autoSolo, const bool networkGame)
{

    enum eQuickCombatType : int
    {
        eQuickCombatType_Off = 0,
        eQuickCombatType_QuickCombatWithAutoSpells = 1,
        eQuickCombatType_QuickCombatWithoutAutoSpells = 2,
        eQuickCombatType_Ask = 3
    };

    const auto &extraConfig = AdditionalConfig::Get().quickCombatType;
    // The selected mode is authoritative; the native flag can be temporary.
    quickCombatInfo.SetPersistentQuickCombat(config.quickCombat, extraConfig.value > 0);

    if (autoSolo)
    {
        quickCombatInfo.SetQuickCombat(config.quickCombat, TRUE);
        return;
    }

    if (!main)
        return;

    if (main->inTutorial)
    {
        quickCombatInfo.SetQuickCombat(config.quickCombat, FALSE);
        return;
    }

    const int playerCount = static_cast<int>(std::size(main->players));
    if (attackerPlayerId < -1 || attackerPlayerId >= playerCount ||
        defenderPlayerId < -1 || defenderPlayerId >= playerCount)
        return;

    const bool humanAttacker = attackerPlayerId >= 0 && main->players[attackerPlayerId].isHuman;
    const bool humanDefender = defenderPlayerId >= 0 && main->players[defenderPlayerId].isHuman;
    if (humanAttacker && humanDefender)
    {
        quickCombatInfo.SetQuickCombat(config.quickCombat, FALSE);
        return;
    }
    if (!humanAttacker && !humanDefender)
        return;

    const int humanPlayerId = humanAttacker ? attackerPlayerId : defenderPlayerId;
    if (networkGame && !main->players[humanPlayerId].isLocal)
        return;

    int quickCombatType = std::clamp(extraConfig.value, int(eQuickCombatType_Off), int(eQuickCombatType_Ask));
    if (quickCombatType == eQuickCombatType_Ask)
    {
        quickCombatType = AskQuickCombatType();
        if (quickCombatType < 0)
            return; // A menu callback abandoned this game context.
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

int CombatSettings::AskQuickCombatType()
{
    LPCSTR keys[] = {"era.opt.map.quickCombat.menu", "era.opt.map.quickCombatManual.menu",
                     "era.opt.map.quickCombatMana.menu", "era.opt.map.quickCombatManaFree.menu"};
    Era::TErmZVar storedZ[4];
    const int storedV1 = Era::v[1];
    const int storedY1 = Era::y[1];
    const auto generation = quickCombatInfo.generation;
    for (size_t i = 0; i < std::size(keys); ++i)
        std::memcpy(storedZ[i], Era::z[i + 1], sizeof(storedZ[i]));

    int selectionMask = 0;
    __try
    {
        for (size_t i = 0; i < std::size(keys); ++i)
        {
            const auto text = EraJS::read(keys[i]);
            std::strncpy(Era::z[i + 1], text ? text : keys[i], sizeof(Era::z[i + 1]) - 1);
            Era::z[i + 1][sizeof(Era::z[i + 1]) - 1] = '\0';
        }
        Era::v[1] = 0; // A cancelled menu must not consume the caller's old v1.
        Era::y[1] = 1 << std::clamp(quickCombatInfo.lastSelection, 0, 2);
        Era::ExecErmCmd("IF:G1/1/y1/1/2/3/4");
        selectionMask = Era::v[1];
    }
    __finally
    {
        // A loaded game owns its ERM variables; an old frame must not edit them.
        if (generation == quickCombatInfo.generation)
        {
            Era::v[1] = storedV1;
            Era::y[1] = storedY1;
            for (size_t i = 0; i < std::size(keys); ++i)
                std::memcpy(Era::z[i + 1], storedZ[i], sizeof(storedZ[i]));
        }
    }
    if (generation != quickCombatInfo.generation)
        return -1;
    // IF:G returns one bit per item, including in radio mode. Decode the
    // selected bit; the third item returns 4, not its ordinal number 3.
    switch (selectionMask)
    {
    case 1 << 0:
        quickCombatInfo.lastSelection = 0;
        break;
    case 1 << 1:
        quickCombatInfo.lastSelection = 1;
        break;
    case 1 << 2:
        quickCombatInfo.lastSelection = 2;
        break;
    default:
        return 0; // Cancel or invalid output: enter manual combat.
    }
    return quickCombatInfo.lastSelection;
}

_ERH_(CombatSettings::OnAfterBattle)
{
    RestoreQuickCombatOptions();
}

_ERH_(CombatSettings::OnGameContextChanged)
{
    ResetGameContext(OriginalConfig::Get());
}

void CombatSettings::ResetGameContext(OriginalConfig &config) noexcept
{
    ++quickCombatInfo.generation;
    RestoreQuickCombatOptions(config);
}

void CombatSettings::RestoreQuickCombatOptions() noexcept
{
    RestoreQuickCombatOptions(OriginalConfig::Get());
}

void CombatSettings::RestoreQuickCombatOptions(OriginalConfig &config) noexcept
{
    // Restore only fields temporarily overridden for this battle. A user can
    // change an untouched option in the manual battle's settings dialog.
    quickCombatInfo.Restore(config.quickCombat, config.autoSpells);
    quickCombatInfo.SetPersistentQuickCombat(config.quickCombat, AdditionalConfig::Get().quickCombatType.value > 0);
}

int __stdcall CombatSettings::AdventureManager_Battle(HiHook *hook, H3AdventureManager *manager, int point,
                                                       H3Hero *attacker, H3Army *attackerArmy, int defenderOwner,
                                                       H3Town *town, H3Hero *defender, H3Army *defenderArmy,
                                                       int seed, int combatIsLocal, int isBank)
{
    // Include pre-battle events: they can cancel the attack after selecting a
    // mode, without ever firing OnAfterBattle.
    RestoreQuickCombatOptions();
    const auto generation = quickCombatInfo.generation;
    int result = 0;
    __try
    {
        result = THISCALL_11(int, hook->GetDefaultFunc(), manager, point, attacker, attackerArmy, defenderOwner,
                             town, defender, defenderArmy, seed, combatIsLocal, isBank);
    }
    __finally
    {
        if (generation == quickCombatInfo.generation)
            RestoreQuickCombatOptions();
    }
    return result;
}

LONG __stdcall CombatSettings::WriteGameSettings(HiHook *hook)
{
    auto &config = OriginalConfig::Get();
    ApplyQuickCombatType(AdditionalConfig::Get().quickCombatType, AdditionalConfig::EOptionChangeSource::InitialLoad);
    quickCombatInfo.BeginSettingsWrite(config.quickCombat, config.autoSpells);
    LONG result = 0;
    __try
    {
        result = CDECL_0(LONG, hook->GetDefaultFunc());
    }
    __finally
    {
        quickCombatInfo.EndSettingsWrite(config.quickCombat, config.autoSpells);
    }
    return result;
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

    if (combatManager && msg && msg->IsKeyDown() &&
        bool(combatManager->isHuman[0]) != bool(combatManager->isHuman[1]))
    {
        if (msg->GetKey() == eVKey::H3VK_L && !IntAt(0x69959C))
        {
            // Match the battle hotkey behavior: Ctrl+L skips confirmation, L asks first.
            if (msg->CtrlPressed() || H3Messagebox::Choice(P_GeneralText->GetText(69)))
                Era::FastQuitToGameMenu(Era::EGameMenuTarget::PAGE_LOAD_GAME);
            return TRUE; // prevent the game's default handling, even if the dialog was declined
        }

        if (msg->GetKey() == eVKey::H3VK_Q)
        {
            LPCSTR question = EraJS::read("era.opt.combat.autoQuick.question");
            if (H3Messagebox::Choice(question))
            {
                FinishBattleInstantly();
                return TRUE;
            }
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
    _pi->WriteHiHook(0x473A00, SPLICE_, EXTENDED_, THISCALL_, CombatManager_ProcessMessage);
    _pi->WriteLoHook(0x47478A, CombatManager_AutoCombatButton);
    _pi->WriteLoHook(0x476DA5, CombatManager_EndBattle);

    // combat type selection
    _pi->WriteHiHook(0x75ADD9, SPLICE_, EXTENDED_, THISCALL_, AdventureManager_Battle);
    // ERA redirects both native write entry points to this same procedure.
    const _ptr_ writeSettings = ByteAt(0x50C370) == 0xE9 ? 0x50C375 + IntAt(0x50C371) : 0x50C370;
    _pi->WriteHiHook(writeSettings, SPLICE_, EXTENDED_, CDECL_, WriteGameSettings);
    _REH_(OnBeforeBattleUniversal);
    _REH_(OnAfterBattle);
    Era::RegisterHandler(OnGameContextChanged, "OnBeforeFastQuitToGameMenu");
    Era::RegisterHandler(OnGameContextChanged, "OnBeforeLoadGame");
    Era::RegisterHandler(OnGameContextChanged, "OnAfterLoadGame");
    Era::RegisterHandler(OnGameContextChanged, "OnGameLeave");
    Era::RegisterHandler(OnGameContextChanged, "OnGameLeft");
    Era::RegisterHandler(OnGameContextChanged, "OnGameEnter");
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
    quickCombatInfo.SetPersistentQuickCombat(OriginalConfig::Get().quickCombat, entry.value > 0);
}

int CombatSettings::PersistentQuickCombat() noexcept
{
    return AdditionalConfig::Get().quickCombatType.value > 0;
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
