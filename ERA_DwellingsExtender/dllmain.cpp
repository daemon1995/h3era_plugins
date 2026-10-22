void RegisterDwellingConfigProvider();
#include "DwellingsExtender.h"
#include "pch.h"
Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace
{
void __stdcall OnAfterWog(Era::TEvent *)
{
    dwellings::DwellingsExtender::Get().Install();
}
void __stdcall OnReportVersion(Era::TEvent *)
{
    Era::ReportPluginVersion("{ERA_DwellingsExtender} v1.0.1 (" __DATE__ ")");
}
} // namespace
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Era::ConnectEra(module, "EraPlugin.ERA_DwellingsExtender.daemon_n");
        RegisterDwellingConfigProvider();
        globalPatcher = GetPatcher();
        Era::RegisterHandler(OnAfterWog, "OnAfterWog");
        Era::RegisterHandler(OnReportVersion, "OnReportVersion");
    }
    return TRUE;
}
