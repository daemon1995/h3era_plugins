#include "HelpUI.h"
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
const H3Town *townFromClick = nullptr;
bool helpDialogInitialized = false;

namespace
{
_LHF_(MainWindow_F1)
{
    main::MainDlg::PrepareMainDlg(c);
    c->return_address = 0x4F877D;
    return NO_EXEC_DEFAULT;
}
void __stdcall TownInfoConstructor(HiHook *hook, const H3Dlg *dlg, const H3Town *town, int info)
{
    THISCALL_3(void, hook->GetDefaultFunc(), dlg, town, info);
    townFromClick = town;
}
_LHF_(InitializeHelp)
{
    static bool initialized = false;
    if (initialized)
        return EXEC_DEFAULT;
    initialized = true;
    _PI->WriteLoHook(0x4F8751, MainWindow_F1);
    _PI->WriteHiHook(0x530600, THISCALL_, TownInfoConstructor);
    mainmenu::MenuWidgetInfo widget{main::MainDlg::MAIN_MENU_WIDGET_NAME,
                                    helpdlg::Text("help.ui.menu_button", "Help and reference"),
                                    mainmenu::eMenuFlags::ALL, main::MainDlg::MainMenuButtonProc};
    mainmenu::MainMenu_RegisterWidget(widget);
    helpDialogInitialized = true;
    return EXEC_DEFAULT;
}
} // namespace
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        globalPatcher = GetPatcher();
        if (!globalPatcher)
            return FALSE;
        _PI = globalPatcher->CreateInstance("EraPlugin.Help.daemon_n");
        if (!_PI)
            return FALSE;
        _PI->WriteLoHook(0x4EEAF2, InitializeHelp);
    }
    return TRUE;
}
