// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;
namespace dllText
{
    LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

void __stdcall BattleStack_BeforeShoot(HiHook* h, H3CombatCreature* attacker, H3CombatCreature* defender)

{
    // native shooter

    THISCALL_2(void, h->GetDefaultFunc(), attacker, defender);

    if (!THISCALL_2(BOOL8, 0x442610, defender, nullptr) // can't shoot
        || attacker->info.noRetaliation || defender->blinded || defender->paralyzed ||
        defender->activeSpellDuration[eSpell::STONE] || defender->retaliations < 1)
    {
        return;
    }

    P_CombatManager->currentActiveSide = 1 - P_CombatManager->currentActiveSide;
    THISCALL_2(void, 0x043F620, defender, attacker);
    P_CombatManager->currentActiveSide = 1 - P_CombatManager->currentActiveSide;
    --defender->retaliations;

}
void SetRangedRetaliation()
{
    // allow ranged retaliation
    _PI->WriteHiHook(0x043FF79, THISCALL_, BattleStack_BeforeShoot);
}

_LHF_(HooksInit)
{
    SetRangedRetaliation();
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
