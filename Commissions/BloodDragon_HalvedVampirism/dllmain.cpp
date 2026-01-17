// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

BOOL isBloodDragonAttack = false;
_LHF_(WoG_BattleStack_AfterHitSpell_BloodDragon)
{
    isBloodDragonAttack = true;
    c->return_address = 0x0756D7D;
    return NO_EXEC_DEFAULT;
}
_LHF_(BattleStack_AfterHitSpell_VamprireLoc)
{
    if (isBloodDragonAttack)
    {
        isBloodDragonAttack = false;

        *(c->Eax<int *>()) >>= 1; // halve total effective damage dealt
    }
    return EXEC_DEFAULT;
}
void ChangeBloodDragonVampirism()
{
    _PI->WriteLoHook(0x756D60, WoG_BattleStack_AfterHitSpell_BloodDragon);
    _PI->WriteLoHook(0x44097C, BattleStack_AfterHitSpell_VamprireLoc);
}

_LHF_(HooksInit)
{
    ChangeBloodDragonVampirism();
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
