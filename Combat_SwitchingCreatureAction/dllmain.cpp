#include "pch.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

_LHF_(LoHook_InitTxtFiles)
{
    switcher::ActionSwitcher::Get();
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        globalPatcher = GetPatcher();
        static LPCSTR moduleName = "EraPlugin.Combat.SwitchCreatureAction.daemon_n";
        _PI = globalPatcher->CreateInstance(moduleName);
        Era::ConnectEra(hModule, moduleName);

        // Initialize after the game has loaded its text and combat resources.
        _PI->WriteLoHook(0x4EEAC0, LoHook_InitTxtFiles);
    }
    return TRUE;
}
