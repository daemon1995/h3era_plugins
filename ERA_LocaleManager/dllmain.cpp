// dllmain.cpp : Defines the entry point for the DLL application.
#define _H3API_PLUGINS_
#include "framework.h"
void InitLanguageSelectionDlg();

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
constexpr LPCSTR PLUGIN_AUTHOR = "daemon_n";
constexpr LPCSTR PLUGIN_VERSION = "3.0";
constexpr LPCSTR PLUGIN_DATA = __DATE__;
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";
} // namespace dllText

_ERH_(OnReportVersion)
{
    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
    InitLanguageSelectionDlg();
}

_LHF_(HooksInit)
{
    return EXEC_DEFAULT;
}

_ERH_(OnAfterCreateWindow)
{
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static bool initialized = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            initialized = true;
            if (Era::GetVersionNum() >= 3930)
            {
                globalPatcher = GetPatcher();
                _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

                Era::ConnectEra(hModule, dllText::INSTANCE_NAME);

                _REH_(OnReportVersion);
                // _REH_(OnAfterWog);
                // _REH_(OnAfterCreateWindow);
                // _PI->WriteLoHook(0x4EEAF2, HooksInit);
            }
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
