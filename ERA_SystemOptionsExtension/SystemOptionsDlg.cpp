#include "SystemOptionsDlg.h"

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;

SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, 1, 1, P_Game->GetPlayerID())
{

    CreateOKButton();
    CreateCancelButton();
    instance = this;
}
BOOL SystemOptionsDlg::OnCreate()
{
    return 1;
}
BOOL SystemOptionsDlg::DialogProc(H3Msg &msg)
{
    return 1;
}
VOID SystemOptionsDlg::OnOK()
{
    return VOID();
}
VOID SystemOptionsDlg::OnCancel()
{

    // auto module = LoadLibraryA("ERA_LocaleManager.era");
    // if (!module)
    //     return VOID();
    // auto func = reinterpret_cast<void(__stdcall *)(int, int, int)>(GetProcAddress(module, "CallLocaleSelectionDlg"));
    // if (func)
    //{
    //     func(-1, -1, 1);
    // }

    return VOID();
}
SystemOptionsDlg::~SystemOptionsDlg()
{
    instance = nullptr;
}
