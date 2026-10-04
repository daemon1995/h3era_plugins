// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include "CreatureDlgHooks.h"
#include "PluginSettings.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

using namespace h3;

_LHF_(HooksInit)
{

    preview::MonPreview::Get();
    Dlg_CreatureInfo_HooksInit(_PI);
    Dlg_CreatureSpellInfo_HooksInit(_PI);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static _bool_ plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        // if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("EraPlugins.CreatureInformation.daemon_n");
            Era::ConnectEra(hModule, "EraPlugins.CreatureInformation.daemon_n");
            Era::RegisterHandler(creatureInfo::RegisterPluginSettingsButton, "OnAfterErmInited");
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
