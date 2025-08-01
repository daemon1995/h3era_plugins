// dllmain.cpp : Defines the entry point for the DLL application.

#include "pch.h"

using namespace h3;
namespace dllText
{
constexpr const char *PLUGIN_AUTHOR = "daemon_n";
constexpr const char *PLUGIN_VERSION = "2.1";
constexpr const char *PLUGIN_DATA = __DATE__;
constexpr const char *INSTANCE_NAME = "EraPlugin.LanguageSelectionDlg.daemon_n";
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

void __stdcall OnReportVersion(Era::TEvent *e)
{

    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}

_LHF_(HooksInit)
{

    MapObjectHandler::Init();
    TownHandler::Init();
    HeroHandler::Init();
    // ArtifactHandler::Init();
    LanguageSelectionDlg::Init();

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

            // Era::ConnectEra();
            if (Era::GetVersionNum() >= 3906)
            {

                Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
                Era::RegisterHandler(OnReportVersion, "OnReportVersion");

                globalPatcher = GetPatcher();
                _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
                // must have hooks before other
                MonsterHandler::Init();

                ArtifactHandler::Init();
                //   _PI->WriteLoHook(0x04EE053, gem_TestHook);

                _PI->WriteLoHook(0x4EEAF2, HooksInit);
            }
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
