#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

struct DefenderState
{
    INT initialAmount = 0;
    INT afterAttackAmount = 0;
    BOOL stackIsUnderAttack = false;
    BOOL retaliationPending = false;

    H3CombatCreature *stack = nullptr;
} defenderState;

void FinishDeferredDeath(H3CombatCreature *stack)
{
    // A stack killed during retaliation (e.g. by fire shield) was already removed by the engine.
    if (stack->numberAlive <= 0 && !stack->info.cannotMove)
    {
        stack->numberAlive = 0;
        stack->ApplyPhysicalDamage(0);
        P_CombatManager->ApplyAnimationToLastHitArmy(-1, false);
    }
}

void RestoreDefenderAmount(H3CombatCreature *stack, const int attackLosses)
{
    // Keep any further losses or gains that occurred while the original amount was used to retaliate.
    stack->numberAlive = stack->numberAlive > attackLosses ? stack->numberAlive - attackLosses : 0;
    FinishDeferredDeath(stack);
}

VOID __stdcall BattleStack_AttackMelee_Pre(HiHook *hook, H3CombatCreature *attacker, const int direction)
{
    // Keep nested melee actions from overwriting the enclosing exchange's saved defender.
    const DefenderState previousState = defenderState;
    defenderState = DefenderState();
    THISCALL_2(VOID, hook->GetDefaultFunc(), attacker, direction);

    if (defenderState.retaliationPending)
    {
        H3CombatCreature *stack = defenderState.stack;
        const int attackLosses = defenderState.initialAmount - defenderState.afterAttackAmount;
        defenderState = DefenderState();
        RestoreDefenderAmount(stack, attackLosses);
    }
    defenderState = previousState;
}

BOOL8 __stdcall BattleStack_BeforeAttackEnemy(HiHook *hook, H3CombatCreature *attacker, H3CombatCreature *defender,
                                              const int direction)
{
    defenderState = DefenderState();
    // Leave attacks without a possible retaliation entirely to the engine.
    if (attacker->info.noRetaliation || defender->retaliations <= 0 || defender->numberAlive <= 0 ||
        defender->info.cannotMove)
    {
        return THISCALL_3(BOOL8, hook->GetDefaultFunc(), attacker, defender, direction);
    }

    // store defender amount
    defenderState.initialAmount = defender->numberAlive;

    // set flag in order to draw proper defender amount
    defenderState.stackIsUnderAttack = true;
    defenderState.stack = defender;
    auto result = THISCALL_3(BOOL8, hook->GetDefaultFunc(), attacker, defender, direction);
    defenderState.stackIsUnderAttack = false;

    const int afterHitAmount = defender->numberAlive;
    // check if need to change retaliation logic
    defenderState.afterAttackAmount = afterHitAmount;

    // if defender can't retaliate don't change anything
    if (defenderState.initialAmount <= afterHitAmount || result || defender->activeSpellDuration[eSpell::STONE] ||
        defender->retaliations <= 0)
    {
        defenderState = DefenderState();
        FinishDeferredDeath(defender);
    }
    else
    {
        defenderState.retaliationPending = true;
        defender->numberAlive = defenderState.initialAmount;
    }

    return result;
}

BOOL8 __stdcall BattleStack_BeforeRetaliateEnemy(HiHook *hook, H3CombatCreature *attacker, H3CombatCreature *defender,
                                                 const int direction)
{
    if (!defenderState.retaliationPending || defenderState.stack != attacker)
    {
        return THISCALL_3(BOOL8, hook->GetDefaultFunc(), attacker, defender, direction);
    }

    const int attackLosses = defenderState.initialAmount - defenderState.afterAttackAmount;
    defenderState = DefenderState();
    auto result = THISCALL_3(BOOL8, hook->GetDefaultFunc(), attacker, defender, direction);
    RestoreDefenderAmount(attacker, attackLosses);
    return result;
}

_LHF_(BattleStack_DeferDefenderDeath)
{
    // 0x443E15 writes isDead after physical damage reduced the stack's amount to zero.
    // Only the main defender needs to stay alive until retaliation; all other victims die normally.
    if (defenderState.stackIsUnderAttack && defenderState.stack == c->Esi<H3CombatCreature *>())
    {
        c->return_address = 0x443E1B;
        return SKIP_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(BattleStack_DrawNumber)
{

    if (defenderState.stackIsUnderAttack && defenderState.stack == c->Ebx<H3CombatCreature *>())
    {
        c->Pop();
        c->Push(defenderState.initialAmount);
    }

    return EXEC_DEFAULT;
}
void SetSimultaniusDamage()
{

    _PI->WriteHiHook(0x04419D0, THISCALL_, BattleStack_AttackMelee_Pre);
    _PI->WriteHiHook(0x0441AE5, THISCALL_, BattleStack_BeforeAttackEnemy);
    _PI->WriteHiHook(0x0441B5D, THISCALL_, BattleStack_BeforeRetaliateEnemy);
    _PI->WriteLoHook(0x0443E15, BattleStack_DeferDefenderDeath);
    _PI->WriteLoHook(0x043E47C, BattleStack_DrawNumber);
}

_LHF_(HooksInit)
{
    SetSimultaniusDamage();
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginInitialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginInitialized)
        {
            pluginInitialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::instanceName);
            Era::ConnectEra(hModule, dllText::instanceName);

            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
