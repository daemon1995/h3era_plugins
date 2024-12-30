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
            _PI = globalPatcher->CreateInstance("EraPlugin.Graphics.TimeZoneSwitching.daemon_n");

            timezone::TownManager::Get();
            Era::ConnectEra();
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
