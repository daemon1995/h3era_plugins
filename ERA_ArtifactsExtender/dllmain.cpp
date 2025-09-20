// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
// @todo
/**
    1. Combo artifacts suppot
        - increase max art ids to the last one
    2. New artifacts features
        - loaded from json
    3. Scripts Support
        - Getting a proper max art Id
    4. RMG Dlg/Game artifacts Support (RMG plugin)
        - enable/disable during the map generation

    */
namespace dllText
{
LPCSTR PLUGIN_VERSION = "0.0.1";
LPCSTR INSTANCE_NAME = "EraPlugin.ERA_ArtifactsExtender.daemon_n";
} // namespace dllText
namespace artifacts
{

_ERH_(OnReportVersion)
{
    //  MessageBoxA(nullptr, "OnReportVersion", "", MB_OK);

    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}
} // namespace artifacts
_ERH_(OnAfterWog)
{

    void *originalTableAddress = reinterpret_cast<void *>(0x07B6DA0);

    // allow only if address is not chabged by other mods
    if (originalTableAddress == Era::GetRealAddr(originalTableAddress))
    {
        Era::RegisterHandler(artifacts::OnReportVersion, "OnReportVersion");

        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
        artifacts::ArtifactsExtender::Get();
        //        Era::RegisterHandler(artifacts::ArtifactsExtender::OnAfterWog, "OnAfterWog");
    }
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

            // _PI->WriteHiHook(0x4EE01C, THISCALL_, artifacts::ArtifactsExtender::H3GameMainSetup__LoadObjects);

            // created interlayer to avoid issues with other plugins
            Era::RegisterHandler(OnAfterWog, "OnAfterWog");

            //    _PI->WriteLoHook(0x04EDEA2, artifacts::ArtifactsExtender::LoadArtTraits);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
