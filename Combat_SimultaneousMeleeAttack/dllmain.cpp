#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

BOOL stackAttacking = false;
BOOL shouldRestoreDefenderAmount = false;
INT savedDefenderAmount = 0;
Patch *skipIsDiedCheck = nullptr;
struct DefenderState
{
    INT initialAmount = 0;
    INT afterAttackAmount = 0;
    BOOL stackIsUnderAttack = false;

    H3CombatCreature *stack = nullptr;
} defenderState;

BOOL8 __stdcall BattleStack_BeforeAttackEnemy(HiHook *hook, H3CombatCreature *attacker, H3CombatCreature *defender,
                                              const int direction)
{

    // if no retaliation ten return native logic
    if (attacker->info.noRetaliation)
    {
        return THISCALL_3(BOOL8, hook->GetDefaultFunc(), attacker, defender, direction);
    }

    // store defender amount
    defenderState.initialAmount = defender->numberAlive;

    // disable stack dying
    skipIsDiedCheck->Apply();

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
        defender->retaliations == 0)
    {
        skipIsDiedCheck->Undo();
        if (!defender->numberAlive)
        {
            defender->ApplyPhysicalDamage(0);
            P_CombatManager->ApplyAnimationToLastHitArmy(-1, false);
        }
        defenderState.stack = nullptr;
    }
    else
    {
        defender->numberAlive = defenderState.initialAmount;
    }

    return result;
}

BOOL8 __stdcall BattleStack_BeforeRetaliateEnemy(HiHook *hook, H3CombatCreature *attacker, H3CombatCreature *defender,
                                                 const int direction)
{
    skipIsDiedCheck->Undo();
    auto result = THISCALL_3(BOOL8, hook->GetDefaultFunc(), attacker, defender, direction);

    attacker->numberAlive = defenderState.afterAttackAmount;

    if (!attacker->numberAlive)
    {
        attacker->ApplyPhysicalDamage(0);
        P_CombatManager->ApplyAnimationToLastHitArmy(-1, false);
    }
    return result;
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

    skipIsDiedCheck =
        _PI->CreateBytePatch(0x0443E13, 0xEB); // skip the check that prevents retaliation if defender is killed

    _PI->WriteHiHook(0x0441AE5, THISCALL_, BattleStack_BeforeAttackEnemy);
    _PI->WriteHiHook(0x0441B5D, THISCALL_, BattleStack_BeforeRetaliateEnemy);
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
