#include "SystemOptionsDlg.h"

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;

SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, false, 1, P_Game->GetPlayerID()),
      isInCombat(P_CombatManager->Get() && P_CombatManager->dlg)

{

    // CreateOKButton();
    // CreateCancelButton();

    auto captionBttn = H3DlgCaptionButton::Create(15, DLG_MARGIN, ePageItemId::PAGE_ITEM_GENERAL, BIG_BUTTON,
                                                  P_GeneralText->GetText(570), NH3Dlg::Text::BIG, 0, 0, false,
                                                  eVKey::H3VK_1, eTextColor::HIGHLIGHT);

    captionBttn->SetClickFrame(1);
    ISettingsPage *page = GeneralSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    captionBttn = H3DlgCaptionButton::Create(165, DLG_MARGIN, ePageItemId::PAGE_ITEM_ADV_MAP, BIG_BUTTON,
                                             ValueAt<LPCSTR>(0x06A6598), NH3Dlg::Text::BIG, 0, false, false,
                                             eVKey::H3VK_2, eTextColor::HIGHLIGHT);

    captionBttn->SetClickFrame(1);
    page = AdventureMapSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    captionBttn = H3DlgCaptionButton::Create(315, DLG_MARGIN, ePageItemId::PAGE_ITEM_COMBAT, BIG_BUTTON,
                                             P_GeneralText->GetText(394), NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_3,
                                             eTextColor::HIGHLIGHT);

    page = CombatSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    InitDlgPages();

    CreateGameControlButtons();

    // Create10xStepsSwitchPanel(switchPanelsInfo[0], nullptr);
    // Create10xStepsSwitchPanel(switchPanelsInfo[1], nullptr);
    instance = this;
}
#include <array>
SystemOptionsDlg::GeneralSettingsPage *SystemOptionsDlg::GeneralSettingsPage::Create(H3DlgCaptionButton *captionbttn,
                                                                                     H3BaseDlg *dlg)
{
    GeneralSettingsPage *page = new GeneralSettingsPage(captionbttn);

    constexpr int x = DLG_WIDTH - ISetting::WIDTH - 33;
    // constexpr int panelY = DLG_HEIGHT - SwitchPanel::WIDTH - 33;

    // const DWORD musicHintPtrs[Switch10XPanel::BUTTONS_COUNT] = {
    //     0x06A761C, 0x06A7624, 0x06A762C, 0x06A7634, 0x06A763C, 0x06A7644, 0x06A764C, 0x06A7654, 0x06A765C, 0x06A7664,
    // };
    // constexpr DWORD soundEffectsHintPtrs[Switch10XPanel::BUTTONS_COUNT] = {
    //     0x06A761C, 0x06A7624, 0x06A762C, 0x06A7634, 0x06A763C, 0x06A7644, 0x06A764C, 0x06A7654, 0x06A765C, 0x06A7664,
    // };

    constexpr auto hintPtrs = [] {
        std::array<DWORD, Switch10XPanel::BUTTONS_COUNT << 1> arr{};

        for (size_t i = 0; i < arr.size(); ++i)
        {
            arr[i] = 0x06A761C + i * 8;
        }

        return arr;
    }();

    const SwitchPanelInfo switchPanelsInfo[] = {
        {{x, 150}, 396, 202, 0x06987B0, &hintPtrs[0]},                            // music level switch panel
        {{x, 220}, 397, 212, 0x06987B4, &hintPtrs[Switch10XPanel::BUTTONS_COUNT]} // sound effects level switch panel
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

    constexpr auto hintPtrs = [] {
        std::array<DWORD, Switch10XPanel::BUTTONS_COUNT << 1> arr{};

        for (size_t i = 0; i < 3; ++i)
            arr[i] = 0x06A5704;
        for (size_t i = 3; i < 7; ++i)
            arr[i] = 0x06A570C;
        for (size_t i = 7; i < arr.size(); ++i)
            arr[i] = 0x06A5714;
        return arr;
    }();
    constexpr int x = DLG_ITEM_VMARGIN;
    constexpr int y = DLG_HEIGHT - Switch10XPanel::HEIGHT - 25;

    const SwitchPanelInfo switchPanelsInfo{
        {x, y}, 395, 202, H3CurrentAnimationSpeed::ADDRESS, &hintPtrs[0] // music level switch panel
    };

    page->settings.Add(Switch10XPanel::Create(switchPanelsInfo, page->items));

    return page;
}

void SystemOptionsDlg::CreateGameControlButtons() noexcept
{

    // const BOOL isNetworkGame = P_
    using target = Era::EGameMenuTarget;
    static const struct
    {
        const INT32 buttonId;
        const DWORD defNamePtr;
        const eVKey hotkey;
        const DWORD hintPtr;
        const INT32 disableOnCombat = FALSE;
    } gameControlButtons[]{
        {target::PAGE_LOAD_GAME, 0x0688630, eVKey::H3VK_L, 0x06A75F4, isInCombat && networkGame},                          // load game
        {target::PAGE_SAVE_GAME, 0x0688624, eVKey::H3VK_S, 0x06A75FC, isInCombat},                          // save game
        {target::PAGE_RESTART, 0x0688618, eVKey::H3VK_R, 0x06A7604, isInCombat && networkGame}, // restart the map
        {target::PAGE_MAIN, 0x068860C, eVKey::H3VK_M, 0x06A75EC},                               // quit to main menu
        {target::PAGE_QUIT, 0x0688600, eVKey::H3VK_Q, 0x06A760C},                               // quit to desktop
        {30722, 0x0670130, eVKey::H3VK_ESCAPE, isInCombat ? 0x06A5614 : 0x06A7614}, // back to adv map / combat
    };

    constexpr size_t length = std::size(gameControlButtons);
    constexpr int buttonWidth = 100 + 13;
    constexpr int buttonHeight = 48 + 10;

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
        if (button.disableOnCombat)
        {
			bttn->Disable();
//            bttn->SendCommand(5, 4096);
        }
    }
}
BOOL SystemOptionsDlg::OnCreate()
{
    SetActivePage(PAGE_INDEX_GENERAL, FALSE);

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

        SetActivePage(itemId - 1, TRUE);
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
