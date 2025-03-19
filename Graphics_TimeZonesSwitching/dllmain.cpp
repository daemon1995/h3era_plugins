// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static bool pluginIsOn = false;
    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;

            globalPatcher = GetPatcher();
            constexpr char pluginName[] = "Graphics.TimeZoneSwitching.daemon_n";
            _PI = globalPatcher->CreateInstance(pluginName);

            timezone::TownManager::Get();
            Era::ConnectEra(hModule, pluginName);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
