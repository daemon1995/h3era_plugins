// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

void Minimap_Init();
void WoGLine_Init();

Patcher *globalPatcher;
PatcherInstance *_PI;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static BOOL plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("XXL");
            Era::ConnectEra(hModule, "XXL");
            Minimap_Init();
            WoGLine_Init();
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
