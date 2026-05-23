#include "SystemOptionsDlg.h"

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;

SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, false, 1, P_Game->GetPlayerID())
{

    // CreateOKButton();
    // CreateCancelButton();
    CreateGameControlButtons();

    constexpr int panelX = DLG_WIDTH - ISetting::WIDTH - 33;
    // constexpr int panelY = DLG_HEIGHT - SwitchPanel::WIDTH - 33;

    constexpr SwitchPanelInfo switchPanelsInfo[] = {
        {{panelX, 150}, 396, 202, 0x06987B0}, // music level switch panel
        {{panelX, 220}, 397, 212, 0x06987B4}, // sound effects level switch panel
    };
    Create10xStepsSwitchPanel(switchPanelsInfo[0], nullptr);
    Create10xStepsSwitchPanel(switchPanelsInfo[1], nullptr);
    instance = this;
}
void SystemOptionsDlg::Create10xStepsSwitchPanel(const SwitchPanelInfo &panelInfo, ISettingsPage *page) noexcept
{

    int itemX = panelInfo.position.x;
    int itemY = panelInfo.position.y;

    const DWORD currentValue = Clamp(0, IntAt(panelInfo.valuePtr), 9);
    // create text field with name of the panel
    constexpr int textFieldWidth = 10 * 19;

    auto textField =
        H3DlgText::Create(itemX, itemY, textFieldWidth, 24, P_GeneralText->GetText(panelInfo.generalStringIndex),
                          NH3Dlg::Text::MEDIUM, eTextColor::HIGHLIGHT, -1);
    AddItem(textField, page);

    itemY += 20;
    // create background pcx
    auto bgPcx = H3DlgPcx::Create(itemX, itemY, SwitchPanel::bgPcxPath);
    AddItem(bgPcx, page);
    itemY += 3;
    itemX += 4;
    for (size_t i = 0; i < 10; i++)
    {

        auto def = H3DlgDef::Create(itemX, itemY, panelInfo.firstItemId + i, NH3Dlg::Assets::SYSLB_DEF, i);
        def->SendCommand(6, 4);
        if (currentValue == i)
        {
            def->SendCommand(5, 4);
        }
        AddItem(def, page);
        itemX += 19;
    }
}
void SystemOptionsDlg::CreateGameControlButtons() noexcept
{
    constexpr size_t length = std::size(gameControlButtons);

    constexpr int buttonWidth = 100 + 13;
    constexpr int buttonHeight = 58;

    for (size_t i = 0; i < length; i++)
    {
        auto &button = gameControlButtons[i];
        const int x = widthDlg - buttonWidth * 2 + ((i & 1) * buttonWidth) - 10;
        const int y = heightDlg - buttonHeight * 3 + ((i / 2) * buttonHeight) - 12;
        auto bttn = CreateButton(x, y, button.buttonId, LPCSTR(button.defNamePtr), 1, 0, false, button.hotkey);
        if (button.hotkey == eVKey::H3VK_ESCAPE)
        {
            bttn->AddHotkey(eVKey::H3VK_ENTER);
        }
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
    instance = nullptr;

    P_WindowManager->resultItemID = this->resultItemId;
}
