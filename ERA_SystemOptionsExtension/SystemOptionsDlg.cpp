#include <array>

#include "SystemOptionsDlg.h"

namespace scroll
{
class MapScroller : public IGamePatch
{
  public:
    static MapScroller &Get() noexcept;
};

} // namespace scroll

#define ERA_CAPTION(page, field) EraJS::read("era.opt." #page "." #field)
#define ERA_OPT(page, id, field) "era.opt." #page "." #id "." #field
#define ERA_ARRAY_OPT(page, id, field, i) "era.opt." #page "." #id "." #field "." #i

#define FRAME_COLOR H3RGB888(0x7A, 0x65, 0x48)

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;
SystemOptionsDlg::HealthBarDlgCallInfo SystemOptionsDlg::healthBarDlgInfo;

void __stdcall ShowHealthBarDlg();
BOOL *__stdcall HealthBarIsEnabledAddress();

void DrawThickFrameOverItem(H3LoadedPcx16 *back, H3DlgItem *item)
{
    static auto color = FRAME_COLOR;
    back->DrawThickFrame(item->GetX() - 1, item->GetY() - 1, item->GetWidth() + 2, item->GetHeight() + 2, 1, color);
}

SystemOptionsDlg::SystemOptionsDlg(int width, int height, int x, int y)
    : H3Dlg(width, height, x, y, false, 1, P_Game->GetPlayerID()),
      isInCombat(P_CombatManager->Get() && P_CombatManager->dlg)

{
    if (isInCombat)
        dlgCallSource = COMBAT;
    else if (P_AdventureManager && P_AdventureMgr->dlg)
        dlgCallSource = ADV_MAP;
    else
        dlgCallSource = MAIN_MENU;

    // split dlg by half with thick frame
    const auto color = FRAME_COLOR;
    background->DrawThickFrame(DLG_WIDTH >> 1, DLG_TOPSETTINGS_MARGIN, 1,
                               DLG_HEIGHT - DLG_TOPSETTINGS_MARGIN - DLG_CAPTION_BUTTON_TOP_MARGIN + 15, 1, color);

    for (size_t i = 0; i < 255; i += 10)
    {
        // background->DarkenArea(16, 16 +i, DLG_WIDTH - 32, 10, i);
    }
    //    background->DrawShadow(16,16 +35, DLG_WIDTH - 32, DLG_HEIGHT>>1);
    // create buttons for loading/saving/restarting/quitting the game
    CreateGameControlButtons();

    // create and init page buttons and its actual pages with settings
    CreateDlgPages();

    // RGB565 color565(0x7A, 0x65, 0x48);
    // CreateFrame(DLG_WIDTH >> 1, 40, 2, DLG_HEIGHT - 50, color);

    // draw frames over page caption buttons and copy background to pages' backgrounds
    for (const auto &page : m_pages)
    {
        // DrawThickFrameOverItem(background, page->captionBttn);
    }
    for (const auto &page : m_pages)
    {
        //  libc::memcpy(page->background->buffer, background->buffer, page->background->buffSize);
    }
    instance = this;
}

void SystemOptionsDlg::CreateGameControlButtons() noexcept
{
    // const BOOL isNetworkGame = P_
    using target = Era::EGameMenuTarget;
    const BOOL isMainMenu = dlgCallSource == MAIN_MENU;
    const struct
    {
        const INT32 buttonId;
        const DWORD defNamePtr;
        const eVKey hotkey;
        const DWORD hintPtr;
        const INT32 disableOnCreation = FALSE;
    } gameControlButtons[]{
        {target::PAGE_LOAD_GAME, 0x0688630, eVKey::H3VK_L, 0x06A75F4,
         isInCombat && networkGame || isMainMenu},                                               // load game
        {target::PAGE_SAVE_GAME, 0x0688624, eVKey::H3VK_S, 0x06A75FC, dlgCallSource != ADV_MAP}, // save game
        {target::PAGE_RESTART, 0x0688618, eVKey::H3VK_R, 0x06A7604,
         isInCombat && networkGame || isMainMenu},                                         // restart the map
        {target::PAGE_MAIN, 0x068860C, eVKey::H3VK_M, 0x06A75EC, isMainMenu},              // quit to main menu
        {target::PAGE_QUIT, 0x0688600, eVKey::H3VK_Q, 0x06A760C, isMainMenu},              // quit to desktop
        {30722, 0x0670130, eVKey::H3VK_ESCAPE, DWORD(isInCombat ? 0x06A5614 : 0x06A7614)}, // back to adv map / combat
    };

    constexpr size_t length = std::size(gameControlButtons);
    constexpr int buttonWidth = 100 + 13;
    constexpr int buttonHeight = 48 + 10;
    const size_t startIndex = isMainMenu ? 5 : 0;
    constexpr int frameY = DLG_HEIGHT - buttonHeight * 3 - 20;
    // draw a horizontal thick frame over general buttons
    background->DrawThickFrame(DLG_WIDTH >> 1, frameY, (DLG_WIDTH >> 1) - 20, 1, 1, FRAME_COLOR);

    for (size_t i = startIndex; i < length; i++)
    {
        auto &button = gameControlButtons[i];
        const int x = DLG_WIDTH - buttonWidth * 2 + ((i & 1) * buttonWidth) - 10;
        const int y = DLG_HEIGHT - buttonHeight * 3 + ((i / 2) * buttonHeight) - 12;
        auto bttn = CreateButton(x, y, button.buttonId, LPCSTR(button.defNamePtr), 1, 0, false, button.hotkey);
        if (button.hotkey == eVKey::H3VK_ESCAPE)
        {
            bttn->AddHotkey(eVKey::H3VK_ENTER);
        }
        if (const auto hint = button.hintPtr)
        {
            bttn->SetRightClickHint(EraJS::read(ValueAt<LPCSTR>(hint)));
        }
        if (button.disableOnCreation)
        {
            bttn->Disable();
            //  reinterpret_cast<H3DlgDef*>(bttn)->SendCommand(5, 4096);
            // bttn->Cast<H3DlgDef>()->SendCommand(6, 2);
        }
    }
}
void SystemOptionsDlg::CreateDlgPages() noexcept
{
    OriginalConfig &config = OriginalConfig::Get();
    AdditionalConfig &additionalConfig = AdditionalConfig::Get();

    {
        auto captionBttn = H3DlgCaptionButton::Create(15, DLG_CAPTION_BUTTON_TOP_MARGIN, ePageItemId::PAGE_ITEM_GENERAL,
                                                      BIG_BUTTON, ERA_CAPTION(system, name), NH3Dlg::Text::BIG, 0, 0,
                                                      false, eVKey::H3VK_1, eTextColor::HIGHLIGHT);

        captionBttn->SetRightClickHint(ERA_CAPTION(system, hint));
        auto *page = new SettingsPage(captionBttn);
        int itemId = page->firstItemId;
        auto &pageItems = page->items;

        LPCSTR videoDefNames[] = {LPCSTR(DwordAt(0x05B2300 + 1)), LPCSTR(DwordAt(0x05B22A9 + 1))};
        LPCSTR switchPanelHints[] = {ERA_ARRAY_OPT(system, videoQuality, hints, 0),
                                     ERA_ARRAY_OPT(system, videoQuality, hints, 1)};
        constexpr size_t videoDefNum = std::size(videoDefNames);
        constexpr int switchPanelX = DLG_LEFT_PART_X_MARGIN;

        const int startY = captionBttn->GetY() + captionBttn->GetHeight() + DLG_CAPTION_BUTTON_TOP_MARGIN;

        SwitchPanelInfo switchPanelsInfo = {
            {switchPanelX, startY}, itemId,      ERA_OPT(system, videoQuality, name),
            &config.videoQuality,   videoDefNum, &videoDefNames[0],
            &switchPanelHints[0],
        };

        page->CreateSwitchPanel(switchPanelsInfo);
        itemId += videoDefNum;
        constexpr int checkboxX = DLG_LEFT_PART_X_MARGIN;
        LPCSTR checkboxesHintPtrs[] = {ERA_OPT(system, videoSubtitles, hint), ERA_OPT(system, buildingOutlines, hint),
                                       ERA_OPT(system, spellBookAnimation, hint)};

        const SettingsInfo checkboxesInfo[] = {
            {

                "videoSubtitles",
                {checkboxX, startY + 90},
                itemId++,
                &config.videoSubtitles, //  0x06987D0,
                ERA_OPT(system, videoSubtitles, name),
                ERA_OPT(system, videoSubtitles, hint),

            }, // show tips
            {

                "buildingOutlines",
                {checkboxX, startY + 120},
                itemId++,
                &config.buildingOutlines, // 0x06987D4,
                ERA_OPT(system, buildingOutlines, name),
                ERA_OPT(system, buildingOutlines, hint),

            }, // show tips in battle
            {

                "spellBookAnimation",
                {checkboxX, startY + 150},
                itemId++,
                &config.spellBookAnimation, // 0x06987D8,
                ERA_OPT(system, spellBookAnimation, name),
                ERA_OPT(system, spellBookAnimation, hint),

            } // show tips in battle
        };
        for (auto &info : checkboxesInfo)
        {
            page->CreateCheckBox(info);
        }

        // CREATE CALLBACK BUTTONS
        // wog option buttons:
        const SettingsInfo wogOptionCaption = {"system_wog_option",
                                               {checkboxX, 300},
                                               itemId++,
                                               0,
                                               ERA_OPT(system, wogOptions, name),
                                               ERA_OPT(system, wogOptions, hint)};

        auto captionSetting = page->CreateCaption(wogOptionCaption);
        captionSetting->SetOnChange([](ISetting *) { CallWogOptionsDlg(); });

        // language selection dlg
        auto plugin = GetModuleHandleA("ERA_LocaleManager.era");
        if (plugin)
        {
            typedef void(__stdcall * CallLocaleSelectionDlg_t)(int, int, int);
            typedef const char *(__stdcall * GetDisplayedName_t)();

            auto callDlg = reinterpret_cast<CallLocaleSelectionDlg_t>(GetProcAddress(plugin, "CallLocaleSelectionDlg"));
            auto getDisplayedName = reinterpret_cast<GetDisplayedName_t>(GetProcAddress(plugin, "GetDisplayedName"));

            if (callDlg && getDisplayedName)
            {
                const SettingsInfo selectLang = {
                    "system_select_language", {checkboxX, 330}, itemId++, nullptr, getDisplayedName(),
                };

                captionSetting = page->CreateCaption(selectLang);
                captionSetting->SetOnChange([callDlg, getDisplayedName](ISetting *setting) {
                    auto it = dynamic_cast<CaptionButtonSetting *>(setting)->captionButton;
                    callDlg(it->GetAbsoluteX() + it->GetWidth(), -1, 0);
                    LPCSTR newLangDisplayedName = getDisplayedName();
                    if (libc::strcmpi(it->GetText(), newLangDisplayedName) != 0)
                    {
                        it->SetText(newLangDisplayedName);
                        it->Draw();
                        it->Refresh();
                    }
                });
            }
        }

        // RIGHT PAGE PART
        constexpr int panelX = DLG_RIGHT_PART_X_MARGIN;

        const SettingsInfo rightCheckboxesInfo[] = {{
                                                        additionalConfig.backgroundSound.keyName,
                                                        {panelX, startY},
                                                        itemId++,
                                                        &additionalConfig.backgroundSound.value,
                                                        ERA_OPT(system, backgroundSound, name),
                                                        ERA_OPT(system, backgroundSound, hint),
                                                    }, // show tips
                                                    {
                                                        additionalConfig.buttonSoundSplit.keyName,
                                                        {panelX, startY + 30},
                                                        itemId++,
                                                        &additionalConfig.buttonSoundSplit.value,
                                                        ERA_OPT(system, buttonSoundSplit, name),
                                                        ERA_OPT(system, buttonSoundSplit, hint),
                                                    }};
        for (auto &info : rightCheckboxesInfo)
        {
            page->CreateCheckBox(info);
        }

        constexpr size_t count = Switch10XPanel::BUTTONS_COUNT;
        // combat speed switch panel
        std::string strHints[count << 1];
        auto hintPtrs = [&strHints, count] {
            std::array<LPCSTR, 20> arr{};

            for (size_t i = 0; i < count; i++)
            {
                libc::sprintf(h3_TextBuffer, ERA_OPT(system, musicVolume, hints) ".%d", i);
                strHints[i] = h3_TextBuffer;
                arr[i] = strHints[i].c_str();

                libc::sprintf(h3_TextBuffer, ERA_OPT(system, effectsVolume, hints) ".%d", i);
                strHints[i + count] = h3_TextBuffer;
                arr[i + count] = strHints[i + count].c_str();
            }
            return arr;
        }();

        SettingsInfo switch10PanelsInfo[] = {
            {
                "system_music_level",
                {panelX, startY + 70},
                itemId,
                &config.musicVolume,
                ERA_OPT(system, musicVolume, name),
            }, // music level switch panel
            {
                "system_sound_effects_level",
                {panelX, startY + 140},
                itemId + count,
                &config.effectsVolume, // 0x06987B4,
                ERA_OPT(system, effectsVolume, name),

            } // sound effects level switch panel
        };
        void (*funcs[2])(ISetting *) = {OnMusicVolumeChanged, OnSoundVolumeChanged};

        for (size_t i = 0; i < std::size(switch10PanelsInfo); i++)
        {
            auto &info = switch10PanelsInfo[i];
            info.rmcHints = &hintPtrs[i * count];
            auto panel = page->Create10XPanel(switch10PanelsInfo[i]);
            panel->SetOnChange(funcs[i]);
        }

        itemId += Switch10XPanel::BUTTONS_COUNT << 1;

        AddItem(captionBttn);
        m_pages.Add(page);
    }

    {

        auto captionBttn = H3DlgCaptionButton::Create(
            146 + 6 + 15, DLG_CAPTION_BUTTON_TOP_MARGIN, ePageItemId::PAGE_ITEM_ADV_MAP, BIG_BUTTON,
            ERA_CAPTION(map, name), NH3Dlg::Text::BIG, 0, false, false, eVKey::H3VK_2, eTextColor::HIGHLIGHT);
        captionBttn->SetRightClickHint(ERA_CAPTION(map, hint));

        auto *page = new SettingsPage(captionBttn);
        AddItem(captionBttn);

        int itemId = page->firstItemId;
        auto &pageItems = page->items;

        constexpr int switchPanelX = DLG_LEFT_PART_X_MARGIN;
        const int startY = captionBttn->GetY() + captionBttn->GetHeight() + DLG_CAPTION_BUTTON_TOP_MARGIN;

        LPCSTR playerSpeedDefNames[] = {ValueAt<LPCSTR>(0x005B1EEF + 1), ValueAt<LPCSTR>(0x005B1F43 + 1),
                                        ValueAt<LPCSTR>(0x005B1F97 + 1), ValueAt<LPCSTR>(0x005B1FEB + 1)};
        LPCSTR enemySpeedDefNames[] = {ValueAt<LPCSTR>(0x05B2042 + 1), ValueAt<LPCSTR>(0x05B2099 + 1),
                                       ValueAt<LPCSTR>(0x05B20F0 + 1), ValueAt<LPCSTR>(0x05B2147 + 1)};
        LPCSTR mapScrollDefNames[] = {ValueAt<LPCSTR>(0x05B21A1 + 1), ValueAt<LPCSTR>(0x05B21F8 + 1),
                                      ValueAt<LPCSTR>(0x05B224F + 1)};
        LPCSTR switchPanelHints[] = {
            ERA_ARRAY_OPT(map, playerSpeed, hints, 0), ERA_ARRAY_OPT(map, playerSpeed, hints, 1),
            ERA_ARRAY_OPT(map, playerSpeed, hints, 2), ERA_ARRAY_OPT(map, playerSpeed, hints, 3),

            ERA_ARRAY_OPT(map, enemySpeed, hints, 0),  ERA_ARRAY_OPT(map, enemySpeed, hints, 1),
            ERA_ARRAY_OPT(map, enemySpeed, hints, 2),  ERA_ARRAY_OPT(map, enemySpeed, hints, 3),

            ERA_ARRAY_OPT(map, scrollSpeed, hints, 0), ERA_ARRAY_OPT(map, scrollSpeed, hints, 1),
            ERA_ARRAY_OPT(map, scrollSpeed, hints, 2)};

        constexpr size_t playerDefNum = std::size(playerSpeedDefNames);
        constexpr size_t enemyDefNum = std::size(enemySpeedDefNames);
        constexpr size_t mapScrollDefNum = std::size(mapScrollDefNames);
        SwitchPanelInfo switchPanelsInfo[] = {
            {{switchPanelX, startY},
             itemId,
             ERA_OPT(map, playerSpeed, name), //  P_GeneralText->GetText(571),
             &config.playerSpeed,             //(int*)0x06987AC,
             playerDefNum,
             &playerSpeedDefNames[0],
             &switchPanelHints[0],
             0},
            {{switchPanelX, startY + 70},
             itemId + playerDefNum,
             ERA_OPT(map, enemySpeed, name), //    P_GeneralText->GetText(572),
             &config.enemySpeed,             // 0x06987A8,
             enemyDefNum,
             &enemySpeedDefNames[0],
             &switchPanelHints[playerDefNum],
             0},
            {{switchPanelX, startY + 140},
             itemId + playerDefNum + enemyDefNum,
             ERA_OPT(map, scrollSpeed, name), //     P_GeneralText->GetText(573),
             &config.mapScrollSpeed,          // 0x06987DC,
             mapScrollDefNum,
             &mapScrollDefNames[0],
             &switchPanelHints[playerDefNum + enemyDefNum]},
        };
        constexpr size_t switchPanelNum = std::size(switchPanelsInfo);
        void (*funcs[switchPanelNum])(ISetting *) = {OnPlayerSpeedButtonClicked, OnEnemySpeedButtonClicked, nullptr};
        for (size_t i = 0; i < switchPanelNum; i++)
        {
            auto &info = switchPanelsInfo[i];
            auto panel = page->CreateSwitchPanel(info);
            panel->SetOnChange(funcs[i]);
        }

        for (auto &info : switchPanelsInfo)
        {
        }

        itemId += playerDefNum + enemyDefNum + mapScrollDefNum;
        // create checkboxes
        constexpr int checkboxX = DLG_LEFT_PART_X_MARGIN;
        int checkboxY = 280;
        const DWORD checkboxesHintPtrs[] = {DWORD(ERA_OPT(map, showRoute, hint)),
                                            DWORD(ERA_OPT(map, moveReminder, hint)),
                                            DWORD(ERA_OPT(map, autoSave, hint))};

        SettingsInfo checkboxesInfo[] = {
            {

                "adventure_show_route",
                {checkboxX, checkboxY + 30},
                itemId++,
                &config.showRoute, // 0x06987C4,
                ERA_OPT(map, showRoute, name),
                ERA_OPT(map, showRoute, hint),

            }, // show tips
            {

                "adventure_hero_reminder",
                {checkboxX, checkboxY + 60},
                itemId++,
                &config.moveReminder, //  0x06987C8,
                ERA_OPT(map, moveReminder, name),
                ERA_OPT(map, moveReminder, hint),
                P_Game->inTutorial

            }, // show tips in battle
            {

                "adventure_game_autosave",
                {checkboxX, checkboxY + 90},
                itemId++,
                &config.autoSave, //   0x06987C0,
                ERA_OPT(map, autoSave, name),
                ERA_OPT(map, autoSave, hint),

            }, // show tips in battle
            {

                "SmoothScroll",
                {checkboxX, checkboxY + 120},
                itemId++,
                &additionalConfig.smoothMapScroll.value, //   0x06987C0,
                ERA_OPT(map, smoothScroll, name),
                ERA_OPT(map, smoothScroll, hint),

            }, // show tips in battle
        };

        for (auto &info : checkboxesInfo)
        {
            page->CreateCheckBox(info);
        }
        auto &smoothScroll = *page->settings.Last();
        smoothScroll->SetOnChange(
            [](ISetting *setting) { scroll::MapScroller::Get().SetEnabled(setting->value.current); });
        m_pages.Add(page);
    }

    // COMBAT PAGE

    {
        auto captionBttn = H3DlgCaptionButton::Create(
            (146 + 6 << 1) + 15, DLG_CAPTION_BUTTON_TOP_MARGIN, ePageItemId::PAGE_ITEM_COMBAT, BIG_BUTTON,
            ERA_CAPTION(combat, name), NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_3, eTextColor::HIGHLIGHT);
        captionBttn->SetRightClickHint(ERA_CAPTION(combat, hint));

        auto page = new SettingsPage(captionBttn);
        AddItem(captionBttn);

        constexpr int x = DLG_LEFT_PART_X_MARGIN;
        const int startY = captionBttn->GetY() + captionBttn->GetHeight() + 12;
        // standard checkboxes
        int itemId = page->firstItemId;
        auto &pageItems = page->items;

        const SettingsInfo checkboxesInfo[] = {
            {
                "show_grid",
                {x, startY},
                itemId++,
                &config.showHexGrid, // 0x069880C,
                ERA_OPT(combat, showHexGrid, name),
                ERA_OPT(combat, showHexGrid, hint),

            },
            {

                "movements_shadow",
                {x, startY + 30},
                itemId++,
                &config.movementShadow, //  0x0698814,
                ERA_OPT(combat, movementShadow, name),
                ERA_OPT(combat, movementShadow, hint)

            }, // show movements_shadow
            {

                "cursor_shadow",
                {x, startY + 60},
                itemId++,
                &config.cursorShadow, //  0x0698810,
                ERA_OPT(combat, cursorShadow, name),
                ERA_OPT(combat, cursorShadow, hint)

            },
            {

                "auto_combat_creatures",
                {x, startY + 120},
                itemId++,
                &config.autoCreatures, //  0x06987E4,
                ERA_OPT(combat, autoCreatures, name),
                ERA_OPT(combat, autoCreatures, hint)

            }, // show combat messages
            {

                "auto_combat_spells",
                {x, startY + 150},
                itemId++,
                &config.autoSpells, //   0x06987E8,
                ERA_OPT(combat, autoSpells, name),
                ERA_OPT(combat, autoSpells, hint)

            }, // show combat animations
            {

                "auto_combat_catapult",
                {x, startY + 180},
                itemId++,
                &config.autoCatapult, //  0x06987EC,
                ERA_OPT(combat, autoCatapult, name),
                ERA_OPT(combat, autoCatapult, hint)
                //

            }, // show floating combat text
            {

                "auto_combat_ballista",
                {x, startY + 210},
                itemId++,
                &config.autoBallista, //  0x06987F0,
                ERA_OPT(combat, autoBallista, name),
                ERA_OPT(combat, autoBallista, hint)

            },
            {

                "auto_combat_tent",
                {x, startY + 240},
                itemId++,
                &config.autoFirstAidTent, // 0x06987F4,
                ERA_OPT(combat, autoFirstAidTent, name),
                ERA_OPT(combat, autoFirstAidTent, hint)

            }, // show battle interface

        };

        for (auto &info : checkboxesInfo)
        {
            page->CreateCheckBox(info);
        }
        constexpr size_t count = Switch10XPanel::BUTTONS_COUNT;
        // combat speed switch panel
        std::string strHints[count];
        auto hintPtrs = [&strHints] {
            std::array<LPCSTR, Switch10XPanel::BUTTONS_COUNT> arr{};

            for (size_t i = 0; i < Switch10XPanel::BUTTONS_COUNT; i++)
            {
                libc::sprintf(h3_TextBuffer, ERA_OPT(combat, animationSpeed, hints) ".%d", i);
                strHints[i] = h3_TextBuffer;
                arr[i] = strHints[i].c_str();
            }
            return arr;
        }();
        constexpr int ySwitch = DLG_HEIGHT - Switch10XPanel::HEIGHT - 25;

        SettingsInfo switch10xPanelsInfo{
            "combat_animation_speed",
            {x, ySwitch},
            itemId,
            &config.animationSpeed, //  H3CurrentAnimationSpeed::ADDRESS,
            ERA_OPT(combat, animationSpeed, name),
        };
        switch10xPanelsInfo.rmcHints = &hintPtrs[0];

        itemId += count;
        page->Create10XPanel(switch10xPanelsInfo);

        constexpr int rightX = DLG_RIGHT_PART_X_MARGIN;

        const auto healthBarValuePtr = HealthBarIsEnabledAddress();
        const SettingsInfo healthBarcheckBoxInfo = {"system_health_bar_checkbox",
                                                    {rightX, startY + 60},
                                                    itemId++,
                                                    healthBarValuePtr,
                                                    ERA_OPT(combat, healthBar, name),
                                                    ERA_OPT(combat, healthBar, hint),
                                                    P_Game->inTutorial};
        auto &info = instance->healthBarDlgInfo;
        info.healthBarValuePtr = healthBarValuePtr;
        auto healthBarcheckBox = page->CreateCheckBox(healthBarcheckBoxInfo);
        info.affectedCheckbox = healthBarcheckBox->checkBoxItem;
        info.dlgValuePtr = &healthBarcheckBox->value.current;

        const SettingsInfo healthBarCaptionInfo = {"system_health_bar",
                                                   {rightX, startY + 90},
                                                   itemId++,
                                                   nullptr,
                                                   ERA_OPT(combat, healthBar, button),
                                                   ERA_OPT(combat, healthBar, hint)};
        auto button = page->CreateCaption(healthBarCaptionInfo);
        button->SetOnChange(CallHealthBarDlg);
        m_pages.Add(page);
    }
    InitDlgPages();
}

BOOL SystemOptionsDlg::OnCreate()
{
    ePageIndex pageIndex = PAGE_INDEX_GENERAL;
    switch (dlgCallSource)
    {
    case ADV_MAP:
        pageIndex = PAGE_INDEX_ADV_MAP;
        break;
    case COMBAT:
        pageIndex = PAGE_INDEX_COMBAT;
        break;
    default:
        pageIndex = PAGE_INDEX_GENERAL;
        break;
    }

    SetActivePage(pageIndex, FALSE);
    return 1;
}

BOOL SystemOptionsDlg::DialogProc(H3Msg &msg)
{
    const int itemId = msg.itemId;
    if (itemId < 1)
    {
        return 1;
    }
    if (msg.IsRightClick())
    {
        auto it = GetH3DlgItem(itemId);
        if (it && it->GetRightClickHint())
        {
            H3Messagebox::RMB(it->GetRightClickHint());
            return 0;
        }
    }
    else if (m_currentPage && m_currentPage->ProcessMessage(msg))
    {
        // end processing if message was processed by page
        return 0;
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
    case ePageItemId::PAGE_ITEM_COMBAT:
        SetActivePage(itemId - 1, TRUE);
        break;
    case target::PAGE_LOAD_GAME: // these 3 reqquires user agreement
    case target::PAGE_QUIT:      // since they can cause loss of unsaved progress
    case target::PAGE_MAIN:
        if (!H3Messagebox::Choice(P_GeneralText->GetText(580)))
            break;
    case target::PAGE_SAVE_GAME:
    case target::PAGE_RESTART:
    case 30722:
        this->resultItemId = itemId;
        this->Stop();
        break;
    default:
        break;
    }

    return TRUE;
}

void __stdcall SystemOptionsDlg::CallWogOptionsDlg()
{
    const BOOL allowWogOptionsChanges = instance->dlgCallSource == MAIN_MENU && !instance->networkGame;
    const int storeValue = IntAt(0x291A430);
    IntAt(0x291A430) = allowWogOptionsChanges;
    auto jumpOverMouseCheck = _PI->WriteJmp(0x07790FB, 0x0779157);
    STDCALL_0(VOID, 0x597AA0); // stop video animation
    THISCALL_0(int, 0x07790E1);
    STDCALL_0(VOID, 0x597B50); // resume video animation
    jumpOverMouseCheck->Destroy();
    IntAt(0x291A430) = storeValue;
}

void __stdcall SystemOptionsDlg::CallHealthBarDlg(ISetting *sender)
{
    auto &info = instance->healthBarDlgInfo;
    const int checkboxStateBefore = *(info.healthBarValuePtr);
    ShowHealthBarDlg();
    const int checkboxStateAfter = *(info.healthBarValuePtr);
    if (checkboxStateBefore != checkboxStateAfter)
    {
        *(info.dlgValuePtr) = checkboxStateAfter;
        auto it = info.affectedCheckbox;
        it->SetFrame(checkboxStateAfter);
        it->Draw();
        it->Refresh();
    }
}
void SystemOptionsDlg::OnEnemySpeedButtonClicked(ISetting *sender)
{
    OriginalConfig::Get().blackoutComputer = sender->value.current == 5;
}
void SystemOptionsDlg::AfterDlgClose()
{
}
SystemOptionsDlg::~SystemOptionsDlg()
{
    for (auto &page : m_pages)
    {
        for (auto &setting : page->settings)
        {
            auto &value = setting->value;
            if (value.current != value.dlgStart)
            {
                *(value.valuePtr) = value.current;
                settingsChanged = TRUE;
                // if (dynamic_cast<CheckBoxSetting *>(setting))
                //{
                //     setting->TriggerChange();
                // }
            }
        }
        delete page;
    }
    if (settingsChanged)
    {
        const int newQuickCombatState = OriginalConfig::Get().quickCombat;
        // if quick combat settings were changed
        if (dlgCallSource == ADV_MAP && networkGame && quickCombatSettingState != newQuickCombatState)
        {
            P_Main->GetPlayer()->quickCombatEnabled = newQuickCombatState;
            H3NetworkData<int> netMsg(-1, eNetwork::PLAYER_QUICK, newQuickCombatState);
            netMsg.SendData(false);
        }
        AdditionalConfig::Save();
        CDECL_0(LONG, 0x0050C370); // j_WriteRegistry -> save settings to heroes3.ini
    }

    instance = nullptr;

    P_WindowManager->resultItemID = this->resultItemId;
}
