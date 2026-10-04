// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

#include "GraphicsEnhancements.h"
#include "GameplayFeature.h"
#include "MithrilDisplay.h"
#include "ArtifactHints.h"
#include "AdventureMapHints.h"
#include "ModuleSupport.h"
#include "PluginVersion.h"

namespace dllText
{
const char *PLUGIN_VERSION = GEM_VERSION_STRING;
const char *INSTANCE_NAME = "EraPlugin.GameplayFeatures.daemon_n";
} // namespace dllText

_ERH_(OnReportVersion)
{
    const H3String version = H3String::Format("{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    Era::ReportPluginVersion(version.String());
}

_LHF_(HooksInit)
{

    if (gem::ModuleEnabled("gem_plugin.modules.graphics"))
        graphics::GraphicsEnhancements::Get();
    if (gem::ModuleEnabled("gem_plugin.modules.features"))
        features::GameplayFeature::Get();
    if (gem::ModuleEnabled("gem_plugin.modules.resources"))
        ERI::ExtendedResourcesInfo::Get();
    if (gem::ModuleEnabled("gem_plugin.modules.artifact_hints"))
        artifacts::ArtifactHints::Get();

    static constexpr LPCSTR vipPluginInstanceName = "EraPlugin.AdventureMapHints.daemon_n";
    if (gem::ModuleEnabled("gem_plugin.modules.adventure_hints") &&
        globalPatcher->GetInstance(vipPluginInstanceName) == nullptr)
        advMapHints::AdventureMapHints::Get();

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
            _REH_(OnReportVersion);

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
