#include "framework.h"

#include "MainMenuGraphics.h"
#include "MainMenuPng.h"

MainMenuGraphics *MainMenuGraphics::instance = nullptr;
H3BaseDlg *MainMenuGraphics::showingMenu = nullptr;

// Keep the actual HD-adjusted 800x600 menu origin, independently of the
// fitted picture/canvas origin. Credits retain the native 460..800 region.
static int menuOriginX = 0, menuOriginY = 0;
static int composingMenu = 0;

static void __stdcall PresentMenu(HiHook *hook, H3WindowManager *wm, int x, int y, int width, int height)
{
    if (!composingMenu)
        THISCALL_5(void, hook->GetDefaultFunc(), wm, x, y, width, height);
}

static H3LoadedPcx16 *creditsBackground = nullptr;

static void __cdecl ShowCredits(HiHook *hook)
{
    auto *previous = creditsBackground;
    // Do this at the actual Credits entry, after MainMenu.pcx replacements
    // (including credits_unlink) and the menu's HD hide/snapshot restore.
    creditsBackground = mainmenu::background::GetBuffer();
    auto *mouse = H3MouseManager::Get();
    const bool hideMouse = mouse && !mouse->cursorHidden;
    if (creditsBackground)
    {
        if (hideMouse) mouse->TurnOff();
        auto *screen = H3WindowManager::Get()->GetDrawBuffer();
        // Match the original Mainmenu.pcx strip, including the text margins.
        // Copy from the fitted canvas at identical absolute coordinates;
        // leave the rest of the menu/video and its frame untouched.
        const int left = std::max(0, menuOriginX + 460), top = std::max(0, menuOriginY);
        const int right = std::min(screen->width, menuOriginX + 800);
        const int bottom = std::min(screen->height, menuOriginY + 600);
        const int bpp = H3BitMode::Get();
        if (right > left && bottom > top)
        {
            for (int y = top; y < bottom; ++y)
                libc::memcpy(screen->buffer + y * screen->scanlineSize + left * bpp,
                             creditsBackground->buffer + y * creditsBackground->scanlineSize + left * bpp,
                             (right - left) * bpp);
            H3WindowManager::Get()->H3Redraw(left, top, right - left, bottom - top);
        }
        if (hideMouse) mouse->TurnOn();
    }
    CDECL_0(void, hook->GetDefaultFunc());
    creditsBackground = previous;
}

static void __stdcall Credits_SaveBackground(HiHook *hook, H3LoadedPcx16 *source, int sx, int sy,
                                            int width, int height, unsigned char *dest, int dx, int dy,
                                            int destWidth, int destHeight, int pitch, BOOL8 transparent)
{
    if (!creditsBackground)
    {
        THISCALL_12(void, hook->GetDefaultFunc(), source, sx, sy, width, height, dest, dx, dy,
                    destWidth, destHeight, pitch, transparent);
        return;
    }
    // The native 328x580 scrolling backing must use the same fitted canvas
    // as the initial screen. Bypass HD's source-coordinate transformation:
    // the canvas already has absolute screen coordinates and its own stride.
    const int left = menuOriginX + 460, top = menuOriginY + 10;
    const int copyWidth = std::min(328, std::min(destWidth, creditsBackground->width - left));
    const int copyHeight = std::min(580, std::min(destHeight, creditsBackground->height - top));
    const int bpp = H3BitMode::Get();
    if (left >= 0 && top >= 0 && copyWidth > 0 && copyHeight > 0)
        for (int y = 0; y < copyHeight; ++y)
            libc::memcpy(dest + y * pitch,
                         creditsBackground->buffer + (top + y) * creditsBackground->scanlineSize + left * bpp,
                         copyWidth * bpp);
}

static int __stdcall HighScores_Redraw(HiHook *hook, H3BaseDlg *dlg, BOOL8 redrawScreen,
                                      int firstItemId, int lastItemId)
{
    auto *asset = mainmenu::background::GetAsset();
    auto *screen = H3WindowManager::Get()->GetDrawBuffer();
    if (!mainmenu::background::Enabled() || !asset || !screen ||
        (asset->width == 800 && asset->height == 600))
        return THISCALL_4(int, hook->GetDefaultFunc(), dlg, redrawScreen, firstItemId, lastItemId);
    auto *mouse = H3MouseManager::Get();
    const bool hideMouse = mouse && !mouse->cursorHidden;
    if (hideMouse) mouse->TurnOff();
    const int x = dlg->GetX(), y = dlg->GetY();
    const int right = x + dlg->GetWidth(), bottom = y + dlg->GetHeight();
    screen->BackgroundRegion(0, 0, screen->width, y, "DlgGreyBk.pcx");
    screen->BackgroundRegion(0, bottom, screen->width, screen->height - bottom, "DlgGreyBk.pcx");
    screen->BackgroundRegion(0, y, x, dlg->GetHeight(), "DlgGreyBk.pcx");
    screen->BackgroundRegion(right, y, screen->width - right, dlg->GetHeight(), "DlgGreyBk.pcx");
    const int leftEdge = std::max(0, x - 4), topEdge = std::max(0, y - 4);
    screen->SimpleFrameRegion(leftEdge, topEdge, std::min(screen->width, right + 4) - leftEdge,
                              std::min(screen->height, bottom + 4) - topEdge);
    const int result = THISCALL_4(int, hook->GetDefaultFunc(), dlg, FALSE, firstItemId, lastItemId);
    if (mainmenu::config::Enabled("era.main_menu.background.frame.enabled", false))
        screen->SimpleFrameRegion(0, 0, screen->width, screen->height);
    if (redrawScreen)
        H3WindowManager::Get()->H3Redraw(0, 0, screen->width, screen->height);
    if (hideMouse) mouse->TurnOn();
    return result;
}
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



int __stdcall MainMenuGraphics::DlgMainMenu_Show(HiHook *h, H3BaseDlg *dlg, int zOrder, BOOL8 draw)
{
    // WindowMan_AddDialog links dlg into the manager AFTER vShow returns.
    // HD copies its cached frame inside vShow, then calls vRedraw on dlg.
    menuOriginX = dlg->GetX();
    menuOriginY = dlg->GetY();
    auto *previous = showingMenu;
    showingMenu = dlg;
    const int result = THISCALL_3(int, h->GetDefaultFunc(), dlg, zOrder, draw);
    showingMenu = previous;
    return result;
}

int __stdcall MainMenuGraphics::DlgMainMenu_Hide(HiHook *h, H3BaseDlg *dlg, BOOL8 redrawScreen)
{
    RECT bounds;
    const bool haveVideo = mainmenu::video::GetBackground(bounds) != nullptr;
    // The native/HD hide chain restores a pre-menu snapshot. Keep that
    // intermediate image off screen; the next page presents its completed UI.
    const int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, haveVideo ? FALSE : redrawScreen);
    if (haveVideo)
        mainmenu::video::RestoreBackground();
    return result;
}

int __stdcall MainMenuGraphics::DlgMainMenu_Redraw(HiHook *h, H3BaseDlg *dlg, BOOL8 redrawScreen, int firstItemId,
                                                   int lastItemId)
{
    // Keep partial button updates intact. Replacing the background there would
    // erase API items outside the requested ID range.
    bool replaced = false;
    auto *mouse = H3MouseManager::Get();
    const bool hideMouse = mouse && !mouse->cursorHidden;
    const bool fullRedraw =
        firstItemId == -65535 && lastItemId == 65535 && (showingMenu == dlg || H3WindowManager::Get()->lastDlg == dlg);
    if (fullRedraw)
    {
        ++composingMenu;
        if (hideMouse)
            mouse->TurnOff();
        replaced = mainmenu::background::Draw(dlg);
        mainmenu::video::RestoreBackground();
        static bool reported = false;
        if (!reported)
        {
            reported = true;
            auto *asset = mainmenu::background::GetAsset();
            auto *screen = H3WindowManager::Get()->GetDrawBuffer();
            const auto layout = mainmenu::background::GetLayout(dlg, mainmenu::background::Enabled());
#ifdef DEBUG

            Era::WriteLog("Interface_MainMenuAPI", "First menu redraw",
                          H3String::Format("showing=%d replaced=%d asset=%dx%d screen=%dx%d canvas=%dx%d at %d,%d",
                                           showingMenu == dlg, replaced, asset ? asset->width : 0,
                                           asset ? asset->height : 0, screen ? screen->width : 0,
                                           screen ? screen->height : 0, layout.width, layout.height, layout.x, layout.y)
                              .String());
#endif
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
    const int result = THISCALL_4(int, h->GetDefaultFunc(), dlg, replaced ? FALSE : redrawScreen,
                                 firstItemId, lastItemId);
    if (fullRedraw)
    {
        mainmenu::png::SetDrawingMenu(previousPngMenu);
        --composingMenu;
    }
    if (fullRedraw && redrawScreen)
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

        _PI->WriteHiHook(0x4EF7F4, CALL_, EXTENDED_, CDECL_, ShowCredits);
        _PI->WriteHiHook(0x4EE536, CALL_, EXTENDED_, THISCALL_, Credits_SaveBackground);
        _PI->WriteHiHook(0x603190, SPLICE_, EXTENDED_, THISCALL_, PresentMenu);
        _PI->WriteHiHook(0x63EBAC, FUNCPTR_, EXTENDED_, THISCALL_, HighScores_Redraw);

        // Only these three menu vtables: retain HD's original redraw chain and
        // render the background before it draws the dialog/API items.
        _PI->WriteHiHook(0x63FF74, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Redraw);
        _PI->WriteHiHook(0x63E6EC, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Redraw);
        _PI->WriteHiHook(0x63BC1C, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Redraw);
        _PI->WriteHiHook(0x63FF64, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Show);
        _PI->WriteHiHook(0x63E6DC, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Show);
        _PI->WriteHiHook(0x63BC0C, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Show);
        _PI->WriteHiHook(0x63FF68, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Hide);
        _PI->WriteHiHook(0x63E6E0, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Hide);
        _PI->WriteHiHook(0x63BC10, FUNCPTR_, EXTENDED_, THISCALL_, DlgMainMenu_Hide);

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
}
