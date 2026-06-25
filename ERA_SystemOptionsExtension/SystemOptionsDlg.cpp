#include "SystemOptionsDlg.h"
#include "SoundSettings.h"

#pragma comment(linker, "/EXPORT:RegisterErmCallbackButton=_RegisterErmCallbackButton@16")
#pragma comment(linker, "/EXPORT:UnregisterErmCallbackButton=_UnregisterErmCallbackButton@4")

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

#define FRAME_LIGHT_COLOR H3RGB888(0x7A, 0x65, 0x48)
#define FRAME_DARK_COLOR H3RGB888(0x31, 0x21, 0x10)

SystemOptionsDlg *SystemOptionsDlg::instance = nullptr;
std::unordered_map<std::string, SystemOptionsDlg::RegisteredErmButtonInfo> SystemOptionsDlg::registeredErmButtons;

DllExport BOOL __stdcall RegisterErmCallbackButton(const char *tag, const char *name, const char *description,
                                                   int ermFunctionId)
{
    const BOOL isNew = SystemOptionsDlg::registeredErmButtons.find(tag) == SystemOptionsDlg::registeredErmButtons.end();
    SystemOptionsDlg::registeredErmButtons[tag] = {name, description, ermFunctionId};
    return isNew;
}
DllExport BOOL __stdcall UnregisterErmCallbackButton(const char *tag)
{
    const BOOL isNew = SystemOptionsDlg::registeredErmButtons.find(tag) == SystemOptionsDlg::registeredErmButtons.end();
    if (!isNew)
    {
        SystemOptionsDlg::registeredErmButtons.erase(tag);
    }
    return !isNew;
}

void __stdcall ShowHealthBarDlg();
BOOL *__stdcall HealthBarIsEnabledAddress();

void DrawThickFrameOverItem(H3LoadedPcx16 *back, H3DlgItem *item)
{
    static auto color = FRAME_LIGHT_COLOR;
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
    constexpr int dlgCenterX = DLG_WIDTH >> 1;
    background->DrawThickFrame(dlgCenterX, DLG_TOPSETTINGS_MARGIN, 1,
                               DLG_HEIGHT - DLG_TOPSETTINGS_MARGIN - DLG_CAPTION_BUTTON_TOP_MARGIN, 1,
                               FRAME_LIGHT_COLOR);
    background->DrawThickFrame(dlgCenterX + 1, DLG_TOPSETTINGS_MARGIN, 1,
                               DLG_HEIGHT - DLG_TOPSETTINGS_MARGIN - DLG_CAPTION_BUTTON_TOP_MARGIN, 1,
                               FRAME_DARK_COLOR);

    // create buttons for loading/saving/restarting/quitting the game
    CreateGameControlButtons();

    // create and init page buttons and its actual pages with settings
    CreateDlgPages();

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
        const LPCSTR hintPtr;
        const INT32 disableOnCreation = FALSE;
    } gameControlButtons[]{
        {target::PAGE_LOAD_GAME, 0x0688630, eVKey::H3VK_L, ERA_OPT(common, load, hint),
         isInCombat && networkGame || isMainMenu}, // load game
        {target::PAGE_SAVE_GAME, 0x0688624, eVKey::H3VK_S, ERA_OPT(common, save, hint),
         dlgCallSource != ADV_MAP}, // save game
        {target::PAGE_RESTART, 0x0688618, eVKey::H3VK_R, ERA_OPT(common, restart, hint),
         isInCombat && networkGame || isMainMenu},                                              // restart the map
        {target::PAGE_MAIN, 0x068860C, eVKey::H3VK_M, ERA_OPT(common, menu, hint), isMainMenu}, // quit to main menu
        {target::PAGE_QUIT, 0x0688600, eVKey::H3VK_Q, ERA_OPT(common, quit, hint), isMainMenu}, // quit to desktop
        {30722, 0x0670130, eVKey::H3VK_ESCAPE,
         isInCombat ? ERA_OPT(common, return, hint) : ERA_OPT(common, return, hint)}, // back to adv map / combat
    };

    constexpr size_t length = std::size(gameControlButtons);
    constexpr int buttonWidth = 100 + 13;
    constexpr int buttonHeight = 48 + 10;
    const size_t startIndex = isMainMenu ? 5 : 0; // don't create
    constexpr int frameY = DLG_HEIGHT - buttonHeight * 3 - 20;
    // draw a horizontal thick frame over general buttons

    constexpr int dlgCenterX = DLG_WIDTH >> 1;

    background->DrawThickFrame(dlgCenterX + 1, frameY, (DLG_WIDTH >> 1) - 20, 1, 1, FRAME_LIGHT_COLOR);
    background->DrawThickFrame(dlgCenterX + 2, frameY + 1, (DLG_WIDTH >> 1) - 20, 1, 1, FRAME_DARK_COLOR);

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
            bttn->SetRightClickHint(EraJS::read(hint));
        }
        if (button.disableOnCreation)
        {
            bttn->Disable();
        }
    }
}
void SystemOptionsDlg::CreateDlgPages() noexcept
{

    OriginalConfig &config = OriginalConfig::Get();
    AdditionalConfig &extraConfig = AdditionalConfig::Get();
    const int callType = dlgCallSource;

    if (callType != eDlgCallSource::COMBAT)
    {
        if (quickCombatSettingState == FALSE && extraConfig.quickCombatType.value != FALSE ||
            quickCombatSettingState != FALSE && extraConfig.quickCombatType.value == FALSE)
        {
            extraConfig.quickCombatType.value = quickCombatSettingState;
            settingsChanged = TRUE;
        }
    }

    constexpr int baseSettingHeight = ISetting::BASE_SETTINGS_Y_OFFSET;
    constexpr int leftStartX = DLG_LEFT_PART_X_MARGIN;
    constexpr int rightStartX = DLG_RIGHT_PART_X_MARGIN;
    constexpr int settingsStartY = DLG_TOPSETTINGS_MARGIN;
    const BOOL isTutorial = P_Game->inTutorial;

    // GENERAL PAGE
    {
        // LEFT PAGE PART
        auto captionBttn =
            H3DlgCaptionButton::Create(DLG_CAPTION_BUTTON_TOP_MARGIN, DLG_CAPTION_BUTTON_TOP_MARGIN,
                                       ePageItemId::PAGE_ITEM_GENERAL, PAGE_CAPTION_DEF_NAME, ERA_CAPTION(system, name),
                                       NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_1, eTextColor::HIGHLIGHT);

        captionBttn->SetRightClickHint(ERA_CAPTION(system, hint));
        auto *page = new SettingsPage(captionBttn);
        int itemId = page->firstItemId;
        auto &pageItems = page->items;

        LPCSTR videoDefNames[] = {LPCSTR(DwordAt(0x05B2300 + 1)), LPCSTR(DwordAt(0x05B22A9 + 1))};
        LPCSTR switchPanelHints[] = {ERA_ARRAY_OPT(system, videoQuality, hints, 0),
                                     ERA_ARRAY_OPT(system, videoQuality, hints, 1)};
        constexpr size_t videoDefNum = std::size(videoDefNames);
        constexpr int switchPanelX = leftStartX;

        SwitchPanelInfo switchPanelsInfo = {
            {switchPanelX, settingsStartY},
            itemId,
            &config.videoQuality,
            ERA_OPT(system, videoQuality, name),
            videoDefNum,
            &videoDefNames[0],
            &switchPanelHints[0],
        };

        page->CreateSetting<SwitchPanel>(switchPanelsInfo);
        itemId += videoDefNum;
        constexpr int checkboxX = leftStartX;
        LPCSTR checkboxesHintPtrs[] = {ERA_OPT(system, videoSubtitles, hint), ERA_OPT(system, buildingOutlines, hint),
                                       ERA_OPT(system, spellBookAnimation, hint)};

        const int videoCheckboxY = settingsStartY + baseSettingHeight * 2 + ISetting::TITLE_Y_OFFSET;
        const SettingsInfo checkboxesInfo[] = {
            {"videoSubtitles",
             {checkboxX, videoCheckboxY},
             itemId++,
             &config.videoSubtitles, //  0x06987D0,
             ERA_OPT(system, videoSubtitles, name),
             ERA_OPT(system, videoSubtitles, hint)}, // show tips
            {"buildingOutlines",
             {checkboxX, videoCheckboxY + baseSettingHeight * 1},
             itemId++,
             &config.buildingOutlines, // 0x06987D4,
             ERA_OPT(system, buildingOutlines, name),
             ERA_OPT(system, buildingOutlines, hint)}, // show tips in battle
            {"spellBookAnimation",
             {checkboxX, videoCheckboxY + baseSettingHeight * 2},
             itemId++,
             &config.spellBookAnimation, // 0x06987D8,
             ERA_OPT(system, spellBookAnimation, name),
             ERA_OPT(system, spellBookAnimation, hint)} // show tips in battle
        };
        for (auto &info : checkboxesInfo)
        {
            page->CreateSetting<CheckBoxSetting>(info);
        }

        // CREATE CALLBACK BUTTONS
        CreateOtherSettingsPanel(page, checkboxX, settingsStartY + baseSettingHeight * 7, itemId);

        // RIGHT PAGE PART
        constexpr int panelX = DLG_RIGHT_PART_X_MARGIN;

        int rightPartY = settingsStartY;
        page->CreateTitle(panelX, rightPartY, ERA_OPT(system, soundSettings, name));

        const SettingsInfo backgroundSoundInfo = {extraConfig.backgroundSound.keyName,
                                                  {panelX, rightPartY},
                                                  itemId++,
                                                  &extraConfig.backgroundSound.value,
                                                  ERA_OPT(system, backgroundSound, name),
                                                  ERA_OPT(system, backgroundSound, hint)};

        auto bgSoundSetting = page->CreateSetting<CheckBoxSetting>(backgroundSoundInfo);
        bgSoundSetting->SetOnDlgClose([callType](ISetting *setting) {
            if (callType == ADV_MAP)
                sound::SoundSettings::StopBackgroundSounds(setting);
            else
                sound::SoundSettings::SetBackgroundSoundsState(setting->value.current);
        });

        const SettingsInfo splitButtonSoundInfo{extraConfig.alternativeButtonClick.keyName,
                                                {panelX, rightPartY + baseSettingHeight},
                                                itemId++,
                                                &extraConfig.alternativeButtonClick.value,
                                                ERA_OPT(system, alternativeButtonClick, name),
                                                ERA_OPT(system, alternativeButtonClick, hint)};

        auto splitButtonSoundSetting = page->CreateSetting<CheckBoxSetting>(splitButtonSoundInfo);
        ;
        splitButtonSoundSetting->SetOnChange(
            [](ISetting *setting) { sound::SoundSettings::SetAlternativButtonClickState(setting->value.current); });

        constexpr size_t count = Switch10XPanel::BUTTONS_COUNT;
        // combat speed switch panel
        std::string strHints[count << 1];

        LPCSTR hintPtrs[count << 1]{};
        for (size_t i = 0; i < count; i++)
        {
            libc::sprintf(h3_TextBuffer, ERA_OPT(system, musicVolume, hints) ".%d", i);
            strHints[i] = h3_TextBuffer;
            hintPtrs[i] = strHints[i].c_str();

            libc::sprintf(h3_TextBuffer, ERA_OPT(system, effectsVolume, hints) ".%d", i);
            strHints[i + count] = h3_TextBuffer;
            hintPtrs[i + count] = strHints[i + count].c_str();
        }

        SettingsInfo switch10PanelsInfo[] = {
            {"system_music_level",
             {panelX, rightPartY + baseSettingHeight * 2},
             itemId,
             &config.musicVolume,
             ERA_OPT(system, musicVolume, name)}, // music level switch panel
            {"system_sound_effects_level",
             {panelX, rightPartY + baseSettingHeight * 2 + 70},
             itemId + count,
             &config.effectsVolume,                // 0x06987B4,
             ERA_OPT(system, effectsVolume, name)} // sound effects level switch panel
        };
        void (*funcs[2])(ISetting *) = {sound::SoundSettings::OnMusicVolumeChanged,
                                        sound::SoundSettings::OnSoundVolumeChanged};

        for (size_t i = 0; i < std::size(switch10PanelsInfo); i++)
        {
            auto &info = switch10PanelsInfo[i];
            info.rmcHints = &hintPtrs[i * count];
            auto panel = page->CreateSetting<Switch10XPanel>(info);
            panel->SetOnChange(funcs[i]);
        }

        itemId += Switch10XPanel::BUTTONS_COUNT << 1;

        AddItem(captionBttn);
        m_pages.Add(page);
    }

    // ADV MAP PAGE
    {
        // LEFT PAGE PART
        auto captionBttn =
            H3DlgCaptionButton::Create(146 + 4 + DLG_CAPTION_BUTTON_TOP_MARGIN, DLG_CAPTION_BUTTON_TOP_MARGIN,
                                       ePageItemId::PAGE_ITEM_ADV_MAP, PAGE_CAPTION_DEF_NAME, ERA_CAPTION(map, name),
                                       NH3Dlg::Text::BIG, 0, false, false, eVKey::H3VK_2, eTextColor::HIGHLIGHT);
        captionBttn->SetRightClickHint(ERA_CAPTION(map, hint));

        auto *page = new SettingsPage(captionBttn);
        AddItem(captionBttn);

        int itemId = page->firstItemId;
        auto &pageItems = page->items;

        constexpr int switchPanelX = leftStartX;

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
            {{switchPanelX, settingsStartY},
             itemId,
             &config.playerSpeed,             //(int*)0x06987AC,
             ERA_OPT(map, playerSpeed, name), //  P_GeneralText->GetText(571),
             playerDefNum,
             &playerSpeedDefNames[0],
             &switchPanelHints[0],
             1},
            {{switchPanelX, settingsStartY + 70},
             itemId + playerDefNum,
             &config.enemySpeed,             // 0x06987A8,
             ERA_OPT(map, enemySpeed, name), //    P_GeneralText->GetText(572),
             enemyDefNum,
             &enemySpeedDefNames[0],
             &switchPanelHints[playerDefNum],
             2},
            {{switchPanelX, settingsStartY + 140},
             itemId + playerDefNum + enemyDefNum,
             &config.mapScrollSpeed,          // 0x06987DC,
             ERA_OPT(map, scrollSpeed, name), //     P_GeneralText->GetText(573),
             mapScrollDefNum,
             &mapScrollDefNames[0],
             &switchPanelHints[playerDefNum + enemyDefNum]},
        };
        for (auto &info : switchPanelsInfo)
            page->CreateSetting<SwitchPanel>(info);

        itemId += playerDefNum + enemyDefNum + mapScrollDefNum;
        // create checkboxes
        constexpr int checkboxX = leftStartX;

        //	page->CreateTitle(checkboxX, settingsStartY + baseSettingHeight * 5, ERA_OPT(map, miscSettings, name));

        const int checkboxY = settingsStartY + baseSettingHeight * 5 + ISetting::TITLE_Y_OFFSET;
        const DWORD checkboxesHintPtrs[] = {DWORD(ERA_OPT(map, showRoute, hint)),
                                            DWORD(ERA_OPT(map, moveReminder, hint)),
                                            DWORD(ERA_OPT(map, autoSave, hint))};

        SettingsInfo checkboxesInfo[] = {
            {"adventure_show_route",
             {checkboxX, checkboxY + baseSettingHeight},
             itemId++,
             &config.showRoute, // 0x06987C4,
             ERA_OPT(map, showRoute, name),
             ERA_OPT(map, showRoute, hint)}, // show tips
            {"adventure_hero_reminder",
             {checkboxX, checkboxY + baseSettingHeight * 2},
             itemId++,
             &config.moveReminder, //  0x06987C8,
             ERA_OPT(map, moveReminder, name),
             ERA_OPT(map, moveReminder, hint),
             isTutorial}, // show tips in battle
            {"adventure_game_autosave",
             {checkboxX, checkboxY + baseSettingHeight * 3},
             itemId++,
             &config.autoSave, //   0x06987C0,
             ERA_OPT(map, autoSave, name),
             ERA_OPT(map, autoSave, hint)}, // show tips in battle
            {extraConfig.smoothMapScroll.keyName,
             {checkboxX, checkboxY + baseSettingHeight * 4},
             itemId++,
             &extraConfig.smoothMapScroll.value, //   0x06987C0,
             ERA_OPT(map, smoothMapScroll, name),
             ERA_OPT(map, smoothMapScroll, hint)}, // show tips in battle
        };

        for (auto &info : checkboxesInfo)
        {
            page->CreateSetting<CheckBoxSetting>(info);
        }
        auto &smoothScroll = *page->settings.Last();
        smoothScroll->SetOnDlgClose([callType](ISetting *setting) {
            const BOOL value = setting->value.current;
            scroll::MapScroller::Get().SetEnabled(value);
            if (!value && callType == eDlgCallSource::ADV_MAP)
            {
                P_AdventureManager->screenDrawOffset = {};
            }
        });

        // RIGHT PAGE PART
        constexpr int rCheckboxX = DLG_RIGHT_PART_X_MARGIN;
        int rCheckboxY = settingsStartY;

        LPCSTR radioTexts[] = {ERA_OPT(map, quickCombatManual, name), ERA_OPT(map, quickCombatMana, name),
                               ERA_OPT(map, quickCombatManaFree, name), ERA_OPT(map, quickCombatAsk, name)};
        LPCSTR radioHints[] = {ERA_OPT(map, quickCombatManual, hint), ERA_OPT(map, quickCombatMana, hint),
                               ERA_OPT(map, quickCombatManaFree, hint), ERA_OPT(map, quickCombatAsk, hint)};
        constexpr int radioNums = std::size(radioTexts);
        RadioButtonInfo radioInfo = {"quick_combat_type",
                                     {rCheckboxX, rCheckboxY},
                                     itemId,
                                     &extraConfig.quickCombatType.value,
                                     ERA_OPT(map, quickCombat, name),
                                     radioNums,
                                     radioTexts,
                                     radioHints,
                                     TRUE};
        itemId += radioNums;
        auto radioBox = page->CreateSetting<RadioBoxSetting>(radioInfo);
        radioBox->value.isBlocked = isTutorial || callType == eDlgCallSource::COMBAT;
        // radioBox->SetOnDlgClose([callType](ISetting *sender) {
        //     if (callType == eDlgCallSource::COMBAT)
        //     {
        //         // sender->value.current = sender->value.dlgStart;
        //         *(sender->value.valuePtr) = sender->value.dlgStart;
        //     }
        // });
        m_pages.Add(page);
    }

    // COMBAT PAGE
    if (1)
    {
        // LEFT PAGE PART
        auto captionBttn =
            H3DlgCaptionButton::Create((146 + 4 << 1) + DLG_CAPTION_BUTTON_TOP_MARGIN, DLG_CAPTION_BUTTON_TOP_MARGIN,
                                       ePageItemId::PAGE_ITEM_COMBAT, PAGE_CAPTION_DEF_NAME, ERA_CAPTION(combat, name),
                                       NH3Dlg::Text::BIG, 0, 0, false, eVKey::H3VK_3, eTextColor::HIGHLIGHT);
        captionBttn->SetRightClickHint(ERA_CAPTION(combat, hint));

        auto page = new SettingsPage(captionBttn);
        AddItem(captionBttn);

        constexpr int x = leftStartX;

        // standard checkboxes
        int itemId = page->firstItemId;
        int leftPartY = settingsStartY;
        page->CreateTitle(x, leftPartY, ERA_OPT(combat, gridSettings, name));

        const SettingsInfo gridSettingsInfo[] = {{"show_grid",
                                                  {x, leftPartY},
                                                  itemId++,
                                                  &config.showHexGrid, // 0x069880C,
                                                  ERA_OPT(combat, showHexGrid, name),
                                                  ERA_OPT(combat, showHexGrid, hint)},
                                                 {"movements_shadow",
                                                  {x, leftPartY + baseSettingHeight},
                                                  itemId++,
                                                  &config.movementShadow, //  0x0698814,
                                                  ERA_OPT(combat, movementShadow, name),
                                                  ERA_OPT(combat, movementShadow, hint)},
                                                 {"cursor_shadow",
                                                  {x, leftPartY + baseSettingHeight * 2},
                                                  itemId++,
                                                  &config.cursorShadow, //  0x0698810,
                                                  ERA_OPT(combat, cursorShadow, name),
                                                  ERA_OPT(combat, cursorShadow, hint)}};

        for (auto &info : gridSettingsInfo)
        {
            page->CreateSetting<CheckBoxSetting>(info);
        }

        leftPartY += baseSettingHeight * 3;
        page->CreateTitle(x, leftPartY, ERA_OPT(combat, autoCombat, name));
        const SettingsInfo autoCombatInfo[] = {{"auto_combat_creatures",
                                                {x, leftPartY},
                                                itemId++,
                                                &config.autoCreatures, //  0x06987E4,
                                                ERA_OPT(combat, autoCreatures, name),
                                                ERA_OPT(combat, autoCreatures, hint)}, // show combat messages
                                               {"auto_combat_spells",
                                                {x, leftPartY + baseSettingHeight},
                                                itemId++,
                                                &config.autoSpells, //   0x06987E8,
                                                ERA_OPT(combat, autoSpells, name),
                                                ERA_OPT(combat, autoSpells, hint)}, // show combat animations
                                               {"auto_combat_catapult",
                                                {x, leftPartY + baseSettingHeight * 2},
                                                itemId++,
                                                &config.autoCatapult, //  0x06987EC,
                                                ERA_OPT(combat, autoCatapult, name),
                                                ERA_OPT(combat, autoCatapult, hint)}, // show floating combat text
                                               {"auto_combat_ballista",
                                                {x, leftPartY + baseSettingHeight * 3},
                                                itemId++,
                                                &config.autoBallista, //  0x06987F0,
                                                ERA_OPT(combat, autoBallista, name),
                                                ERA_OPT(combat, autoBallista, hint)},
                                               {"auto_combat_tent",
                                                {x, leftPartY + baseSettingHeight * 4},
                                                itemId++,
                                                &config.autoFirstAidTent, // 0x06987F4,
                                                ERA_OPT(combat, autoFirstAidTent, name),
                                                ERA_OPT(combat, autoFirstAidTent, hint)},
                                               {"auto_combat_quick",
                                                {x, leftPartY + baseSettingHeight * 5},
                                                itemId++,
                                                &extraConfig.quickAutoResolve.value, // 0x06987F8,
                                                ERA_OPT(combat, autoQuick, name),
                                                ERA_OPT(combat, autoQuick, hint),
                                                1}};

        for (auto &info : autoCombatInfo)
        {
            page->CreateSetting<CheckBoxSetting>(info);
        }

        const int ySwitch = leftPartY + baseSettingHeight * 6;

        constexpr size_t count = Switch10XPanel::BUTTONS_COUNT;
        // combat speed switch panel
        std::string strHints[count];
        LPCSTR hintPtrs[count]{};
        for (size_t i = 0; i < count; i++)
        {
            libc::sprintf(h3_TextBuffer, ERA_OPT(combat, animationSpeed, hints) ".%d", i);
            strHints[i] = h3_TextBuffer;
            hintPtrs[i] = strHints[i].c_str();
        }

        SettingsInfo switch10xPanelsInfo{
            "combat_animation_speed",
            {x, ySwitch},
            itemId,
            &config.animationSpeed, //  H3CurrentAnimationSpeed::ADDRESS,
            ERA_OPT(combat, animationSpeed, name),
        };
        switch10xPanelsInfo.rmcHints = &hintPtrs[0];

        itemId += count;
        page->CreateSetting<Switch10XPanel>(switch10xPanelsInfo);

        // RIGHT PAGE PART

        constexpr int rightX = DLG_RIGHT_PART_X_MARGIN;
        int rightPartY = settingsStartY;
        //     page->CreateTitle(rightX, rightPartY, ERA_OPT(combat, creatureInfo, name));

        LPCSTR infoTexts[] = {ERA_OPT(combat, allStatistics, name), ERA_OPT(combat, onlySpells, name)};
        LPCSTR infoHints[] = {ERA_OPT(combat, allStatistics, hint), ERA_OPT(combat, onlySpells, hint)};
        constexpr size_t radioButtonsCount = std::size(infoTexts);
        RadioButtonInfo radioInfo = {"combat_creature_info",
                                     {rightX, rightPartY},
                                     itemId,
                                     &config.combatViewArmy[0],
                                     ERA_OPT(combat, creatureInfo, name),
                                     radioButtonsCount,
                                     infoTexts,
                                     infoHints};
        itemId += radioButtonsCount;
        page->CreateSetting<RadioBoxSetting>(radioInfo);

        rightPartY += baseSettingHeight * 2 + ISetting::TITLE_Y_OFFSET;

        auto queuePI = globalPatcher->GetInstance("H3.ERA_BattleQueue");
        if (queuePI)
        {
            const SettingsInfo combatQueueCheckBoxInfo = {
                "combat_creatures_queue",
                {rightX, rightPartY},
                itemId++,
                &extraConfig.battleQueue.value,
                ERA_OPT(combat, battleQueue, name),
                ERA_OPT(combat, battleQueue, hint),
                isInCombat // disable in combat since it requres dlg reconstruction
            };

            auto combatQueueCheckbox = page->CreateSetting<CheckBoxSetting>(combatQueueCheckBoxInfo);
            combatQueueCheckbox->SetOnDlgClose([queuePI](ISetting *setting) {
                const BOOL enabled = setting->value.current;
                enabled ? queuePI->ApplyAll() : queuePI->UndoAll();
            });
            rightPartY += baseSettingHeight;
        }
        const auto healthBarValuePtr = HealthBarIsEnabledAddress();
        const SettingsInfo healthBarcheckBoxInfo = {"combat_health_bar_checkbox",
                                                    {rightX, rightPartY},
                                                    itemId++,
                                                    &extraConfig.showCreatureHealthBar.value,
                                                    ERA_OPT(combat, healthBar, name),
                                                    ERA_OPT(combat, healthBar, hint)};
        auto healthBarcheckBox = page->CreateSetting<CheckBoxSetting>(healthBarcheckBoxInfo);
        healthBarcheckBox->SetOnChange(
            [healthBarValuePtr](ISetting *setting) { *healthBarValuePtr = setting->value.current; });

        const SettingsInfo healthBarCaptionInfo = {"system_health_bar",
                                                   {rightX, rightPartY + baseSettingHeight},
                                                   itemId++,
                                                   nullptr,
                                                   ERA_OPT(combat, healthBar, button),
                                                   ERA_OPT(combat, healthBar, hint)};
        auto button = page->CreateSetting<CaptionButtonSetting>(healthBarCaptionInfo);
        button->SetOnChange([healthBarValuePtr, healthBarcheckBox](ISetting *setting) {
            const int checkboxStateBefore = *healthBarValuePtr;
            ShowHealthBarDlg();
            const int checkboxStateAfter = *healthBarValuePtr;
            if (checkboxStateBefore != checkboxStateAfter)
            {
                healthBarcheckBox->value.current = checkboxStateAfter;
                *(healthBarcheckBox->value.valuePtr) = checkboxStateAfter;
                CheckBoxSetting::SetCheckBoxValue(healthBarcheckBox->checkBoxItem, checkboxStateAfter);
            }
        });

        m_pages.Add(page);
    }
    InitDlgPages();
}

void SystemOptionsDlg::CreateOtherSettingsPanel(SettingsPage *page, const int x, const int y, int &itemId) noexcept
{
    constexpr int baseSettingHeight = ISetting::BASE_SETTINGS_Y_OFFSET;
    int callbackY = y; // settingsStartY + baseSettingHeight * 7;

    page->CreateTitle(x, callbackY, ERA_OPT(system, otherSettings, name));

    INT maxButtonsToShow = (DLG_HEIGHT - callbackY - DLG_CAPTION_BUTTON_TOP_MARGIN) / baseSettingHeight;

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
            maxButtonsToShow--;
            const SettingsInfo selectLang = {
                "system_select_language", {x, callbackY}, itemId++, nullptr, getDisplayedName(),
            };
            callbackY += baseSettingHeight;
            auto captionSetting = page->CreateSetting<CaptionButtonSetting>(selectLang);
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

    const SettingsInfo wogOptionCaption = {"system_wog_option",
                                           {x, callbackY},
                                           itemId++,
                                           nullptr,
                                           ERA_OPT(system, wogOptions, name),
                                           ERA_OPT(system, wogOptions, hint)};

    auto captionSetting = page->CreateSetting<CaptionButtonSetting>(wogOptionCaption);
    captionSetting->SetOnChange([](ISetting *) { CallWogOptionsDlg(); });
    maxButtonsToShow--;
    callbackY += baseSettingHeight;

    // only for game on map
    maxButtonsToShow = Clamp(0, maxButtonsToShow, registeredErmButtons.size());

    for (auto &bttn : registeredErmButtons)
    {
        const auto &info = bttn.second;
        const int ermFunctionId = info.ermFunctionId;
        LPCSTR namePtr = info.nameKey.empty() ? nullptr : info.nameKey.c_str();
        LPCSTR descriptionPtr = info.descriptionKey.empty() ? nullptr : info.descriptionKey.c_str();
        SettingsInfo bttnInfo = {bttn.first.c_str(), {x, callbackY}, itemId++, nullptr, namePtr, descriptionPtr};

        auto captionSetting = page->CreateSetting<CaptionButtonSetting>(bttnInfo);
        captionSetting->SetOnChange([ermFunctionId](ISetting *) { Era::FireErmEvent(ermFunctionId); });
        callbackY += baseSettingHeight;
        if (--maxButtonsToShow == 0)
            return;
    }

    // wog option buttons:
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
        }
        return 0;
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
    case target::PAGE_LOAD_GAME: // these 3 requries user agreement
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
        return TRUE;
    }

    return FALSE;
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
            if (value.current != value.dlgStart && !value.isBlocked)
            {
                *(value.valuePtr) = value.current;
                settingsChanged = TRUE;
                setting->TriggerDlgClose();
            }
        }
        delete page;
    }
    OriginalConfig &config = OriginalConfig::Get();
    AdditionalConfig &extraConfig = AdditionalConfig::Get();

    if (dlgCallSource != eDlgCallSource::COMBAT)
    {
        config.quickCombat = extraConfig.quickCombatType.value ? true : false;
    }
    const int newQuickCombatState = config.quickCombat;
    if (settingsChanged || quickCombatSettingState != newQuickCombatState)
    {
        config.blackoutComputer = config.enemySpeed == 5;

        // if quick combat settings were changed
        if (dlgCallSource == eDlgCallSource::ADV_MAP && networkGame && quickCombatSettingState != newQuickCombatState)
        {
            P_Main->GetPlayer()->quickCombatEnabled = newQuickCombatState;
            H3NetworkData<int> netMsg(-1, eNetwork::PLAYER_QUICK, newQuickCombatState);
            netMsg.SendData(false);
        }
        extraConfig.Save();
        CDECL_0(LONG, 0x0050C370); // j_WriteRegistry -> save settings to heroes3.ini
    }

    instance = nullptr;

    P_WindowManager->resultItemID = this->resultItemId;
}
