#include "SystemOptionsDlg.h"

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;

SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, 1, 1, P_Game->GetPlayerID())
{

    CreateOKButton();
    CreateCancelButton();
    CreateGameControlButtons();
    instance = this;
}
void SystemOptionsDlg::CreateGameControlButtons() noexcept
{
    constexpr size_t length = std::size(gameControlButtons);

    for (size_t i = 0; i < length; i++)
    {
        auto &button = gameControlButtons[i];
        const int x = widthDlg - 400 + ((i & 1) * 300);
        const int y = 100 + ((i / 2) * 50);
        auto bttn = CreateButton(x, y, button.buttonId, LPCSTR(button.defNamePtr), 1, 0, false, button.hotkey);
        // bttn->SetFrame(0);
        // bttn->SetClickFrame(1);
        // bttn->RemoveState(eControlState::ACTIVE);
        // H3DlgDefButton
    }
}
BOOL SystemOptionsDlg::OnCreate()
{
    return 1;
}
BOOL SystemOptionsDlg::DialogProc(H3Msg &msg)
{

    return 1;
}
BOOL SystemOptionsDlg::OnLeftClick(INT itemId, H3Msg &msg)
{
    using target = Era::EGameMenuTarget;

    switch (itemId)
    {
    case target::PAGE_LOAD_GAME:

    case target::PAGE_SAVE_GAME:
    case target::PAGE_RESTART:
    case target::PAGE_MAIN:
    case target::PAGE_QUIT:
    case 30722:
        break;
    default:
        return TRUE;
    }
    this->resultItemId = itemId;
    this->Stop();
    return TRUE;
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
void SystemOptionsDlg::AfterDlgClose()
{
}
SystemOptionsDlg::~SystemOptionsDlg()
{
    P_WindowManager->resultItemID = this->resultItemId;

    instance = nullptr;
}
