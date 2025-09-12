// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
LPCSTR PLUGIN_VERSION = "0.0.1";
LPCSTR INSTANCE_NAME = "EraPlugin.ERA_ArtifactsExtender.daemon_n";
} // namespace dllText

_ERH_(OnReportVersion)
{
    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static BOOL plugin_On = 0; //!< Flag to indicate if the plugin is on
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");

            _PI->WriteHiHook(0x4EE01C, THISCALL_, artifacts::ArtifactsExtender::H3GameMainSetup__LoadObjects);

            Era::RegisterHandler(artifacts::ArtifactsExtender::OnAfterWog, "OnAfterWog");
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
