// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"

namespace dllText
{
constexpr const char *PLUGIN_NAME = "Legend_Heroes.era";
constexpr const char *PLUGIN_AUTHOR = "daemon_n";
constexpr const char *PLUGIN_DATA = __DATE__;
constexpr const char *INSTANCE_NAME = "EraPlugin.LegendHeroes.daemon_n";
constexpr const char *PLUGIN_VERSION = "1.1";
} // namespace dllText

Patcher *globalPatcher;
PatcherInstance *_PI;
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

_LHF_(HooksInit)
{
    LegendHeroes::Init(_PI);
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static _bool_ plugin_On = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        if (!plugin_On)
        {
            plugin_On = 1;
            //Era::ConnectEra();

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
