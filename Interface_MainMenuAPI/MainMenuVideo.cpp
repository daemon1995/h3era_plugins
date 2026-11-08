#include "framework.h"
#include "MainMenuPng.h"
#include <unordered_map>

namespace mainmenu
{
static int videoX = 0;
static int videoY = 0;

namespace
{
struct VideoMenu
{
    UINT firstItem;
    H3DlgPcx16 *panelBackground = nullptr;
};
std::unordered_map<H3BaseDlg *, VideoMenu> videoMenus;
HiHook *frameHooks[2] = {};
HiHook *backgroundHook = nullptr;
HiHook *decodeHooks[2] = {};
HiHook *addDialogHook = nullptr;
std::unordered_map<_ptr_, HiHook *> externalDecodeHooks;
int showingOtherDialog = 0;
int decodingVideo = 0;
bool decodeHadBackground = false;
UINT presentedFrames = 0;
H3MouseManager *decodeMouse = nullptr;
bool restoringWidgets = false;
H3LoadedPcx16 *videoFrame = nullptr;
RECT videoFrameBounds{};
const void *videoSource = nullptr;
int videoBitMode = 0;

void ReleaseVideoFrame()
{
    if (videoFrame)
        videoFrame->Destroy();
    videoFrame = nullptr;
    videoSource = nullptr;
}

void SetFrameHooksEnabled(bool enabled)
{
    for (auto *hook : frameHooks)
        if (hook)
            enabled ? hook->Apply() : hook->Undo();
    if (backgroundHook)
        enabled ? backgroundHook->Apply() : backgroundHook->Undo();
    for (auto *hook : decodeHooks)
        if (hook)
            enabled ? hook->Apply() : hook->Undo();
    if (addDialogHook)
        enabled ? addDialogHook->Apply() : addDialogHook->Undo();
    for (const auto &entry : externalDecodeHooks)
        if (entry.second)
            enabled ? entry.second->Apply() : entry.second->Undo();
}

bool GetVideoBounds(RECT &rect, const void **source = nullptr)
{
    const int *bik = *reinterpret_cast<const int **>(0x694D00);
    if (!bik)
        bik = *reinterpret_cast<const int **>(0x694D04);
    if (bik && ByteAt(0x694DAC))
    {
        if (source) *source = bik;
        rect = {IntAt(0x694D14), IntAt(0x694D18), IntAt(0x694D14) + bik[0], IntAt(0x694D18) + bik[1]};
        return bik[0] > 0 && bik[1] > 0;
    }
    const int *smk = *reinterpret_cast<const int **>(0x69FE58);
    if (!smk)
        smk = *reinterpret_cast<const int **>(0x69FE5C);
    if (!smk || !ByteAt(0x69FEBC))
        return false;
    if (source) *source = smk;
    rect = {IntAt(0x69FE60), IntAt(0x69FE64), IntAt(0x69FE60) + smk[1], IntAt(0x69FE64) + smk[2]};
    return smk[1] > 0 && smk[2] > 0;
}

void CopyVideoRegion(H3LoadedPcx16 *dst, const H3LoadedPcx16 *src, const RECT &rect)
{
    const RECT screen{0, 0, src->width, src->height};
    RECT clipped;
    if (!IntersectRect(&clipped, &rect, &screen))
        return;
    for (int y = clipped.top; y < clipped.bottom; ++y)
        libc::memcpy(dst->buffer + y * dst->scanlineSize + clipped.left * videoBitMode,
                     src->buffer + y * src->scanlineSize + clipped.left * videoBitMode,
                     (clipped.right - clipped.left) * videoBitMode);
}

void CaptureVideoFrame(const RECT &dirty)
{
    RECT bounds;
    const void *source = nullptr;
    auto *screen = H3WindowManager::Get()->GetDrawBuffer();
    if (!screen || !GetVideoBounds(bounds, &source))
        return;
    const int mode = H3BitMode::Get();
    if (!videoFrame || videoFrame->width != screen->width || videoFrame->height != screen->height ||
        videoBitMode != mode)
    {
        if (videoFrame) videoFrame->Destroy();
        videoFrame = H3LoadedPcx16::Create(screen->width, screen->height);
        videoSource = nullptr;
    }
    if (!videoFrame)
        return;
    videoBitMode = mode;
    RECT region;
    if (videoSource != source || !EqualRect(&videoFrameBounds, &bounds))
        region = bounds;
    else if (!IntersectRect(&region, &dirty, &bounds))
        return;
    CopyVideoRegion(videoFrame, screen, region);
    videoFrameBounds = bounds;
    videoSource = source;
}

void __stdcall DrawPanelBackground(HiHook *hook, H3DlgPcx16 *item)
{
    const auto menu = videoMenus.find(item->GetParent());
    if (menu == videoMenus.end() || menu->second.panelBackground != item)
    {
        THISCALL_1(void, hook->GetDefaultFunc(), item);
        return;
    }
    auto *wm = H3WindowManager::Get();
    auto *pcx = item->GetPcx();
    auto *screen = wm->GetDrawBuffer();
    RECT videoRect, cut;
    const int x = item->GetAbsoluteX(), y = item->GetAbsoluteY();
    const RECT panel{x, y, x + item->GetWidth(), y + item->GetHeight()};
    if (!pcx || !screen || !GetVideoBounds(videoRect) ||
        !IntersectRect(&cut, &panel, &videoRect))
    {
        THISCALL_1(void, hook->GetDefaultFunc(), item);
        return;
    }
    // Native PCX16 region drawing retains clipping and 16/32-bit support.
    const auto draw = [&](int left, int top, int right, int bottom) {
        if (right > left && bottom > top)
            THISCALL_12(void, 0x44DF80, pcx, left - x, top - y, right - left, bottom - top,
                        screen->buffer, left, top, screen->width, screen->height, screen->scanlineSize, FALSE);
    };
    draw(panel.left, panel.top, panel.right, cut.top);
    draw(panel.left, cut.bottom, panel.right, panel.bottom);
    draw(panel.left, cut.top, cut.left, cut.bottom);
    draw(cut.right, cut.top, panel.right, cut.bottom);
}

void __stdcall PresentVideoFrame(HiHook *hook, H3WindowManager *wm, int x, int y, int width, int height)
{
    auto *dlg = wm->lastDlg;
    const auto menu = videoMenus.find(dlg);
    auto *screen = wm->GetDrawBuffer();
    RECT dirty{x, y, x + width, y + height};
    RECT clipped;
    const RECT bounds{0, 0, screen ? screen->width : 0, screen ? screen->height : 0};
    if (showingOtherDialog || restoringWidgets || menu == videoMenus.end() || !screen || width <= 0 || height <= 0 ||
        !IntersectRect(&clipped, &dirty, &bounds))
    {
        THISCALL_5(void, hook->GetDefaultFunc(), wm, x, y, width, height);
        return;
    }

    // These calls occur after BIK/SMK decoding and before the frame reaches
    // the screen. Use the player's actual dirty rectangle, including offsets.
    restoringWidgets = true;
    ++presentedFrames;
    // A codec's dirty bounding box can contain unchanged pixels. Those are
    // clean only while the decode wrappers have removed the previous UI.
    if (decodingVideo)
        CaptureVideoFrame(clipped);
    dirty = clipped;
    bool redrawn = false;
    auto *mouse = H3MouseManager::Get();
    const bool hideMouse = mouse && !mouse->cursorHidden;
    if (hideMouse)
        mouse->TurnOff();
    auto *previousMenu = png::SetDrawingMenu(dlg);
    // New/Load headings are direct PCX/PNG draws, not dialog button items.
    // Restore them before buttons and panels, in the same presentation.
    redrawn = png::RedrawTitles(dlg, dirty);
    const auto drawItem = [&](H3DlgItem *item) {
        if (!item || !item->IsVisible())
            return;
        const int itemX = item->GetAbsoluteX(), itemY = item->GetAbsoluteY();
        const RECT itemRect{itemX, itemY, itemX + item->GetWidth(), itemY + item->GetHeight()};
        RECT visibleRect, overlap;
        if (!IntersectRect(&visibleRect, &itemRect, &bounds) ||
            !IntersectRect(&overlap, &visibleRect, &dirty))
            return;

        item->Draw();
        redrawn = true;
        // Drawing a panel's backing also erases its child controls. Expand
        // the repair area so later items restore those controls in draw order.
        UnionRect(&dirty, &dirty, &visibleRect);
    };
    auto &items = dlg->GetList();
    // Native menu buttons are linked into the dialog by the game, rather
    // than added through H3API's GetList(). Restore their PNGs over the frame.
    if (items.Count())
    {
        auto *item = items[0];
        while (item->GetNextItem()) item = item->GetNextItem();
        for (; item; item = item->GetPreviousItem())
        {
            const auto table = *reinterpret_cast<const h3::ADDRESS *>(item);
            if ((table == h3::H3DlgDefButton::VTABLE || table == h3::H3DlgCaptionButton::VTABLE ||
                 table == h3::H3DlgDef::VTABLE) &&
                std::find(items.begin() + menu->second.firstItem, items.end(), item) == items.end())
                drawItem(item);
        }
    }
    for (UINT i = menu->second.firstItem; i < items.Count(); ++i)
    {
        auto *item = items[i];
        drawItem(item);
        // Native scrollable text has a no-op Draw; its lines and scrollbar
        // are linked into the dialog separately, outside GetList().
        if (item && item->IsVisible() &&
            *reinterpret_cast<const h3::ADDRESS *>(item) == H3DlgScrollableText::VTABLE)
        {
            auto *scroll = item->Cast<H3DlgScrollableText>();
            for (auto *line : *scroll->GetItems())
                drawItem(line);
            drawItem(scroll->GetTextScrollBar());
        }
    }
    png::SetDrawingMenu(previousMenu);
    if (redrawn)
        THISCALL_5(void, hook->GetDefaultFunc(), wm, dirty.left, dirty.top,
                   dirty.right - dirty.left, dirty.bottom - dirty.top);
    else
        THISCALL_5(void, hook->GetDefaultFunc(), wm, x, y, width, height);
    if (hideMouse)
        mouse->TurnOn();
    restoringWidgets = false;
}

bool BeginVideoDecode()
{
    auto *wm = H3WindowManager::Get();
    RECT bounds;
    if (showingOtherDialog || decodingVideo || restoringWidgets || videoMenus.find(wm->lastDlg) == videoMenus.end() ||
        !GetVideoBounds(bounds))
        return false;
    decodeHadBackground = video::GetBackground(bounds) != nullptr;
    ++decodingVideo;
    auto *mouse = H3MouseManager::Get();
    if (mouse && !mouse->cursorHidden)
    {
        mouse->TurnOff();
        decodeMouse = mouse;
    }
    if (decodeHadBackground)
        video::RestoreBackground();
    return true;
}

void EndVideoDecode(UINT previousFrame)
{
    // Wait/pause paths may not submit a new frame. Put the UI back even then.
    RECT bounds;
    if (presentedFrames == previousFrame && decodeHadBackground && GetVideoBounds(bounds))
        PresentVideoFrame(frameHooks[0], H3WindowManager::Get(), bounds.left, bounds.top,
                          bounds.right - bounds.left, bounds.bottom - bounds.top);
    --decodingVideo;
    if (decodeMouse)
    {
        auto *mouse = decodeMouse;
        decodeMouse = nullptr;
        mouse->TurnOn();
    }
}

void __cdecl AnimateVideo(HiHook *hook)
{
    const bool started = BeginVideoDecode();
    const UINT previous = presentedFrames;
    CDECL_0(void, hook->GetDefaultFunc());
    if (started) EndVideoDecode(previous);
}

int __cdecl DrawVideoFrame(HiHook *hook)
{
    const bool started = BeginVideoDecode();
    const UINT previous = presentedFrames;
    const int result = CDECL_0(int, hook->GetDefaultFunc());
    if (started) EndVideoDecode(previous);
    return result;
}

void __stdcall AddDialog(HiHook *hook, H3WindowManager *wm, H3BaseDlg *dlg, int index, int draw)
{
    // vShow draws the new dialog and forces a video frame BEFORE AddDialog
    // changes lastDlg. Restoring the menu there would erase the new window.
    const bool otherDialog = videoMenus.find(dlg) == videoMenus.end();
    if (otherDialog)
        ++showingOtherDialog;
    THISCALL_4(void, hook->GetDefaultFunc(), wm, dlg, index, draw);
    if (otherDialog)
        --showingOtherDialog;
}

void TrackExternalDecoder()
{
    // The BIK plugin replaces this call with DrawModernFrame in its DLL,
    // bypassing Video_DrawCurrentFrame (0x597A40) completely. Hook the active
    // callee, not the call operand which the player rewrites on codec changes.
    if (ByteAt(0x5FF667) != 0xE8)
        return;
    const _ptr_ target = 0x5FF66C + IntAt(0x5FF668);
    if (target == 0x597A40 || externalDecodeHooks.count(target))
        return;
    auto *hook = _PI->CreateHiHook(target, SPLICE_, EXTENDED_, CDECL_,
                                  reinterpret_cast<void *>(DrawVideoFrame));
    externalDecodeHooks.emplace(target, hook);
    if (hook && !videoMenus.empty())
        hook->Apply();
}
} // namespace

namespace video
{
void Track(H3BaseDlg *dlg)
{
    if (!dlg)
        return;
    const bool firstMenu = videoMenus.empty();
    videoMenus.emplace(dlg, VideoMenu{dlg->GetList().Count()});
    TrackExternalDecoder();
    RECT bounds;
    const void *source = nullptr;
    // MainLoop/HD may already have painted the static backdrop here. Only
    // decoder presentations can seed a clean frame; retain it between pages.
    if (!GetVideoBounds(bounds, &source) || videoSource != source || !GetBackground(bounds))
        ReleaseVideoFrame();
    if (firstMenu)
        SetFrameHooksEnabled(true);
}

void Release(H3BaseDlg *dlg)
{
    videoMenus.erase(dlg);
    if (videoMenus.empty())
    {
        SetFrameHooksEnabled(false);
        RECT bounds;
        if (!GetVideoBounds(bounds))
            ReleaseVideoFrame();
    }
}

H3LoadedPcx16 *GetBackground(RECT &bounds)
{
    const void *source = nullptr;
    auto *screen = H3WindowManager::Get()->GetDrawBuffer();
    if (!videoFrame || !GetVideoBounds(bounds, &source) ||
        !EqualRect(&bounds, &videoFrameBounds) || videoBitMode != H3BitMode::Get() ||
        !screen || videoFrame->width != screen->width || videoFrame->height != screen->height)
        return nullptr;
    return videoFrame;
}

void RestoreBackground()
{
    RECT bounds;
    if (auto *frame = GetBackground(bounds))
        CopyVideoRegion(H3WindowManager::Get()->GetDrawBuffer(), frame, bounds);
}

void SetPanelBackground(H3BaseDlg *dlg, H3DlgPcx16 *item)
{
    const auto menu = videoMenus.find(dlg);
    if (menu != videoMenus.end())
        menu->second.panelBackground = item;
}
} // namespace video

static _LHF_(CaptureMenuVideoFrame)
{
    // MainLoop has drawn the current video frame, but has not yet painted
    // Gamselbk or created dialog items. Seed the cache before UI can enter it.
    RECT bounds;
    if (GetVideoBounds(bounds))
    {
        auto *mouse = H3MouseManager::Get();
        const bool hideMouse = mouse && !mouse->cursorHidden;
        if (hideMouse)
            mouse->TurnOff();
        CaptureVideoFrame(bounds);
        if (hideMouse)
            mouse->TurnOn();
    }
    return EXEC_DEFAULT;
}

static _LHF_(MainMenu_OpenVideo)
{
    if (c->ecx == 33)
    {
        c->edx = videoX;
        IntAt(c->ebp + 0x8) = videoY;
    }
    return EXEC_DEFAULT;
}

void InstallVideoPositioning()
{
    _PI->WriteLoHook(0x4EEF3F, CaptureMenuVideoFrame);
    // Both players submit their updated region through these native calls.
    // The BIK replacement keeps this common presentation path intact.
    frameHooks[0] = _PI->CreateHiHook(0x597DC1, CALL_, EXTENDED_, THISCALL_,
                                    reinterpret_cast<void *>(PresentVideoFrame)); // SMK
    frameHooks[1] = _PI->CreateHiHook(0x597E95, CALL_, EXTENDED_, THISCALL_,
                                    reinterpret_cast<void *>(PresentVideoFrame)); // BIK
    backgroundHook = _PI->CreateHiHook(H3DlgPcx16::VTABLE + 0x10, FUNCPTR_, EXTENDED_, THISCALL_,
                                      reinterpret_cast<void *>(DrawPanelBackground));
    decodeHooks[0] = _PI->CreateHiHook(0x5979E0, SPLICE_, EXTENDED_, CDECL_,
                                     reinterpret_cast<void *>(AnimateVideo));
    decodeHooks[1] = _PI->CreateHiHook(0x597A40, SPLICE_, EXTENDED_, CDECL_,
                                     reinterpret_cast<void *>(DrawVideoFrame));
    addDialogHook = _PI->CreateHiHook(0x602970, SPLICE_, EXTENDED_, THISCALL_,
                                     reinterpret_cast<void *>(AddDialog));
    if (!config::Enabled("era.main_menu.video.position.enabled", true))
        return;
    if (H3GameWidth::Get() < 808 && H3GameHeight::Get() < 608)
        return;
    // Preserve the compatibility check from Graphics_MainMenuVideoPositioning:
    // the adjustment only applies when the video-menu patch is present.
    if (WordAt(0x597876) == 0x507C)
        return;

    videoX = config::Int("era.main_menu.video.position.x", 0);
    videoY = config::Int("era.main_menu.video.position.y", 0);
    _PI->WriteLoHook(0x597876, MainMenu_OpenVideo);
}
} // namespace mainmenu
