#include "framework.h"
#include "MainMenuGraphics.h"
#include "MainMenuPng.h"

MainMenuGraphics *MainMenuGraphics::instance = nullptr;
H3BaseDlg *MainMenuGraphics::showingMenu = nullptr;
MainMenuGraphics::MainMenuGraphics(PatcherInstance *pi) : IGamePatch(pi)
{
    mainMenu.LoadInfo("main");
    selectScenario.LoadInfo("scenario");
    CreatePatches();
}

H3DlgPcx *H3BaseDlg__CreateLogo(H3BaseDlg *dlg, const MainMenuGraphics::MenuInformation &info)
{
    auto logo = dlg->CreatePcx(info.x, info.y, info.dlgItemId, info.pcxName);
    if (logo)
        logo->DeActivate();
    return logo;
}

void MainMenuGraphics::H3SelectScenarioDlg__HandleLogo(H3BaseDlg *dlg, const int loopCounter)
{
    auto *logo = dlg->GetPcx(selectScenario.dlgItemId);

    if (lobbyMenuCounter == loopCounter)
    {
        if (logo)
        {
            logo->Show();
            lobbyMenuCounter = 0;
        }
        else
        {
            if (logo = H3BaseDlg__CreateLogo(dlg, selectScenario))
                logo->Hide();
        }
    }
    else
    {
        if (logo)
            logo->Hide();
        else
        {
            if (logo = H3BaseDlg__CreateLogo(dlg, selectScenario))
                logo->Hide();
        }
        lobbyMenuCounter = loopCounter;
    }
}

_LHF_(MainMenuGraphics::SelectScenarioDlg_NewGame)
{
    if (auto dlg = reinterpret_cast<H3BaseDlg *>(c->ecx))
        H3BaseDlg__CreateLogo(dlg, Get().selectScenario);
    Get().lobbyMenuCounter = 0;

    return EXEC_DEFAULT;
}
_LHF_(MainMenuGraphics::SelectScenarioDlg_ShowAvailableScenarios)
{
    Get().H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg *>(c->ecx), 1);
    return EXEC_DEFAULT;
}

_LHF_(MainMenuGraphics::SelectScenarioDlg_ShowRandomMap) // call RMG dlg,
{
    Get().H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg *>(c->ecx), 2);
    return EXEC_DEFAULT;
}
_LHF_(MainMenuGraphics::SelectScenarioDlg_ShowAdvancedOptions)
{
    Get().H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg *>(c->ecx), 3);
    return EXEC_DEFAULT;
}

void MainMenuGraphics::CreateLogo(H3BaseDlg *dlg)
{
    if (Get().mainMenu.enable)
        H3BaseDlg__CreateLogo(dlg, Get().mainMenu);
}

void MainMenuGraphics::DrawLogo(H3Msg *msg)
{
    const auto &info = Get().mainMenu;
    if (info.enable && info.alwaysDraw)
        if (auto *logo = msg->GetDlg()->GetPcx(info.dlgItemId))
            logo->Draw();
}

int __stdcall MainMenuGraphics::DlgMainMenu_Show(HiHook *h, H3BaseDlg *dlg, int zOrder, BOOL8 draw)
{
    // WindowMan_AddDialog links dlg into the manager AFTER vShow returns.
    // HD copies its cached frame inside vShow, then calls vRedraw on dlg.
    auto *previous = showingMenu;
    showingMenu = dlg;
    const int result = THISCALL_3(int, h->GetDefaultFunc(), dlg, zOrder, draw);
    showingMenu = previous;
    return result;
}

int __stdcall MainMenuGraphics::DlgMainMenu_Redraw(HiHook *h, H3BaseDlg *dlg, BOOL8 redrawScreen,
                                               int firstItemId, int lastItemId)
{
    // Keep partial button updates intact. Replacing the background there would
    // erase API items outside the requested ID range.
    bool replaced = false;
    auto *mouse = H3MouseManager::Get();
    const bool hideMouse = mouse && !mouse->cursorHidden;
    const bool fullRedraw = firstItemId == -65535 && lastItemId == 65535 &&
                            (showingMenu == dlg || H3WindowManager::Get()->lastDlg == dlg);
    if (fullRedraw)
    {
        if (hideMouse)
            mouse->TurnOff();
        replaced = mainmenu::background::Draw(dlg);
        static bool reported = false;
        if (!reported)
        {
            reported = true;
            auto *asset = mainmenu::background::GetAsset();
            auto *screen = H3WindowManager::Get()->GetDrawBuffer();
            const auto layout = mainmenu::background::GetLayout(dlg, mainmenu::background::Enabled());
            Era::WriteLog("Interface_MainMenuAPI", "First menu redraw",
                          H3String::Format("showing=%d replaced=%d asset=%dx%d screen=%dx%d canvas=%dx%d at %d,%d",
                                           showingMenu == dlg, replaced,
                                           asset ? asset->width : 0, asset ? asset->height : 0,
                                           screen ? screen->width : 0, screen ? screen->height : 0,
                                           layout.width, layout.height, layout.x, layout.y).String());
        }
    }
    H3BaseDlg *previousPngMenu = nullptr;
    if (fullRedraw)
    {
        mainmenu::png::Capture(dlg);
        previousPngMenu = mainmenu::png::SetDrawingMenu(dlg);
    }
    if (fullRedraw)
    {
        MenuWidgetManager::Get().CaptureBackground(dlg);
        if (auto *panel = NotificationPanel::instance)
            panel->PrepareAppearance(dlg, H3WindowManager::Get()->GetDrawBuffer());
    }
    const int result = THISCALL_4(int, h->GetDefaultFunc(), dlg, redrawScreen, firstItemId, lastItemId);
    if (fullRedraw)
        mainmenu::png::SetDrawingMenu(previousPngMenu);
    if (replaced && redrawScreen)
    {
        auto *wm = H3WindowManager::Get();
        auto *screen = wm->GetDrawBuffer();
        wm->H3Redraw(0, 0, screen->width, screen->height);
    }
    if (fullRedraw && hideMouse)
        mouse->TurnOn();
    return result;
}

void MainMenuGraphics::CreatePatches()
{
    if (!m_isInited)
    {
        m_isInited = true;

        // Only these three menu vtables: retain HD's original redraw chain and
        // render the background before it draws the dialog/API items.
        _PI->WriteHiHook(0x63FF74, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Redraw);
        _PI->WriteHiHook(0x63E6EC, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Redraw);
        _PI->WriteHiHook(0x63BC1C, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Redraw);
        _PI->WriteHiHook(0x63FF64, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Show);
        _PI->WriteHiHook(0x63E6DC, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Show);
        _PI->WriteHiHook(0x63BC0C, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Show);

        if (selectScenario.enable)
        {
            _PI->WriteLoHook(0x4F0B6A, SelectScenarioDlg_NewGame);
            _PI->WriteLoHook(0x580180, SelectScenarioDlg_ShowAdvancedOptions);
            _PI->WriteLoHook(0x5813D0, SelectScenarioDlg_ShowRandomMap);
            _PI->WriteLoHook(0x580D40, SelectScenarioDlg_ShowAvailableScenarios);
        }
    }
}

MainMenuGraphics &MainMenuGraphics::Get()
{
    if (instance == nullptr)
    {
        instance = new MainMenuGraphics(_PI);
    }
    // static MainMenuGraphics instance(_PI);
    return *instance;
}

void MainMenuGraphics::MenuInformation::LoadInfo(const char *jsonSubKey)
{
    const auto prefix = H3String::Format("era.main_menu.logos.%s.", jsonSubKey);
    bool found = false;
    pcxName = EraJS::read(H3String::Format("%sasset", prefix.String()).String(), found);
    enable = found && pcxName && pcxName[0] &&
             mainmenu::config::Enabled(H3String::Format("%senabled", prefix.String()).String(), false);
    x = mainmenu::config::Int(H3String::Format("%sx", prefix.String()).String(), 23);
    y = mainmenu::config::Int(H3String::Format("%sy", prefix.String()).String(), 23);
    dlgItemId = Clamp(1, mainmenu::config::Int(H3String::Format("%sitem_id", prefix.String()).String(), 666), 32767);
    alwaysDraw = !strcmp(jsonSubKey, "main") &&
                 mainmenu::config::Enabled("era.main_menu.rendering.redraw_always", false);
}
