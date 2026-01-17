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

void __stdcall CombatManager_InitTownWalls(HiHook *h, H3CombatManager *cmbMgr)
{
    THISCALL_1(void, h->GetDefaultFunc(), cmbMgr);
    if (!cmbMgr->creatureBank && cmbMgr->siegeKind2 > 0)
    {
        for (auto &i : cmbMgr->fortWallsHp)
        {
            i *= 2.5f; // Double wall HP
        }
    }
}

_LHF_(HooksInit)
{
    _PI->WriteHiHook(0x465E70, THISCALL_, CombatManager_InitTownWalls);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            initialized = true;
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
