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

_LHF_(CombatManager_LoadCombatHeroAsset)
{

    if (auto hero = P_CombatManager->hero[c->edi])
    {
        bool hasUniqueGraphic = false;
        libc::sprintf(h3_TextBuffer, "ssm.custom_hero_def.%d", hero->id);
        LPCSTR customDefName = EraJS::read(h3_TextBuffer, hasUniqueGraphic);
        if (hasUniqueGraphic)
        {
            c->Ecx<LPCSTR>(customDefName);
        }
    }


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
            _PI->WriteLoHook(0x46307D, CombatManager_LoadCombatHeroAsset);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
