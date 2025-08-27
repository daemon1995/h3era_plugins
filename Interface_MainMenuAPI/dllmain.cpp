// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

namespace dllText
{
const char *PLUGIN_VERSION = "1.4";
const char *INSTANCE_NAME = "EraPlugin.Interface_MainMenuAPI.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg);
int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg);

int __stdcall DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val)
{
    const int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, val);
    MenuWidgetManager::Get().CreateWidgets(dlg, val ? mainmenu::eMenuList::LOAD_GAME : mainmenu::eMenuList::NEW_GAME);
    return result;
}

int __stdcall DlgMainMenu_NewLoad_Dtor(HiHook *h, H3BaseDlg *dlg)
{
    MenuWidgetManager::Get().DestroyWidgets(dlg);
    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}
int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{
    // Custom processing for the main menu dialog
    MenuWidgetManager::Get().HandleEvent(msg);
    return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}

int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg)
{
    MenuWidgetManager::Get().DestroyWidgets(dlg);

    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}

struct CampaignData
{
    H3BaseDlg *dlg = nullptr;
    Patch *processHook = nullptr;
} campaignData;
int __stdcall DlgMainMenu_Campaign_Ctor(HiHook *h, H3BaseDlg *dlg)
{
    const int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);

    MenuWidgetManager::Get().CreateWidgets(dlg, mainmenu::eMenuList::CAMPAIGN);
    if (campaignData.dlg == nullptr)
    {
        campaignData.dlg = dlg;
        campaignData.processHook->Apply();
    }

    return result;
}

int __stdcall DlgMainMenu_Campaign_Proc(HiHook *h, H3Msg *msg)
{
    // Custom processing for the campaign menu dialog
    if (campaignData.dlg == msg->GetDlg())
    {
        MenuWidgetManager::Get().HandleEvent(msg);
    }
    return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}

int __stdcall DlgMainMenu_Campaign_Dtor(HiHook *h, H3BaseDlg *dlg)
{
    MenuWidgetManager::Get().DestroyWidgets(dlg);

    if (campaignData.dlg)
    {
        campaignData.dlg = nullptr;
        campaignData.processHook->Undo();
    }
    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}
H3BaseDlg *__stdcall DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg)
{

    auto result = THISCALL_1(H3BaseDlg *, h->GetDefaultFunc(), dlg);

    if (const UINT size = MenuWidgetManager::Get().RegisteredNumber())
    {
        if (!MenuWidgetManager::initialized)
        {
            MenuWidgetManager::initialized = true;
            _PI->WriteHiHook(0x04FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
            _PI->WriteHiHook(0x04EF02B, THISCALL_, DlgMainMenu_Dtor); // MAIN menu
            _PI->WriteHiHook(0x04EF267, THISCALL_, DlgMainMenu_Dtor); // MAIN menu

            _PI->WriteHiHook(0x04EF31F, THISCALL_, DlgMainMenu_NewLoad_Create);
            _PI->WriteHiHook(0x04EF65A, THISCALL_, DlgMainMenu_NewLoad_Create);
            _PI->WriteHiHook(0x04D5B50, THISCALL_, DlgMainMenu_Proc); // New/ Load Game Menu Dlg Proc

            _PI->WriteHiHook(0x04F078A, THISCALL_, DlgMainMenu_Campaign_Ctor);

            campaignData.processHook =
                _PI->WriteHiHook(0x05FFAC0, THISCALL_, DlgMainMenu_Campaign_Proc); // Main Main Menu Dlg Proc
            campaignData.processHook->Undo();

            _PI->WriteHiHook(0x04F07A4, THISCALL_, DlgMainMenu_Campaign_Dtor);

            _PI->WriteHiHook(0x04EF343, THISCALL_, DlgMainMenu_NewLoad_Dtor);
            _PI->WriteHiHook(0x04EF67A, THISCALL_, DlgMainMenu_NewLoad_Dtor);
            BaseGameWidgets::RegisterWidgets();
        }
        MenuWidgetManager::Get().CreateWidgets(dlg, mainmenu::eMenuList::MAIN);
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
