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
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Era::ConnectEra(module, INSTANCE_NAME);
        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
        Era::RegisterHandler(OnAfterWog, "OnAfterWog");
        Era::RegisterHandler(OnReportVersion, "OnReportVersion");
    }
    return TRUE;
}
