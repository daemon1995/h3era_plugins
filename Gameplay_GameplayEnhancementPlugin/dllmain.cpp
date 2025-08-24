// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
const char *PLUGIN_VERSION = "1.6.0";
const char *INSTANCE_NAME = "EraPlugin.GameplayFeatures.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText

void __stdcall OnReportVersion(Era::TEvent *e)
{
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}

_LHF_(HooksInit)
{

    scroll::MapScroller::Get();

    // if (EraJS::readInt("gem_plugin.combat_hints.enable"))
    graphics::GraphicsEnhancements::Get();
    cmbhints::CombatHints::Get();

    features::GameplayFeature::Get();
    ERI::ExtendedResourcesInfo::Get();
    static constexpr LPCSTR vipPluginInstanceName = "EraPlugin.AdventureMapHints.daemon_n";
    if (globalPatcher->GetInstance(vipPluginInstanceName) == nullptr)
    {
        advMapHints::AdventureMapHints::Init(globalPatcher->CreateInstance(vipPluginInstanceName));
    }
	artifacts::ArtifactHints::Get();
    

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

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            _PI->WriteLoHook(0x4EEAF2, HooksInit);
            _PI->WriteLoHook(0x070AD9A, WoG_BeforeTownbuildingDemolishQuestion);
            _PI->WriteLoHook(0x070C1A1, WoG_AfterTownbuildingDemolishQuestion);
            demolishButtonPatch = _PI->CreateDwordPatch(0x04F738A + 1, (int)demoBttn);
            // move and resize iam00.def (next hero buttn)
            constexpr BYTE defWidth = 32;
            _PI->WriteByte(0x401A85 + 1, defWidth);        // set width
            _PI->WriteDword(0x401A8C + 1, 679 + defWidth); // set y
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
