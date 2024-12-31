// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher;
PatcherInstance *_PI;

_LHF_(HooksInit)
{

    scroll::MapScroller::Get();

    // if (EraJS::readInt("gem_plugin.combat_hints.enable"))

    cmbhints::CombatHints::Get();

    features::GameplayFeature::Get();

    static constexpr LPCSTR vipPluginInstanceName = "EraPlugin.AdventureMapHints.daemon_n";
    if (globalPatcher->GetInstance(vipPluginInstanceName) == nullptr)
    {
        advMapHints::AdventureMapHints::Init(globalPatcher->CreateInstance(vipPluginInstanceName));
    }

    return EXEC_DEFAULT;
}

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
            _PI = globalPatcher->CreateInstance("EraPlugin.GameplayFeatures.daemon_n");
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
            Era::ConnectEra();
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
