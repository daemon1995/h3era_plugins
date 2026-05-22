#define _H3API_PLUGINS_
#include "SystemOptionsDlg.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
constexpr LPCSTR PLUGIN_AUTHOR = "daemon_n";
constexpr LPCSTR PLUGIN_VERSION = "1.0";
constexpr LPCSTR PLUGIN_DATA = __DATE__;
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";
} // namespace dllText

_LHF_(AdvMapSettingsDlg)
{
    SystemOptionsDlg dlg;
    dlg.Start();

    // H3Dlg dlg(200, 200);
    c->return_address = 0x041ABDD;
    return NO_EXEC_DEFAULT;
}

void __stdcall CombatManager_ShowCombatSettingsDlg(HiHook *h, H3CombatManager *combatManager)
{
    SystemOptionsDlg dlg;
    dlg.networkGame = 0;
    dlg.Start();
    combatManager->doNotDrawShade = false;
    THISCALL_3(void, 0x04934B0, combatManager, FALSE, TRUE); // BattleMgr::DrawGrid
   //  THISCALL_3(void, 0x04934B0, combatManager, FALSE, TRUE); // BattleMgr::DrawGrid
    combatManager->Refresh(1, 0, 1);
    dlg.networkGame = -1;
    // THISCALL_1(void, h->GetDefaultFunc(), combatManager);
}

int __fastcall CurrentDlg_HandleLocaleDlgStart(void* _msg)
{
    if (const auto msg = static_cast<H3Msg*>(_msg))
    {
        const auto callerItem = msg->GetDlg()->GetCaptionButton(msg->itemId);
        if (callerItem && msg->IsLeftClick())
        {
            SystemOptionsDlg dlg;
            dlg.Start();
        }
    }
    return true;
}

_ERH_(OnAfterWog)
{
    _PI->WriteLoHook(0x041ABBA, AdvMapSettingsDlg);
    _PI->WriteHiHook(0x0474834, THISCALL_, CombatManager_ShowCombatSettingsDlg);

    using namespace mainmenu;

    const eMenuFlags flags = static_cast<eMenuFlags>(eMenuFlags::ALL | eMenuFlags::ON_TOP);

	auto UNIQUE_BUTTON_NAME = "ERA_SystemOptionsExtension_Button";
    MenuWidgetInfo langInfo{ UNIQUE_BUTTON_NAME, UNIQUE_BUTTON_NAME, flags, &CurrentDlg_HandleLocaleDlgStart };
    MainMenu_RegisterWidget(langInfo);
    return;
}

_LHF_(HooksInit)
{
    return EXEC_DEFAULT;
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
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);

            _REH_(OnAfterWog);
            // _REH_(OnAfterCreateWindow);
            // _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
