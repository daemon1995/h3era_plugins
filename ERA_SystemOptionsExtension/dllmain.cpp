#define _H3API_PLUGINS_
#include "SystemOptionsDlg.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
constexpr LPCSTR PLUGIN_AUTHOR = "daemon_n";
constexpr LPCSTR PLUGIN_VERSION = "1.3.0";
constexpr LPCSTR PLUGIN_DATA = __DATE__;
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";
constexpr LPCSTR UNIQUE_BUTTON_NAME = "ERA_SystemOptionsExtension_Button";
constexpr LPCSTR BUTTON_NAME_KEY = "era.opt.mainMenuButton.name";
constexpr LPCSTR BUTTON_HINT_KEY = "era.opt.mainMenuButton.hint";
} // namespace dllText

_LHF_(AdvMapSettingsDlg)
{
    SystemOptionsDlg dlg;
    dlg.Start();

    c->return_address = 0x041ABDD;
    return NO_EXEC_DEFAULT;
}

void __stdcall CombatManager_ShowCombatSettingsDlg(HiHook *h, H3CombatManager *combatManager)
{

    using target = Era::EGameMenuTarget;
    target menuTarget = target::PAGE_DEFAULT;
    {
        SystemOptionsDlg dlg;
        dlg.networkGame = 0;
        dlg.Start();
        dlg.networkGame = -1;
        menuTarget = dlg.ResultItemId();
    }
    // return THISCALL_1(void, h->GetDefaultFunc(), combatManager);

    combatManager->doNotDrawShade = 0;
    THISCALL_3(void, 0x04934B0, combatManager, FALSE, TRUE); // BattleMgr::DrawGrid
    combatManager->Refresh();

    switch (menuTarget)
    {
    case target::PAGE_RESTART:
        // if (!H3Messagebox::Choice(P_GeneralText->GetText(69)))
        //     break;
    case target::PAGE_LOAD_GAME:
    case target::PAGE_MAIN:
    case target::PAGE_QUIT:
        Era::FastQuitToGameMenu(menuTarget);
        break;
    default:
        break;
    }
}

int __fastcall HandleSystemDlgStart(void *_msg)
{
    if (const auto msg = static_cast<H3Msg *>(_msg))
    {
        const auto callerItem = msg->GetDlg()->GetCaptionButton(msg->itemId);
        if (!callerItem)
        {
            return true;
        }
        if (msg->IsLeftClick())
        {
            SystemOptionsDlg dlg;
            dlg.Start();
        }
        else if (msg->IsRightClick())
        {
            H3Messagebox::RMB(EraJS::read(dllText::BUTTON_HINT_KEY));
        }
    }
    return true;
}

_ERH_(OnAfterWog)
{
    using namespace mainmenu;
    const eMenuFlags flags = static_cast<eMenuFlags>(eMenuFlags::ALL | eMenuFlags::ON_TOP);
    const char *name = EraJS::read(dllText::BUTTON_NAME_KEY);
    MenuWidgetInfo langInfo{dllText::UNIQUE_BUTTON_NAME, name, flags, &HandleSystemDlgStart};
    MainMenu_RegisterWidget(langInfo);

    return;
}

_LHF_(HooksInit)
{

    _PI->WriteLoHook(0x041ABBA, AdvMapSettingsDlg);
    _PI->WriteHiHook(0x0474834, THISCALL_, CombatManager_ShowCombatSettingsDlg);
    AdditionalConfig::Load();
    SystemOptionsDlg::SetPatches(_PI);

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
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
