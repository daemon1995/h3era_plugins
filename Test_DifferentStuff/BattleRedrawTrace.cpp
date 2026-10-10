#include "framework.h"

#include <cstdio>
#include <cstring>
#include <intrin.h>

namespace
{
constexpr DWORD COMBAT_REDRAW_BATTLEFIELD = 0x00493FC0;
constexpr DWORD WINDOW_MANAGER_REDRAW = 0x00603190;

FILE *traceFile = nullptr;
struct SkippedCall
{
    int hook;
    DWORD caller;
    H3CombatManager *manager;
};
SkippedCall skippedCalls[32] = {};
size_t skippedCallCount = 0;

FILE *GetTraceFile()
{
    if (traceFile)
        return traceFile;

    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return nullptr;

    char *lastSlash = strrchr(path, '\\');
    if (!lastSlash)
        return nullptr;

    lastSlash[1] = '\0';
    strcat_s(path, "BattleRedrawTrace.log");

    if (fopen_s(&traceFile, path, "a") != 0)
        return nullptr;

    std::fprintf(traceFile, "\r\n=== Battle redraw trace started (tick=%lu) ===\r\n", GetTickCount());
    std::fflush(traceFile);
    return traceFile;
}

void Log(const char *event, DWORD caller, H3CombatManager *manager, const char *details)
{
    FILE *file = GetTraceFile();
    if (!file)
        return;

    std::fprintf(file, "[%lu][thread=%lu] %s caller=0x%08lX manager=%p %s\r\n", GetTickCount(), GetCurrentThreadId(),
                 event, caller, manager, details);
    // Keep the trace useful if the game exits unexpectedly during a quick battle.
    std::fflush(file);
}

void LogFirstSkippedCall(int hook, const char *event, DWORD caller, H3CombatManager *manager, const char *details)
{
    for (size_t i = 0; i < skippedCallCount; ++i)
    {
        const SkippedCall &previous = skippedCalls[i];
        if (previous.hook == hook && previous.caller == caller && previous.manager == manager)
            return;
    }

    if (skippedCallCount < sizeof(skippedCalls) / sizeof(skippedCalls[0]))
        skippedCalls[skippedCallCount++] = {hook, caller, manager};
    Log(event, caller, manager, details);
}

void __stdcall RedrawBattlefield(HiHook *hook, H3CombatManager *manager, BOOL8 flip, BOOL8 setBattleRedraws,
                                 BOOL8 useBattleRedraws, INT32 waitingTime, BOOL8 redrawBackground, BOOL8 wait)
{
    // This is the return address immediately after the game's call instruction.
    const DWORD caller = reinterpret_cast<DWORD>(_ReturnAddress());
    if (manager && manager->dlg)
    {
        const BOOL8 hidden = manager->IsHiddenBattle();
        char details[160] = {};
        std::snprintf(details, sizeof(details),
                      "hidden=%u flip=%u setBorders=%u useBorders=%u delay=%d background=%u wait=%u",
                      static_cast<unsigned>(hidden), static_cast<unsigned>(flip),
                      static_cast<unsigned>(setBattleRedraws), static_cast<unsigned>(useBattleRedraws), waitingTime,
                      static_cast<unsigned>(redrawBackground), static_cast<unsigned>(wait));
        if (hidden)
        {
            // Preserve battlefield buffer updates, but suppress presenting them to the screen.
            LogFirstSkippedCall(0, "BATTLEFIELD_REDRAW_NO_FLIP(0x493FC0)", caller, manager, details);
            THISCALL_7(void, hook->GetDefaultFunc(), manager, FALSE, setBattleRedraws, useBattleRedraws, waitingTime,
                       redrawBackground, wait);
            return;
        }
        Log("BATTLEFIELD_REDRAW(0x493FC0)", caller, manager, details);
    }

    THISCALL_7(void, hook->GetDefaultFunc(), manager, flip, setBattleRedraws, useBattleRedraws, waitingTime,
               redrawBackground, wait);
}
Patch *inBattlefieldRedraw = nullptr;
Patch *inCombatRedraw = nullptr;
void __stdcall WindowManagerRedraw(HiHook *hook, H3WindowManager *windowManager, INT32 x, INT32 y, INT32 width,
                                   INT32 height)
{
    const DWORD caller = reinterpret_cast<DWORD>(_ReturnAddress());
    H3CombatManager *manager = P_CombatManager->Get();
    if (manager && manager->dlg && manager->dlg->IsTopDialog() && manager->IsHiddenBattle() && windowManager->lastDlg == manager->dlg)
    {
        char details[144] = {};
        std::snprintf(details, sizeof(details), "hidden=1 rect=(%d,%d,%d,%d)", x, y, width, height);
        LogFirstSkippedCall(1, "WINDOW_MANAGER_REDRAW_SKIPPED(0x603190)", caller, manager, details);
        return;
    }

    THISCALL_5(void, hook->GetDefaultFunc(), windowManager, x, y, width, height);
}

_ERH_(OnBeforeBattleUniversal)
{
  //  if (!P_CombatManager->Get()->IsHiddenBattle())
    {
        inBattlefieldRedraw->Apply();
        inCombatRedraw->Apply();
    }
}
_ERH_(OnAfterBattleUniversal)
{
    inBattlefieldRedraw->Undo();
    inCombatRedraw->Undo();
}
} // namespace

void InstallBattleRedrawTrace(PatcherInstance *patcher)
{
    return;
    _REH_(OnBeforeBattleUniversal);
    _REH_(OnAfterBattleUniversal);
    inBattlefieldRedraw = patcher->WriteHiHook(COMBAT_REDRAW_BATTLEFIELD, THISCALL_, RedrawBattlefield);
    inBattlefieldRedraw->Undo();
    inCombatRedraw = patcher->WriteHiHook(WINDOW_MANAGER_REDRAW, THISCALL_, WindowManagerRedraw);
    inCombatRedraw->Undo();
}
