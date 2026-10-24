// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#include "../headers/EraPluginsAPI/MainMenuCursor.h"

namespace dllText
{
const char *PLUGIN_VERSION = "2.2.3";
const char *INSTANCE_NAME = "EraPlugin.Interface_MainMenuAPI.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

static void InitializeHooks();
int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg);
int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg);

int __stdcall DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val)
{
    const int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, val);
    MainMenuGraphics::CreateLogo(dlg);
    MenuWidgetManager::Get().CreateWidgets(dlg, val ? mainmenu::eMenuFlags::LOAD_GAME : mainmenu::eMenuFlags::NEW_GAME);
    return result;
}

int __stdcall DlgMainMenu_NewLoad_Dtor(HiHook *h, H3BaseDlg *dlg)
{
    MenuWidgetManager::Get().DestroyWidgets(dlg);
    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}
int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{
    MainMenuGraphics::DrawLogo(msg);
    mainmenu::CursorRequest cursor(msg);
    if (auto *panel = NotificationPanel::instance)
    {
        if (panel->parentDlg == msg->GetDlg())
        {
            cursor.RequestHand(panel->IsLinkHovered(msg));
            panel->ProcessPanel(
                msg, mainmenu::config::Enabled("era.main_menu.rendering.redraw_always", false));
        }
    }
    const int result = FASTCALL_1(int, h->GetDefaultFunc(), msg);
    if (result != 2)
    {
        MenuWidgetManager::Get().HandleEvent(msg);
        if (cursor.IsOwner() && (msg->GetX() || cursor.WantsHand()))
            mainmenu::ApplyHandCursor(cursor.WantsHand());
    }
    return result;
}

int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg)
{
    if (auto *panel = NotificationPanel::instance)
    {
        if (panel->parentDlg == dlg)
        {
            panel->SetVisible(false);
            delete panel;
        }
    }
    const int dlgJump = P_WindowManager->resultItemID;

    const BOOL forceRedraw =
        dlgJump == Era::EGameMenuTarget::PAGE_HIGH_SCORE || dlgJump == Era::EGameMenuTarget::PAGE_CREDITS;

    MenuWidgetManager::Get().DestroyWidgets(dlg, forceRedraw);
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

    MainMenuGraphics::CreateLogo(dlg);
    MenuWidgetManager::Get().CreateWidgets(dlg, mainmenu::eMenuFlags::CAMPAIGN);
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
        MainMenuGraphics::DrawLogo(msg);
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
    InitializeHooks();
    auto *result = THISCALL_1(H3BaseDlg *, h->GetDefaultFunc(), dlg);
    if (!MenuWidgetManager::initialized)
    {
        MenuWidgetManager::initialized = true;
        BaseGameWidgets::RegisterWidgets();
        NotificationPanel::RegisterWidget();
    }
    MainMenuGraphics::CreateLogo(dlg);
    MenuWidgetManager::Get().CreateWidgets(dlg, mainmenu::eMenuFlags::MAIN);
    NotificationPanel::Init(dlg, 20, 180, 500, 390);
    return result;
}

static void InitializeHooks()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    MainMenuGraphics::Get();
    mainmenu::InstallVideoPositioning();
    _PI->WriteHiHook(0x04FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
    _PI->WriteHiHook(0x04EF02B, THISCALL_, DlgMainMenu_Dtor); // MAIN menu
    _PI->WriteHiHook(0x04EF267, THISCALL_, DlgMainMenu_Dtor); // MAIN menu

    _PI->WriteHiHook(0x04EF31F, THISCALL_, DlgMainMenu_NewLoad_Create);
    _PI->WriteHiHook(0x04EF65A, THISCALL_, DlgMainMenu_NewLoad_Create);
    _PI->WriteHiHook(0x04D5B50, THISCALL_, DlgMainMenu_Proc); // New/ Load Game Menu Dlg Proc

    _PI->WriteHiHook(0x04F078A, THISCALL_, DlgMainMenu_Campaign_Ctor);

    campaignData.processHook = _PI->CreateHiHook(0x05FFAC0, SPLICE_, EXTENDED_, THISCALL_,
                                                 DlgMainMenu_Campaign_Proc); // Main Main Menu Dlg Proc

    _PI->WriteHiHook(0x04F07A4, THISCALL_, DlgMainMenu_Campaign_Dtor);

    _PI->WriteHiHook(0x04EF343, THISCALL_, DlgMainMenu_NewLoad_Dtor);
    _PI->WriteHiHook(0x04EF67A, THISCALL_, DlgMainMenu_NewLoad_Dtor);
}

static _LHF_(HooksInit)
{
    InitializeHooks();
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
            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            globalPatcher = GetPatcher();
            // Era::RegisterHandler(OnReportVersion, "OnReportVersion");
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            _PI->WriteHiHook(0x4FB930, THISCALL_, DlgMainMenu_Create);
            _PI->WriteLoHook(0x4EEAF2, HooksInit);

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
