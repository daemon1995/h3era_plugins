#include "SystemOptionsDlg.h"

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;
void __stdcall ShowHealthBarDlg();
BOOL *__stdcall HealthBarIsEnabledAddress();

enum eSettingsOriginalItemIds : int
{
    SHOW_HERO_ROUTE_CHECKBOX = 235,
    QUICK_BATTLE_CHECKBOX = 237,
    VIDEO_SUBTITLES_CHECKBOX = 238,
    TOWN_BUILDING_OUTLINES_CHECKBOX = 241,
    SPELL_BOOK_ANIMATION_CHECKBOX = 242,
};
SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, false, 1, P_Game->GetPlayerID()),
      isInCombat(P_CombatManager->Get() && P_CombatManager->dlg)

{

    auto captionBttn = H3DlgCaptionButton::Create(15, DLG_CAPTION_BUTTON_TOP_MARGIN, ePageItemId::PAGE_ITEM_GENERAL,
                                                  BIG_BUTTON, P_GeneralText->GetText(570), NH3Dlg::Text::BIG, 0, 0,
                                                  false, eVKey::H3VK_1, eTextColor::HIGHLIGHT);

    captionBttn->SetClickFrame(1);
    ISettingsPage *page = GeneralSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    captionBttn = H3DlgCaptionButton::Create(165, DLG_CAPTION_BUTTON_TOP_MARGIN, ePageItemId::PAGE_ITEM_ADV_MAP,
                                             BIG_BUTTON, ValueAt<LPCSTR>(0x06A6598), NH3Dlg::Text::BIG, 0, false, false,
                                             eVKey::H3VK_2, eTextColor::HIGHLIGHT);

    captionBttn->SetClickFrame(1);
    page = AdventureMapSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    captionBttn = H3DlgCaptionButton::Create(315, DLG_CAPTION_BUTTON_TOP_MARGIN, ePageItemId::PAGE_ITEM_COMBAT,
                                             BIG_BUTTON, P_GeneralText->GetText(394), NH3Dlg::Text::BIG, 0, 0, false,
                                             eVKey::H3VK_3, eTextColor::HIGHLIGHT);

    page = CombatSettingsPage::Create(captionBttn, this);
    AddItem(captionBttn, page);
    m_pages.Add(page);

    InitDlgPages();

    CreateGameControlButtons();

    instance = this;
}
#include <array>

void __stdcall CallWogOptionsDlg()
{
    const int storeValue = IntAt(0x291A430);
    IntAt(0x291A430) = 1;
    auto jumpOverMouseCheck = _PI->WriteJmp(0x07790FB, 0x0779157);
    STDCALL_0(VOID, 0x597AA0); // stop video animation
    THISCALL_0(int, 0x07790E1);
    STDCALL_0(VOID, 0x597B50); // resume video animation

    jumpOverMouseCheck->Destroy();
    IntAt(0x291A430) = storeValue;
}
SystemOptionsDlg::GeneralSettingsPage *SystemOptionsDlg::GeneralSettingsPage::Create(H3DlgCaptionButton *captionBttn,
                                                                                     H3BaseDlg *dlg)
{
    GeneralSettingsPage *page = new GeneralSettingsPage(captionBttn);
    int itemId = page->firstItemId;

    LPCSTR videoDefNames[] = {LPCSTR(DwordAt(0x05B2300 + 1)), LPCSTR(DwordAt(0x05B22A9 + 1))};
    DWORD switchPanelHints[] = {0x06A774C, 0x06A7754};
    constexpr size_t videoDefNum = std::size(videoDefNames);
    constexpr int switchPanelX = DLG_LEFT_PART_X_MARGIN;

    const int startY = captionBttn->GetY() + captionBttn->GetHeight() + DLG_CAPTION_BUTTON_TOP_MARGIN;

    SwitchPanelInfo switchPanelsInfo = {
        {switchPanelX, startY}, itemId, P_GeneralText->GetText(22), 0x06987F8, 0, videoDefNum, &videoDefNames[0],
        &switchPanelHints[0],

    };
    page->settings.Add(SwitchPanel::Create(switchPanelsInfo, page->items));
    itemId += videoDefNum;
    constexpr int checkboxX = DLG_LEFT_PART_X_MARGIN;
    constexpr DWORD checkboxesHintPtrs[] = {0x06A7744, 0x06A775C, 0x06A7764};

    const SettingsInfo checkboxesInfo[] = {
        {"system_video_subtitles",
         {checkboxX, startY + 90},
         itemId++,
         P_GeneralText->GetText(577),
         0x06987D0,
         TRUE,

         &checkboxesHintPtrs[0]}, // show tips
        {"system_building_outlines",
         {checkboxX, startY + 120},
         itemId++,
         P_GeneralText->GetText(578),
         0x06987D4,
         TRUE,

         &checkboxesHintPtrs[1]}, // show tips in battle
        {"system_spell_book_animation",
         {checkboxX, startY + 150},
         itemId++,
         P_GeneralText->GetText(579),
         0x06987D8,
         TRUE,

         &checkboxesHintPtrs[2]} // show tips in battle

    };
    for (auto &i : checkboxesInfo)
    {
        page->settings.Add(CheckBoxSetting::Create(i, page->items));
    }

    // CREATE CALLBACK BUTTONS
    // wog option buttons:
    const SettingsInfo wogOptionCaption = {"system_wog_option", {checkboxX, 300},          itemId++,
                                           "wog options",       (DWORD)&CallWogOptionsDlg, 0};
    page->settings.Add(
        CaptionButtonSetting::Create(wogOptionCaption, *reinterpret_cast<char **>(0x57A93B + 1), page->items));

    auto plugin = GetModuleHandleA("ERA_LocaleManager.era");
    if (plugin)
    {

        typedef void(__stdcall * CallLocaleSelectionDlg_t)(const int x, const int y, const int style);
        typedef const char *(__stdcall * GetDisplayedName_t)();
        auto callDlg = reinterpret_cast<CallLocaleSelectionDlg_t>(GetProcAddress(plugin, "CallLocaleSelectionDlg"));
        auto getTextName = reinterpret_cast<GetDisplayedName_t>(GetProcAddress(plugin, "GetDisplayedName"));

        if (callDlg && getTextName)
        {
            const SettingsInfo selectLang = {
                "system_select_language", {checkboxX, 330}, itemId++, getTextName(), (DWORD)&callDlg, 0};

            page->settings.Add(
                CaptionButtonSetting::Create(selectLang, *reinterpret_cast<char **>(0x57A93B + 1), page->items));
        }
    }

    // language selection dlg:

    // RIGHT PAGE PART

    constexpr auto hintPtrs = [] {
        std::array<DWORD, Switch10XPanel::BUTTONS_COUNT << 1> arr{};

        for (size_t i = 0; i < arr.size(); ++i)
        {
            arr[i] = 0x06A761C + i * 8;
        }

        return arr;
    }();

    constexpr int panelX = DLG_RIGHT_PART_X_MARGIN;
    const SettingsInfo switch10PanelsInfo[] = {

        {"system_music_level",
         {panelX, 150},
         itemId,
         P_GeneralText->GetText(396),
         0x06987B0,
         1,
         &hintPtrs[0]}, // music level switch panel
        {"system_sound_effects_level",
         {panelX, 220},
         itemId + Switch10XPanel::BUTTONS_COUNT,
         P_GeneralText->GetText(397),
         0x06987B4,
         1,
         &hintPtrs[10]} // sound effects level switch panel
    };
    auto &settings = page->settings;

    for (const auto &info : switch10PanelsInfo)
    {
        settings.Add(Switch10XPanel::Create(info, page->items));
    };
    itemId += Switch10XPanel::BUTTONS_COUNT << 1;

    return page;
}
SystemOptionsDlg::AdventureMapSettingsPage *SystemOptionsDlg::AdventureMapSettingsPage::Create(
    H3DlgCaptionButton *captionBttn, H3BaseDlg *dlg)
{
    AdventureMapSettingsPage *page = new AdventureMapSettingsPage(captionBttn);
    int itemId = page->firstItemId;

    constexpr int switchPanelX = DLG_LEFT_PART_X_MARGIN;
    const int startY = captionBttn->GetY() + captionBttn->GetHeight() + DLG_CAPTION_BUTTON_TOP_MARGIN;

    LPCSTR playerSpeedDefNames[] = {LPCSTR(DwordAt(0x005B1EEF + 1)), LPCSTR(DwordAt(0x005B1F43 + 1)),
                                    LPCSTR(DwordAt(0x005B1F97 + 1)), LPCSTR(DwordAt(0x005B1FEB + 1))};
    LPCSTR enemySpeedDefNames[] = {LPCSTR(DwordAt(0x05B2042 + 1)), LPCSTR(DwordAt(0x05B2099 + 1)),
                                   LPCSTR(DwordAt(0x05B20F0 + 1)), LPCSTR(DwordAt(0x05B2147 + 1))};
    LPCSTR mapScrollDefNames[] = {LPCSTR(DwordAt(0x05B21A1 + 1)), LPCSTR(DwordAt(0x05B21F8 + 1)),
                                  LPCSTR(DwordAt(0x05B224F + 1))};
    DWORD switchPanelHints[] = {0x06A76D4, 0x06A76DC, 0x06A76E4, 0x06A76EC, 0x06A76F4, 0x06A76FC,
                                0x06A7704, 0x06A770C, 0x06A7714, 0x06A771C, 0x06A7724};
    constexpr size_t playerDefNum = std::size(playerSpeedDefNames);
    constexpr size_t enemyDefNum = std::size(enemySpeedDefNames);
    constexpr size_t mapScrollDefNum = std::size(mapScrollDefNames);
    SwitchPanelInfo switchPanelsInfo[] = {
        {{switchPanelX, startY},
         itemId,
         P_GeneralText->GetText(571),
         0x06987AC,
         -1,
         playerDefNum,
         &playerSpeedDefNames[0],
         &switchPanelHints[0]},
        {{switchPanelX, startY + 70},
         itemId + playerDefNum,
         P_GeneralText->GetText(572),
         0x06987A8,
         -2,
         enemyDefNum,
         &enemySpeedDefNames[0],
         &switchPanelHints[playerDefNum]},
        {{switchPanelX, startY + 140},
         itemId + playerDefNum + enemyDefNum,
         P_GeneralText->GetText(573),
         0x06987DC,
         0,
         mapScrollDefNum,
         &mapScrollDefNames[0],
         &switchPanelHints[playerDefNum + enemyDefNum]},
    };
    for (auto &i : switchPanelsInfo)
    {
        page->settings.Add(SwitchPanel::Create(i, page->items));
    }

    itemId += playerDefNum + enemyDefNum + mapScrollDefNum;

    // create checkboxes
    constexpr int checkboxX = DLG_LEFT_PART_X_MARGIN;
    int checkboxY = 280;
    constexpr DWORD checkboxesHintPtrs[] = {0x06A772C, 0x06A7734, 0x69FC88};

    const SettingsInfo checkboxesInfo[] = {
        {"adventure_show_route",
         {checkboxX, checkboxY + 30},
         itemId++,
         P_GeneralText->GetText(574),
         0x06987C4,
         TRUE,

         &checkboxesHintPtrs[0]}, // show tips
        {"adventure_hero_reminder",
         {checkboxX, checkboxY + 60},
         itemId++,
         P_GeneralText->GetText(575),
         0x06987C8,
         TRUE,

         &checkboxesHintPtrs[1]}, // show tips in battle
        {"adventure_game_autosave",
         {checkboxX, checkboxY + 90},
         itemId++,
         P_GeneralText->GetText(576),
         0x06987C0,
         TRUE,

         &checkboxesHintPtrs[2]}, // show tips in battle
    };
    for (auto &i : checkboxesInfo)
    {
        page->settings.Add(CheckBoxSetting::Create(i, page->items));
    }

    return page;
}
SystemOptionsDlg::CombatSettingsPage *SystemOptionsDlg::CombatSettingsPage::Create(H3DlgCaptionButton *captionBttn,
                                                                                   H3BaseDlg *dlg)
{
    CombatSettingsPage *page = new CombatSettingsPage(captionBttn);
    constexpr int x = DLG_LEFT_PART_X_MARGIN;
    const int startY = captionBttn->GetY() + captionBttn->GetHeight() + 12;
    // standard checkboxes
    int itemId = page->firstItemId;

    // LEFT DLG PART
    DWORD hintsArray[] = {
        0x06A572C, // show combat show_grid
        0x06A5734, // show movements_shadow
        0x06A573C, // cursor_shadow
        0x06A56DC, // show combat messages
        0x06A56E4, // show combat animations
        0x06A56EC, // show floating combat text
        0x06A56F4, // show ballista range
        0x06A56FC, // show battle interface
    };
    const SettingsInfo checkboxesInfo[] = {
        {"show_grid", {x, startY}, itemId++, P_GeneralText->GetText(406), 0x069880C, TRUE, &hintsArray[0]},
        {"movements_shadow",
         {x, startY + 30},
         itemId++,
         P_GeneralText->GetText(407),
         0x0698814,
         TRUE,
         &hintsArray[1]}, // show movements_shadow
        {"cursor_shadow", {x, startY + 60}, itemId++, P_GeneralText->GetText(408), 0x0698810, TRUE, &hintsArray[2]},
        {"auto_combat_creatures",
         {x, startY + 120},
         itemId++,
         P_GeneralText->GetText(400),
         0x06987E4,
         TRUE,

         &hintsArray[3]}, // show combat messages
        {"auto_combat_spells",
         {x, startY + 150},
         itemId++,
         P_GeneralText->GetText(401),
         0x06987E8,
         TRUE,

         &hintsArray[4]}, // show combat animations
        {"auto_combat_catapult",
         {x, startY + 180},
         itemId++,
         P_GeneralText->GetText(402),
         0x06987EC,
         TRUE,

         &hintsArray[5]}, // show floating combat text
        {"auto_combat_ballista",
         {x, startY + 210},
         itemId++,
         P_GeneralText->GetText(153),
         0x06987F0,
         TRUE,

         &hintsArray[6]},
        {"auto_combat_tent",
         {x, startY + 240},
         itemId++,
         P_GeneralText->GetText(403),
         0x06987F4,
         TRUE,

         &hintsArray[7]}, // show battle interface
    };

    for (auto &i : checkboxesInfo)
    {
        page->settings.Add(CheckBoxSetting::Create(i, page->items));
    }

    // combat speed switch panel
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
    constexpr int ySwitch = DLG_HEIGHT - Switch10XPanel::HEIGHT - 25;

    const SettingsInfo switch10xPanelsInfo{
        "combat_animation_speed",
        {x, ySwitch},
        itemId,
        P_GeneralText->GetText(395),
        H3CurrentAnimationSpeed::ADDRESS,
        1,
        &hintPtrs[0] // music level switch panel
    };
    itemId += Switch10XPanel::BUTTONS_COUNT;
    page->settings.Add(Switch10XPanel::Create(switch10xPanelsInfo, page->items));

    constexpr int rightX = DLG_RIGHT_PART_X_MARGIN;

    const SettingsInfo healthBarcheckBoxInfo = {"system_health_bar_checkbox",
                                                {rightX, startY + 60},
                                                itemId++,
                                                "health bar checkbox",
                                                (DWORD)HealthBarIsEnabledAddress(),
                                                TRUE,
                                                0};

    auto it = CheckBoxSetting::Create(healthBarcheckBoxInfo, page->items);
    page->settings.Add(it);

    const SettingsInfo healtBarCaptionInfo = {"system_health_bar", {rightX, startY + 90},    itemId++,
                                              "health bar",        (DWORD)&ShowHealthBarDlg, 0};

    page->settings.Add(
        CaptionButtonSetting::Create(healtBarCaptionInfo, *reinterpret_cast<char **>(0x57A93B + 1), page->items));

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
        {target::PAGE_LOAD_GAME, 0x0688630, eVKey::H3VK_L, 0x06A75F4, isInCombat && networkGame}, // load game
        {target::PAGE_SAVE_GAME, 0x0688624, eVKey::H3VK_S, 0x06A75FC, isInCombat},                // save game
        {target::PAGE_RESTART, 0x0688618, eVKey::H3VK_R, 0x06A7604, isInCombat && networkGame},   // restart the map
        {target::PAGE_MAIN, 0x068860C, eVKey::H3VK_M, 0x06A75EC},                                 // quit to main menu
        {target::PAGE_QUIT, 0x0688600, eVKey::H3VK_Q, 0x06A760C},                                 // quit to desktop
        {30722, 0x0670130, eVKey::H3VK_ESCAPE, DWORD(isInCombat ? 0x06A5614 : 0x06A7614)}, // back to adv map / combat
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
