// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher;
PatcherInstance *_PI;

_LHF_(HooksInit)
{

    scroll::MapScroller::Get();

    // if (EraJS::readInt("gem_plugin.combat_hints.enable"))
    graphics::GraphicsEnhancements::Get();
    cmbhints::CombatHints::Get();

    features::GameplayFeature::Get();

    static constexpr LPCSTR vipPluginInstanceName = "EraPlugin.AdventureMapHints.daemon_n";
    if (globalPatcher->GetInstance(vipPluginInstanceName) == nullptr)
    {
        advMapHints::AdventureMapHints::Init(globalPatcher->CreateInstance(vipPluginInstanceName));
    }

    return EXEC_DEFAULT;
}

const char *demoBttn = "iDEMO.def";
Patch *demolishButtonPatch = nullptr;
_LHF_(WoG_BeforeTownbuildingDemolishQuestion)
{

    demolishButtonPatch->Apply();
    return EXEC_DEFAULT;
}

_LHF_(WoG_AfterTownbuildingDemolishQuestion)
{
    demolishButtonPatch->Undo();
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

            static LPCSTR pluginInstanceName = "EraPlugin.GameplayFeatures.daemon_n";
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(pluginInstanceName);

            _PI->WriteLoHook(0x4EEAF2, HooksInit);
            _PI->WriteLoHook(0x070AD9A, WoG_BeforeTownbuildingDemolishQuestion);
            _PI->WriteLoHook(0x070C1A1, WoG_AfterTownbuildingDemolishQuestion);
            demolishButtonPatch = _PI->CreateDwordPatch(0x04F738A + 1, (int)demoBttn);
            Era::ConnectEra(hModule, pluginInstanceName);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
