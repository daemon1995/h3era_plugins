// Standalone x86 tests: compile this file, not the plugin project.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define PCH_H
using LPCSTR = const char *;
constexpr int FALSE = 0, EXEC_DEFAULT = 1, NO_EXEC_DEFAULT = 0, THISCALL_ = 0;
struct LoHook {};
struct Patch { bool IsApplied() { return false; } int Apply() { return 0; } void Undo() {} };
struct HiHook { void *function; void *GetDefaultFunc() { return function; } };
struct HookContext
{
    uintptr_t ebx;
    unsigned edx;
    uintptr_t return_address = 0;
    template <typename T> T Ebx() { return reinterpret_cast<T>(ebx); }
};
#define _LHF_(name) int __stdcall name(LoHook *, HookContext *c)
template <typename R, typename... A> R Invoke(void *function, A... args)
{
    return reinterpret_cast<R(__cdecl *)(A...)>(function)(args...);
}
#define THISCALL_1(R, F, ...) Invoke<R>(F, __VA_ARGS__)
#define THISCALL_2(R, F, ...) Invoke<R>(F, __VA_ARGS__)
#define THISCALL_5(R, F, ...) Invoke<R>(F, __VA_ARGS__)
struct PatcherInstance
{
    std::vector<int> addresses;
    template <typename F> void WriteHiHook(int address, int, F) { addresses.push_back(address); }
    template <typename F> void WriteLoHook(int address, F) { addresses.push_back(address); }
};
namespace EraJS
{
LPCSTR read(LPCSTR, bool &found) { found = true; return "Localized hint"; }
LPCSTR read(LPCSTR key) { assert(!std::strcmp(key, "gem_plugin.level_up_dlg.skip_hint")); return "Localized hint"; }
}
namespace libc { int strcmpi(LPCSTR a, LPCSTR b) { return _stricmp(a, b); } }
struct H3Hero { int level = 1; };
struct H3BaseDlg;
enum class eMsgCommand { NONE, MOUSE_BUTTON, KEY_DOWN, KEY_UP, KEY_HELD };
enum class eMsgSubtype { NONE, LBUTTON_CLICK, RBUTTON_DOWN, END_DIALOG, LBUTTON_DOWN };
enum class eVKey { H3VK_ESCAPE = 1, H3VK_ENTER = 28, H3VK_SPACEBAR = 57 };
struct H3Msg
{
    eMsgCommand command = eMsgCommand::NONE;
    eMsgSubtype subtype = eMsgSubtype::NONE;
    int itemId = 0;
    H3BaseDlg *parentDlg = nullptr;
    int flags = 0;
    H3BaseDlg *GetDlg() { return parentDlg; }
    bool IsKeyDown() { return command == eMsgCommand::KEY_DOWN; }
    bool IsKeyPress() { return command == eMsgCommand::KEY_UP; }
    bool CtrlPressed() { return (flags & 4) != 0; }
    eVKey GetKey() { return static_cast<eVKey>(subtype); }
    bool IsLeftClick() { return command == eMsgCommand::MOUSE_BUTTON && subtype == eMsgSubtype::LBUTTON_CLICK; }
    bool IsLeftDown() { return command == eMsgCommand::MOUSE_BUTTON && subtype == eMsgSubtype::LBUTTON_DOWN; }
    bool IsRightClick() { return command == eMsgCommand::MOUSE_BUTTON && subtype == eMsgSubtype::RBUTTON_DOWN; }
    int StopProcessing() { command = eMsgCommand::NONE; subtype = eMsgSubtype::NONE; return 2; }
};
using H3DlgButton_proc = int(__fastcall *)(H3Msg *);
struct H3DlgDefButton
{
    int x, y, width, height, id;
    LPCSTR def;
    H3DlgButton_proc callback = nullptr;
    int GetX() { return x; }
    int GetY() { return y; }
};
struct H3LoadedPcx16
{
    static int alive;
    static bool failCreation;
    int width, height;
    std::vector<unsigned> pixels;
    std::vector<std::string> operations;
    H3LoadedPcx16(int w, int h) : width(w), height(h), pixels(w * h, 0xDEADBEEF) { ++alive; }
    ~H3LoadedPcx16() { --alive; }
    static H3LoadedPcx16 *Create(int w, int h) { return failCreation ? nullptr : new H3LoadedPcx16(w, h); }
    void Destroy() { delete this; }
    void FillRectangle(int x, int y, int w, int h, int r, int g, int b)
    {
        operations.push_back("fill");
        for (int row = y; row < y + h; ++row)
            for (int col = x; col < x + w; ++col)
                pixels[row * width + col] = (r << 16) | (g << 8) | b;
    }
    void DrawFrame(int x, int y, int w, int h, int r = 232, int g = 212, int b = 120)
    {
        operations.push_back("frame");
        for (int row = y; row < y + h; ++row)
            for (int col = x; col < x + w; ++col)
                if (row == y || row == y + h - 1 || col == x || col == x + w - 1)
                    pixels[row * width + col] = (r << 16) | (g << 8) | b;
    }
};
int H3LoadedPcx16::alive = 0;
bool H3LoadedPcx16::failCreation = false;
struct H3DlgPcx16
{
    int x, y, width, height;
    H3LoadedPcx16 *pcx = nullptr;
    static bool failCreation;
    static H3DlgPcx16 *Create(int x, int y, int w, int h, int, LPCSTR)
    {
        return failCreation ? nullptr : new H3DlgPcx16{x, y, w, h};
    }
    void SetPcx(H3LoadedPcx16 *value) { pcx = value; }
    ~H3DlgPcx16() { if (pcx) pcx->Destroy(); }
};
bool H3DlgPcx16::failCreation = false;
struct H3BaseDlg
{
    int GetWidth() { return 385; }
    std::vector<H3DlgDefButton> buttons;
    std::vector<std::unique_ptr<H3DlgPcx16>> frames;
    std::vector<std::string> additions;
    void AddItem(H3DlgPcx16 *frame) { frames.emplace_back(frame); additions.push_back("frame"); }
    H3DlgDefButton *GetDefButton(int id)
    {
        for (auto &button : buttons) if (button.id == id) return &button;
        return nullptr;
    }
    H3DlgDefButton *CreateCustomButton(int x, int y, int w, int h, int id, LPCSTR def,
                                     H3DlgButton_proc callback, int, int)
    {
        buttons.push_back({x, y, w, h, id, def, callback});
        additions.push_back("callback");
        return &buttons.back();
    }
};
struct H3Messagebox
{
    static std::string text;
    static void RMB(LPCSTR value) { text = value; }
};
std::string H3Messagebox::text;
struct WindowManager { int result = 0; void SetClickedItemId(int id) { result = id; } } manager;
WindowManager *P_WindowManager = &manager;
int timeout = 0;
int &IntAt(int address)
{
    assert(address == 0x6977D4); // Accessing the hidden-mode flag is forbidden.
    return timeout;
}

#include "LevelUpAutoSkip.cpp"

std::function<void(H3Hero *)> nativeLoop;
void NativeCheck(H3Hero *hero) { auto loop = nativeLoop; loop(hero); }
H3BaseDlg *NativeCtor(H3BaseDlg *dlg, H3Hero *, int, int, int)
{
    dlg->buttons.reserve(2);
    dlg->buttons.push_back({296, 413, 64, 30, 30722, "iOkay.def"});
    timeout = 15000;
    return dlg;
}
int NativeProc(H3BaseDlg *, H3Msg *) { return 42; }
int NativeDtor(H3BaseDlg *) { return 7; }
HiHook checkHook{reinterpret_cast<void *>(NativeCheck)}, ctorHook{reinterpret_cast<void *>(NativeCtor)};
HiHook procHook{reinterpret_cast<void *>(NativeProc)}, dtorHook{reinterpret_cast<void *>(NativeDtor)};
void SetTarget(H3Hero *hero, int target)
{
    HookContext c{reinterpret_cast<uintptr_t>(hero), static_cast<unsigned>(target)};
    assert(features::Hero_LevelUpTargetCalculated(nullptr, &c) == EXEC_DEFAULT);
}
int DispatchCustomButton(H3BaseDlg &dlg, H3Msg &msg)
{
    auto *button = dlg.GetDefButton(msg.itemId);
    assert(button && button->callback);
    msg.parentDlg = &dlg;
    const int result = button->callback(&msg);
    // ProcessDialog copies the message ID over resultItemID for item callbacks.
    if (result == 2 && msg.command == eMsgCommand::MOUSE_BUTTON && msg.subtype == eMsgSubtype::END_DIALOG)
        manager.result = msg.itemId;
    return result;
}
void RunSeries(int manuallyAccepted)
{
    H3Hero hero;
    int sounds = 0, drawn = 0;
    nativeLoop = [&](H3Hero *h) {
        SetTarget(h, 7);
        for (int i = 0; i < 6; ++i)
        {
            ++h->level;
            HookContext c{reinterpret_cast<uintptr_t>(h), 0};
            if (features::Hero_LevelUpSound(nullptr, &c) == EXEC_DEFAULT) ++sounds;
            else assert(c.return_address == 0x4DACA0);
            H3BaseDlg dlg;
            const int offers = i % 3;
            features::Hero_LevelUpDialogCtor(&ctorHook, &dlg, h, 0, offers ? 3 : -1, offers == 2 ? 6 : -1);
            ++drawn;
            H3Msg msg;
            if (i < manuallyAccepted)
                assert(features::Hero_LevelUpDialogProc(&procHook, &dlg, &msg) == 42);
            else
            {
                if (i == manuallyAccepted)
                {
                    msg = {eMsgCommand::MOUSE_BUTTON, eMsgSubtype::LBUTTON_CLICK, 2020};
                    assert(DispatchCustomButton(dlg, msg) == 2 && msg.itemId == 9999);
                }
                else
                    assert(features::Hero_LevelUpDialogProc(&procHook, &dlg, &msg) == 2 && msg.itemId == 10);
                assert(manager.result == 9999 && timeout == 0);
            }
            assert(features::Hero_LevelUpDialogDtor(&dtorHook, &dlg) == 7 && !features::currentSeries->dialog);
        }
    };
    features::Hero_CheckLevelUps(&checkHook, &hero);
    assert(hero.level == 7 && drawn == 6 && sounds == manuallyAccepted + 2);
    assert(!features::currentSeries && H3LoadedPcx16::alive == 0);
}
void TestCtrlConfirmShortcuts()
{
    auto check = [](H3Msg msg, bool expected, bool eligible = true, bool matchingDialog = true) {
        H3Hero hero;
        H3BaseDlg dlg, otherDlg;
        features::LevelUpSeries series{&hero};
        series.dialog = matchingDialog ? &dlg : &otherDlg;
        series.eligible = eligible;
        gem::ScopedValue<features::LevelUpSeries *> scope(features::currentSeries, &series);
        manager.result = -1;
        timeout = 15000;
        const int flagsBefore = msg.flags;
        const auto original = msg;
        const int result = features::Hero_LevelUpDialogProc(&procHook, &dlg, &msg);
        if (expected)
        {
            assert(result == 2 && series.mode == features::SkipMode::ShowDialogs);
            assert(msg.command == eMsgCommand::MOUSE_BUTTON && msg.subtype == eMsgSubtype::END_DIALOG);
            assert(msg.itemId == 9999 && manager.result == 9999 && timeout == 0);
            assert(msg.parentDlg == &dlg && msg.flags == flagsBefore);
        }
        else
        {
            assert(result == 42 && series.mode == features::SkipMode::Manual);
            assert(msg.command == original.command && msg.subtype == original.subtype && msg.itemId == original.itemId);
            assert(manager.result == -1 && timeout == 15000);
        }
        assert(series.frameBuffer == nullptr && H3LoadedPcx16::alive == 0);
    };
    for (const auto key : {eVKey::H3VK_ENTER, eVKey::H3VK_ESCAPE, eVKey::H3VK_SPACEBAR})
        for (const auto command : {eMsgCommand::KEY_DOWN, eMsgCommand::KEY_UP})
            for (const int flags : {0, 1, 4, 5, 36})
            {
                H3Msg msg{command, static_cast<eMsgSubtype>(key), 0};
                msg.flags = flags;
                check(msg, (flags & 4) != 0);
            }
    for (const auto subtype : {eMsgSubtype::LBUTTON_DOWN, eMsgSubtype::LBUTTON_CLICK})
    {
        H3Msg msg{eMsgCommand::MOUSE_BUTTON, subtype, 30722};
        check(msg, false);
        msg.flags = 4;
        check(msg, true);
        check(msg, false, false);
        check(msg, false, true, false);
        msg.itemId = 2010;
        check(msg, false);
    }
    H3Msg otherKey{eMsgCommand::KEY_DOWN, static_cast<eMsgSubtype>(30), 0};
    otherKey.flags = 4;
    check(otherKey, false);
    H3Msg rightClick{eMsgCommand::MOUSE_BUTTON, eMsgSubtype::RBUTTON_DOWN, 30722};
    rightClick.flags = 4;
    check(rightClick, false);
    H3Msg held{eMsgCommand::KEY_HELD, static_cast<eMsgSubtype>(eVKey::H3VK_ENTER), 0};
    held.flags = 4;
    check(held, false);
    assert(!features::currentSeries);
}

int main()
{
#ifdef LEVEL_UP_KEYBOARD_TESTS_ONLY
    TestCtrlConfirmShortcuts();
    std::cout << "PASS: Ctrl+Enter/Esc/Space, key down/up, converted OK events, modifier combinations, eligibility and dialog isolation\n";
#else
    PatcherInstance pi;
    features::InstallLevelUpAutoSkip(&pi);
    assert((pi.addresses == std::vector<int>{0x4DA990, 0x4DAA3A, 0x4DAC91, 0x4F8F10, 0x4F9E10, 0x4F9DC0}));
    assert(features::AutoSkipButtonCallback(nullptr) == 0);
    for (int levels = 0; levels <= 4; ++levels)
    {
        H3Hero hero;
        nativeLoop = [&](H3Hero *h) {
            SetTarget(h, h->level + levels);
            H3BaseDlg dlg;
            features::Hero_LevelUpDialogCtor(&ctorHook, &dlg, h, 0, 3, 6);
            assert(dlg.buttons.size() == (levels > 3 ? 2u : 1u) && !dlg.GetDefButton(2021));
            if (levels > 3)
            {
                auto *button = dlg.GetDefButton(2020);
                assert(button->x == 224 && button->y == 413 && !std::strcmp(button->def, "iOKAL.def"));
                assert((dlg.additions == std::vector<std::string>{"frame", "callback"}));
                auto &frame = *dlg.frames.at(0);
                assert(frame.x == 222 && frame.y == 412 && frame.width == 68 && frame.height == 32);
                assert((frame.pcx->operations == std::vector<std::string>{"fill", "frame"}));
                for (int y = 0; y < 32; ++y)
                    for (int x = 0; x < 68; ++x)
                        assert(frame.pcx->pixels[y * 68 + x] == (x == 0 || x == 67 || y == 0 || y == 31 ? 0xE8D478u : 0u));
                H3Msg hint{eMsgCommand::MOUSE_BUTTON, eMsgSubtype::RBUTTON_DOWN, 2020};
                assert(DispatchCustomButton(dlg, hint) == 2 && hint.command == eMsgCommand::NONE);
                assert(H3Messagebox::text == "Localized hint" && features::currentSeries->mode == features::SkipMode::Manual);
                H3BaseDlg other;
                H3Msg foreign{eMsgCommand::MOUSE_BUTTON, eMsgSubtype::LBUTTON_CLICK, 2020, &other};
                assert(features::AutoSkipButtonCallback(&foreign) == 0);
            }
            H3Msg manual{eMsgCommand::MOUSE_BUTTON, eMsgSubtype::LBUTTON_CLICK, 30722};
            assert(features::Hero_LevelUpDialogProc(&procHook, &dlg, &manual) == 42);
            manual.itemId = 2021;
            assert(features::Hero_LevelUpDialogProc(&procHook, &dlg, &manual) == 42);
        };
        features::Hero_CheckLevelUps(&checkHook, &hero);
        assert(!features::currentSeries && H3LoadedPcx16::alive == 0);
    }
    RunSeries(0);
    RunSeries(0);
    RunSeries(2);
    for (int failure = 0; failure < 2; ++failure)
    {
        H3LoadedPcx16::failCreation = failure == 0;
        H3DlgPcx16::failCreation = failure == 1;
        H3Hero hero;
        nativeLoop = [&](H3Hero *h) {
            SetTarget(h, 7);
            H3BaseDlg dlg;
            features::Hero_LevelUpDialogCtor(&ctorHook, &dlg, h, 0, 3, 6);
            assert(dlg.buttons.size() == 1 && dlg.frames.empty() && !features::currentSeries->dialog);
            assert(H3LoadedPcx16::alive == 0);
        };
        features::Hero_CheckLevelUps(&checkHook, &hero);
    }
    assert(!features::currentSeries && H3LoadedPcx16::alive == 0);
    std::cout << "PASS: callback result, auto-close proc, hidden mode disabled, black 68x32 frame, hints, sound limits, ownership/failure cleanup\n";
#endif
}
