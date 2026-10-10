// Based on igrik's BattleReplay.cpp (h3era_plugins_by_igrik/all).
// See LICENSE for the original repository license.
#define _H3API_PLUGINS_
#include "BattleState.h"
#include "framework.h"
#include <cstddef>
#include <cstring>

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
static_assert(sizeof(H3Army) == 56 && sizeof(WoG::NPC) == 296, "Unexpected battle data layout");

namespace battleReplay
{
template <> inline void CopyObject(H3Hero &target, const H3Hero &source)
{
    // H3Hero contains one owning string. Never memcpy that string's pointers.
    // Also avoid H3Hero's implicit assignment: this H3API revision has a broken
    // H3Bitset<48>::operator= (visitedTowns).
    constexpr size_t biographyOffset = offsetof(H3Hero, biography);
    constexpr size_t tailOffset = biographyOffset + sizeof(H3String);
    static_assert(sizeof(H3Hero) == 0x492 && biographyOffset == 0x3DA, "Unexpected hero layout");
    std::memcpy(&target, &source, biographyOffset);
    target.biography.Assign(source.biography.String() ? source.biography.String() : "", source.biography.Length());
    std::memcpy(reinterpret_cast<char *>(&target) + tailOffset, reinterpret_cast<const char *>(&source) + tailOffset,
                sizeof(H3Hero) - tailOffset);
}
} // namespace battleReplay

namespace
{
constexpr char INSTANCE_NAME[] = "igrik.BattleReplay";
constexpr int REPLAY_BUTTON = 30723;

int &QuickBattle()
{
    return *reinterpret_cast<int *>(0x6987CC);
}

battleReplay::ReplayRuntime replayRuntime;
battleReplay::QuickBattleBackup<Variable> quickBattleBackup;

void __stdcall ResetReplayRuntime(Era::TEvent *)
{
    replayRuntime.Reset(); // Never touch the old stack context.
    quickBattleBackup.Restore(QuickBattle());
}

bool ReplayRequested()
{
    const auto context = replayRuntime.Current();
    return context && context->allowed && context->requested;
}

class GoldSnapshot
{
    H3Main *main_;
    int owner_;
    int gold_;

  public:
    GoldSnapshot(H3Main *main, int owner)
        : main_(main), owner_(owner),
          gold_(battleReplay::IsPlayer(owner) ? main->players[owner].playerResources.gold : 0)
    {
    }
    void Restore() const
    {
        if (battleReplay::IsPlayer(owner_))
            main_->players[owner_].playerResources.gold = gold_;
    }
};

WoG::NPC *SavedCommander(H3Hero *hero)
{
    WoG::NPC *npc = hero ? WoG::NPC::Get(hero->id) : nullptr;
    // Keep the original WoG eligibility condition; WoG addresses are not
    // verified against the executable's IDA export.
    return npc && npc->on == 1 && npc->alive != 1 ? npc : nullptr;
}

int __stdcall SkipExperience(LoHook *, HookContext *context)
{
    if (!ReplayRequested())
        return EXEC_DEFAULT;
    context->return_address = 0x477303;
    return NO_EXEC_DEFAULT;
}

int __stdcall SkipMapRedraw(LoHook *, HookContext *context)
{
    if (!ReplayRequested())
        return EXEC_DEFAULT;
    context->return_address = 0x41742D;
    return NO_EXEC_DEFAULT;
}

int __stdcall SkipAfterBattle(LoHook *, HookContext *context)
{
    if (!ReplayRequested())
        return EXEC_DEFAULT;
    // Original WoG replay path: leave world changes to the accepted attempt.
    // The original jump also bypassed these unconditional cleanup operations.
    // Frame offsets and globals are verified at 0x4AE58D..0x4AE632.
    IntAt(0x698A10) = IntAt(context->ebp - 0x30);                   // bShowIt
    IntAt(H3CurrentPlayerID::ADDRESS) = IntAt(context->ebp - 0x3C); // active player
    IntAt(0x699598) = 0;                                            // gAdvDisposeLevel
    IntAt(0x699590) = 0;                                            // battle in progress

    THISCALL_1(void, 0x558190, reinterpret_cast<void *>(0x69D680)); // ContinueTimeAfterBattle
    // ESI normally receives the return value in the skipped tail (0x4AE62C).
    context->esi = P_CombatManager->winnerSide;
    context->return_address = 0x4AE67C;
    return NO_EXEC_DEFAULT;
}

H3BaseDlg *__stdcall CreateResults(HiHook *hook, H3BaseDlg *dlg, H3Hero *attacker, H3Hero *defender, int mySide,
                                   int winningSide, int siege, int experience)
{
    H3BaseDlg *result = THISCALL_7(H3BaseDlg *, hook->GetDefaultFunc(), dlg, attacker, defender, mySide, winningSide,
                                   siege, experience);
    if (replayRuntime.Current() && replayRuntime.Current()->allowed)
    {
        dlg->AddItem(H3DlgPcx::Create(20, 506, 0, NH3Dlg::Assets::BOX_64_30_PCX));
        dlg->AddItem(H3DlgDefButton::Create(21, 507, 64, 30, REPLAY_BUTTON, NH3Dlg::Assets::CANCEL_BUTTON, 0, 1, FALSE,
                                            eVKey::H3VK_ESCAPE));
    }
    return result;
}

int __stdcall ProcessResults(HiHook *hook, H3Msg *msg)
{
    const auto context = replayRuntime.Current();
    if (context && context->allowed && msg->IsLeftClick() && msg->itemId == REPLAY_BUTTON)
    {
        context->requested = true;
        P_WindowManager->resultItemID = REPLAY_BUTTON; // Was an ineffective == expression.
        *reinterpret_cast<int *>(0x6977D4) = 0;
        msg->command = static_cast<eMsgCommand>(512);
        msg->subtype = static_cast<eMsgSubtype>(10);
        msg->itemId = 10;
        return 2;
    }
    // Do not clear requested on subsequent dialog notifications.
    return THISCALL_1(int, hook->GetDefaultFunc(), msg);
}

int __stdcall ReplayBattle(HiHook *hook, H3AdventureManager *advMgr, int position, H3Hero *attacker,
                           H3Army *attackerArmy, int defenderOwner, H3Town *town, H3Hero *defender,
                           H3Army *defenderArmy, int seed, int combatIsLocal, int isBank)
{
    H3Main *main = P_Main;
    const int attackerOwner = attacker ? attacker->owner : -1;
    const bool humanAttacker = battleReplay::IsPlayer(attackerOwner) && main->IsHuman(attackerOwner);
    const bool humanDefender = battleReplay::IsPlayer(defenderOwner) && main->IsHuman(defenderOwner);
    const bool nested = replayRuntime.Current() != nullptr;
    battleReplay::ReplayContext context;
    battleReplay::ReplayRuntime::Scope scope(replayRuntime, context);
    context.allowed = !nested && attackerArmy && defenderArmy &&
                      battleReplay::CanReplay(*reinterpret_cast<int *>(0x69959C) != 0, humanAttacker, humanDefender);

    if (!context.allowed)
        return THISCALL_11(int, hook->GetDefaultFunc(), advMgr, position, attacker, attackerArmy, defenderOwner, town,
                           defender, defenderArmy, seed, combatIsLocal, isBank);

    // Match the original 0x4175E0 call before taking the hero snapshot.
    if (attacker)
        THISCALL_3(void, 0x4175E0, advMgr, 0, 1);

    const battleReplay::SideSnapshot<H3Hero, H3Army> attackerState(attacker, attackerArmy);
    const battleReplay::SideSnapshot<H3Hero, H3Army> defenderState(defender, defenderArmy);
    const battleReplay::Snapshot<H3Army> townArmy(town ? &town->guards : nullptr);
    const battleReplay::Snapshot<WoG::NPC> attackerCommander(SavedCommander(attacker));
    const battleReplay::Snapshot<WoG::NPC> defenderCommander(SavedCommander(defender));
    const GoldSnapshot attackerGold(main, attackerOwner);
    const GoldSnapshot defenderGold(main, defenderOwner);
    const battleReplay::QuickBattleBackup<Variable>::Scope quickBattle(replayRuntime, quickBattleBackup, QuickBattle(),
                                                                       globalPatcher->VarFind("HD.QuickCombat"));

    int result = 0;
    do
    {
        if (context.requested)
        {
            attackerState.Restore();
            defenderState.Restore();
            townArmy.Restore();
            attackerCommander.Restore();
            defenderCommander.Restore();
            attackerGold.Restore();
            defenderGold.Restore();

            // WoG compatibility calls/variables retained from the original.
            CDECL_0(void, 0x75ACDD); // CheckForCompleteAI
            P_CombatManager->turn = 0;
            *reinterpret_cast<int *>(0x79F0B8) = -1;
            *reinterpret_cast<int *>(0x79F0BC) = -1;
            quickBattle.Disable();
            Era::FireEvent("OnBattleReplay", nullptr, 0);
            if (!scope.IsCurrent())
                break; // An event loaded another game; old combat arguments are invalid.
        }

        context.requested = false;
        result = THISCALL_11(int, hook->GetDefaultFunc(), advMgr, position, attacker, attackerArmy, defenderOwner, town,
                             defender, defenderArmy, seed, combatIsLocal, isBank);
        if (scope.IsCurrent() && context.requested)
            Era::FireEvent("OnBeforeBattleReplay", nullptr, 0);
    } while (scope.IsCurrent() && context.requested);
    return result;
}

void __stdcall AstralAfterBattle(HiHook *hook)
{
    if (!ReplayRequested())
        CDECL_0(void, hook->GetDefaultFunc());
}

void InstallHooks()
{
    _PI->WriteHiHook(0x46FE20, SPLICE_, EXTENDED_, THISCALL_, CreateResults);
    _PI->WriteHiHook(0x4716E0, SPLICE_, EXTENDED_, THISCALL_, ProcessResults);
    _PI->WriteLoHook(0x4173E2, SkipMapRedraw);
    _PI->WriteLoHook(0x4ADFE8, SkipAfterBattle);
    _PI->WriteLoHook(0x477254, SkipExperience);

    // Retain the WoG entry point, including the CALL_ SAFE_ ABI.
    _PI->WriteHexPatch(0x75AEB0, "E8 AB22D5FF 90 90");
    _PI->WriteHiHook(0x75AEB0, CALL_, SAFE_, THISCALL_, ReplayBattle);
    _PI->WriteHiHook(0x76C616, SPLICE_, EXTENDED_, CDECL_, AstralAfterBattle);

    // Both ERA FastQuit and an in-place HD load can abandon a battle frame.
    // Reset before loading, with leave/enter events as independent fallbacks.
    Era::RegisterHandler(ResetReplayRuntime, "OnBeforeFastQuitToGameMenu");
    Era::RegisterHandler(ResetReplayRuntime, "OnGameLeave");
    Era::RegisterHandler(ResetReplayRuntime, "OnGameLeft");
    Era::RegisterHandler(ResetReplayRuntime, "OnBeforeLoadGame");
    Era::RegisterHandler(ResetReplayRuntime, "OnGameEnter");
}
} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    static bool initialized = false;
    if (reason == DLL_PROCESS_ATTACH && !initialized)
    {
        initialized = true;
        globalPatcher = GetPatcher();
        if (globalPatcher->GetInstance(INSTANCE_NAME))
            return TRUE;
        _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
        Era::ConnectEra(module, INSTANCE_NAME);
        InstallHooks();
    }
    return TRUE;
}
