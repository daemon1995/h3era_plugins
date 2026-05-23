#include "SystemOptionsDlg.h"

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;

SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, false, 1, P_Game->GetPlayerID())
{

    // CreateOKButton();
    // CreateCancelButton();

    auto captionBttn =
        H3DlgCaptionButton::Create(15, 16, ePageItemId::PAGE_ITEM_GENERAL, BIG_BUTTON, P_GeneralText->GetText(570),
                                   NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_1, eTextColor::HIGHLIGHT);

    captionBttn->SetClickFrame(1);
    ISettingsPage *page = GeneralSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    captionBttn =
        H3DlgCaptionButton::Create(165, 16, ePageItemId::PAGE_ITEM_ADV_MAP, BIG_BUTTON, P_GeneralText->GetText(570),
                                   NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_1, eTextColor::HIGHLIGHT);

    captionBttn->SetClickFrame(1);
    page = AdventureMapSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    captionBttn =
        H3DlgCaptionButton::Create(315, 16, ePageItemId::PAGE_ITEM_COMBAT, BIG_BUTTON, P_GeneralText->GetText(394),
                                   NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_1, eTextColor::HIGHLIGHT);

    page = CombatSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    InitDlgPages();

    CreateGameControlButtons();

    // Create10xStepsSwitchPanel(switchPanelsInfo[0], nullptr);
    // Create10xStepsSwitchPanel(switchPanelsInfo[1], nullptr);
    instance = this;
}

SystemOptionsDlg::GeneralSettingsPage *SystemOptionsDlg::GeneralSettingsPage::Create(H3DlgCaptionButton *captionbttn,
                                                                                     H3BaseDlg *dlg)
{
    GeneralSettingsPage *page = new GeneralSettingsPage(captionbttn);

    constexpr int panelX = DLG_WIDTH - ISetting::WIDTH - 33;
    // constexpr int panelY = DLG_HEIGHT - SwitchPanel::WIDTH - 33;

    const DWORD musicHintPtrs[Switch10XPanel::BUTTONS_COUNT] = {
        0x06A761C, 0x06A7624, 0x06A762C, 0x06A7634, 0x06A763C, 0x06A7644, 0x06A764C, 0x06A7654, 0x06A765C, 0x06A7664,
    };
    const DWORD soundEffectsHintPtrs[Switch10XPanel::BUTTONS_COUNT] = {
        0x06A761C, 0x06A7624, 0x06A762C, 0x06A7634, 0x06A763C, 0x06A7644, 0x06A764C, 0x06A7654, 0x06A765C, 0x06A7664,
    };

    DWORD hintPtrs[Switch10XPanel::BUTTONS_COUNT << 1];
    for (size_t i = 0; i < std::size(hintPtrs); i++)
    {
        hintPtrs[i] = 0x06A761C + i * 8;
    }
    const SwitchPanelInfo switchPanelsInfo[] = {
        {{panelX, 150}, 396, 202, 0x06987B0, hintPtrs}, // music level switch panel
        {{panelX, 220},
         397,
         212,
         0x06987B4,
         &hintPtrs[Switch10XPanel::BUTTONS_COUNT]}, // sound effects level switch panel
    };
    auto &settings = page->settings;

    for (const auto &info : switchPanelsInfo)
    {
        settings.Add(Switch10XPanel::Create(info, page->items));
    };

    return page;
}
SystemOptionsDlg::AdventureMapSettingsPage *SystemOptionsDlg::AdventureMapSettingsPage::Create(
    H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg)
{
    AdventureMapSettingsPage *page = new AdventureMapSettingsPage(captionbttn);

    return page;
}
SystemOptionsDlg::CombatSettingsPage *SystemOptionsDlg::CombatSettingsPage::Create(H3DlgCaptionButton *captionbttn,
                                                                                   H3BaseDlg *dlg)
{
    CombatSettingsPage *page = new CombatSettingsPage(captionbttn);

    return page;
}

void SystemOptionsDlg::CreateGameControlButtons() noexcept
{

    static const struct
    {
        const INT32 buttonId;
        const DWORD defNamePtr;
        const eVKey hotkey;
        const DWORD hintPtr;
    } gameControlButtons[]{
        {Era::EGameMenuTarget::PAGE_LOAD_GAME, 0x0688630, eVKey::H3VK_L, 0x06A75F4}, // load game
        {Era::EGameMenuTarget::PAGE_SAVE_GAME, 0x0688624, eVKey::H3VK_S, 0x06A75FC}, // save game
        {Era::EGameMenuTarget::PAGE_RESTART, 0x0688618, eVKey::H3VK_R, 0x06A7604},   // restart the map
        {Era::EGameMenuTarget::PAGE_MAIN, 0x068860C, eVKey::H3VK_M, 0x06A75EC},      // quit to main menu
        {Era::EGameMenuTarget::PAGE_QUIT, 0x0688600, eVKey::H3VK_Q, 0x06A760C},      // quit to desktop
        {30722, 0x0670130, eVKey::H3VK_ESCAPE, 0x06A7614},                           // back to game
    };

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
        if (const auto hint = button.hintPtr)
        {
            bttn->SetHints(nullptr, ValueAt<LPCSTR>(hint), false);
        }
        // bttn->SetFrame(0);
        // bttn->SetClickFrame(1);
        // bttn->RemoveState(eControlState::ACTIVE);
    }
}
BOOL SystemOptionsDlg::OnCreate()
{
    return 1;
}
BOOL SystemOptionsDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsRightClick() && msg.itemId)
    {
        auto it = GetH3DlgItem(msg.itemId);
        if (it && it->GetRightClickHint())
        {
            H3Messagebox::RMB(it->GetRightClickHint());
            return 0;
        }
    }

    return 1;
}
BOOL SystemOptionsDlg::OnLeftClick(INT itemId, H3Msg &msg)
{
    using target = Era::EGameMenuTarget;

    switch (itemId)
    {
    case ePageItemId::PAGE_ITEM_GENERAL:
    case ePageItemId::PAGE_ITEM_ADV_MAP:
    case ePageItemId::PAGE_ITEM_COMBAT: {

        auto &page = m_pages[itemId - 1];
        if (m_currentPage != page)
        {
            if (m_currentPage)
            {
                m_currentPage->SetVisible(FALSE);
            }
            page->SetVisible(TRUE);
            m_currentPage = page;
            Redraw();
        }
        return TRUE;
    }
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

    for (auto &page : m_pages)
    {
        delete page;
    }
    instance = nullptr;

    P_WindowManager->resultItemID = this->resultItemId;
}
