// dllmain.cpp : Defines the entry point for the DLL application.
#include "MenuWidgetManager.h"
#include "framework.h"
#include <minwindef.h>

namespace dllText
{
const char *PLUGIN_VERSION = "1.0";
const char *INSTANCE_NAME = "EraPlugin.Interface_MainMenuAPI.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg);
int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg);

H3BaseDlg *__stdcall DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg)
{

    auto result = THISCALL_1(H3BaseDlg *, h->GetDefaultFunc(), dlg);

    if (const UINT size = MenuWidgetManager::Get().GetWidgets().size())
    {

        static bool initialized = false;

        if (!initialized)
        {
            _PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc);

            _PI->WriteHiHook(0x04EF267, THISCALL_, DlgMainMenu_Dtor); // MAIN menu
        }
        MenuWidgetManager::Get().CreateWidgets(dlg, 804, 100, 120, 24, 4);
    }
    else
    {
        // No widgets registered, handle accordingly
        h->Undo();
        //  h->Destroy();
    }

    // Call the original function
    return result;
}

int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{
    // Custom processing for the main menu dialog
    MenuWidgetManager::Get().HandleEvent(msg);

    return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}

int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg)
{

    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}
void __stdcall OnAfterWog(Era::TEvent *e)
{

    // This function is called after the Wog event
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
            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            globalPatcher = GetPatcher();
            // Era::RegisterHandler(OnReportVersion, "OnReportVersion");
            Era::RegisterHandler(OnAfterWog, "OnAfterWog");
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            _PI->WriteHiHook(0x04EF247, THISCALL_, DlgMainMenu_Create);
            // _PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc);
            // _PI->WriteHiHook(0x04EF267, THISCALL_, DlgMainMenu_Dtor);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
