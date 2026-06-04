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
    // return EXEC_DEFAULT;

    SystemOptionsDlg dlg;
    dlg.Start();

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
    combatManager->Refresh(1, 0, 1);

    dlg.networkGame = -1;
    using target = Era::EGameMenuTarget;
    target menuTarget = target(dlg.ResultItemId());
    switch (menuTarget)
    {
    case target::PAGE_RESTART:
        if (!H3Messagebox::Choice(P_GeneralText->GetText(69)))
            break;
    case target::PAGE_LOAD_GAME:
    case target::PAGE_MAIN:
    case target::PAGE_QUIT:
        Era::FastQuitToGameMenu(menuTarget);
        break;
    default:
        break;
    }
    // THISCALL_1(void, h->GetDefaultFunc(), combatManager);
}

int __fastcall HandleSystemDlgStart(void *_msg)
{
    if (const auto msg = static_cast<H3Msg *>(_msg))
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

#define PATCH_DECLATOR(nameSpaceName, className)                                                                       \
    namespace nameSpaceName                                                                                            \
    {                                                                                                                  \
    class className : public IGamePatch                                                                                \
    {                                                                                                                  \
      public:                                                                                                          \
        static className &className::Get();                                                                            \
    };                                                                                                                 \
    }
PATCH_DECLATOR(cmbhints, CombatHints)
PATCH_DECLATOR(cmbspd, CombatSpeed)

_ERH_(OnAfterWog)
{
    _PI->WriteLoHook(0x041ABBA, AdvMapSettingsDlg);
    // _PI->WriteHiHook(0x0474834, THISCALL_, CombatManager_ShowCombatSettingsDlg);
    AdditionalConfig::Load();
    using namespace mainmenu;

    const eMenuFlags flags = static_cast<eMenuFlags>(eMenuFlags::ALL | eMenuFlags::ON_TOP);

    auto UNIQUE_BUTTON_NAME = "ERA_SystemOptionsExtension_Button";
    MenuWidgetInfo langInfo{UNIQUE_BUTTON_NAME, UNIQUE_BUTTON_NAME, flags, &HandleSystemDlgStart};
    MainMenu_RegisterWidget(langInfo);
    return;
}

_LHF_(HooksInit)
{
    cmbhints::CombatHints::Get();

    // if original combat speed patch is not present, apply it
    if (globalPatcher->GetInstance("BattleSpeed") == nullptr)
        cmbspd::CombatSpeed::Get();

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
