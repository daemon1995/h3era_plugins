void RegisterArtifactConfigProvider();
#include "pch.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace
{
constexpr LPCSTR INSTANCE_NAME = "EraPlugin.ERA_ArtifactsExtender.daemon_n";
_ERH_(OnReportVersion)
{
    Era::ReportPluginVersion("{ERA_ArtifactsExtender} v0.1.0 (" __DATE__ ")");
}
_ERH_(OnAfterWog)
{
    artifacts::ArtifactsExtender::Get().Init();
}
} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    static BOOL pluginIsOn = 0; //!< Flag to indicate if the plugin is on
    if (reason == DLL_PROCESS_ATTACH && !pluginIsOn)
    {
        pluginIsOn = TRUE;

        Era::ConnectEra(module, INSTANCE_NAME);
        RegisterArtifactConfigProvider();
        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
        Era::RegisterHandler(OnAfterWog, "OnAfterWog");
        Era::RegisterHandler(OnReportVersion, "OnReportVersion");
    }
    return TRUE;
}
