#include "../../include/Plugin.h"
#include "../../include/PluginRng.h"
#include <cstring>

// Battle audio: preserve music fades outside combat by default.
// Set GBFE_PRESERVE_NON_COMBAT_MUSIC_FADES to 0 for immediate transitions everywhere.
namespace gbf
{
namespace BattleAudio
{
// Fanfare stays on the same seeded visual stream as before the extraction.
// Music has its own stream, deliberately not reset by a replay's battle seed.
static thread_local PluginRNG musicRng;
static int __stdcall FanfareRandom(HiHook *, int low, int high)
{
    return CombatVisualRng().range(low, high);
}
static int __stdcall MusicRandom(HiHook *, int low, int high)
{
    return musicRng.range(low, high);
}
// © JackSlater
// Фикс бага SoD - Сказочные драконы колдовали без звука
int __stdcall js_BattleStack_InitAssets_BeforeInitShootingSound(LoHook *, HookContext *c)
{
    if (const auto *stack = reinterpret_cast<h3::H3CombatCreature *>(c->ebx))
    {
        if (stack->type == 133)
        {
            c->return_address = 0x043D910;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}

#ifndef GBFE_PRESERVE_NON_COMBAT_MUSIC_FADES
#define GBFE_PRESERVE_NON_COMBAT_MUSIC_FADES 1
#endif
static constexpr bool PRESERVE_NON_COMBAT_MUSIC_FADES = GBFE_PRESERVE_NON_COMBAT_MUSIC_FADES != 0;
static HANDLE transitionWakeEvent = nullptr;
static volatile LONG fastTransitions = 0;
static volatile LONG musicGeneration = 0;
static LONG resultGeneration = 0; // Protected by SoundMgr + C0.
static LONG restoreGeneration = 0;
static char resultTrack[260] = {};
static char previousTrack[260] = {};
static char combatTrack[260] = {};
static int previousTrackMode = 0;
static bool battleActive = false;
static bool restorePreviousTrack = false;
static thread_local bool requestingResult = false;
// Retain one WAV reference until the intro ends or is cancelled. Miles may reuse its sample
// handle earlier; RetireReusedSample then retires only the handle, not the resource.
static h3::H3WavFile *preBattleWav = nullptr; // Protected by SoundMgr + 90.
static int preBattleSample = 0;
static DWORD preBattleWavStartedAt = 0;
struct CombatMusicRequest
{
    HiHook *hook;
    h3::H3SoundManager *snd;
    LONG generation;
    int positionMode;
    char track[260];
};
static CombatMusicRequest combatMusic = {};
static UINT_PTR combatMusicTimer = 0; // Main-thread timer; never waits for WAV.

static LPCRITICAL_SECTION Section(h3::H3SoundManager *snd, int offset)
{
    return reinterpret_cast<LPCRITICAL_SECTION>(reinterpret_cast<char *>(snd) + offset);
}
static char *CurrentTrack()
{
    return reinterpret_cast<char *>(0x006A32F0);
}
static char *PendingTrack()
{
    return reinterpret_cast<char *>(0x006A33F4);
}
static bool FastTransitions()
{
    return InterlockedCompareExchange(&fastTransitions, 0, 0) != 0;
}
static void SetCombatTransitions(bool combat)
{
    const bool fast = combat || !PRESERVE_NON_COMBAT_MUSIC_FADES;
    InterlockedExchange(&fastTransitions, fast ? 1 : 0);
    if (fast)
        SetEvent(transitionWakeEvent);
    else
        ResetEvent(transitionWakeEvent);
}
static void CopyTrack(char *dest, const char *source)
{
    strncpy(dest, source, 259);
    dest[259] = '\0';
}

static void CancelCombatMusic()
{
    if (combatMusicTimer)
        KillTimer(nullptr, combatMusicTimer);
    combatMusicTimer = 0;
    combatMusic.hook = nullptr;
    combatMusic.snd = nullptr;
    combatMusic.track[0] = '\0';
}
static void StopAdventureSounds()
{
    h3::H3SoundManager *snd = h3::H3SoundManager::Get();
    h3::H3AdventureManager *adv = h3::H3AdventureManager::Get();
    if (!snd || !adv || ByteAt(0x00699658))
        return;
    // Native stop-and-forget for the four adventure loops. Invalid X prevents
    // replacement loops. Preserve the caller's clickSoundVar state.
    int &enabled = IntAt(reinterpret_cast<int>(snd) + 0x84);
    const int previous = enabled;
    enabled = 1;
    THISCALL_3(void, 0x00418330, adv, 1023, 1);
    enabled = previous;
}
static int __stdcall BlockAdventureSound(LoHook *, HookContext *c)
{
    if (!battleActive)
        return EXEC_DEFAULT;
    // Prologue/register saves have run; use the native ret 14h epilogue.
    c->return_address = 0x00418CE2;
    return NO_EXEC_DEFAULT;
}
// HD replaces LoadPlayWAVFile with an asynchronous helper returning no WAV/sample
// pair. Bypass that replacement only for the battle intro, keeping ownership on
// the main thread. Other HD sound hooks and all other WAV playback stay intact.
static int __stdcall LoadBattleWav(LoHook *, HookContext *c)
{
    h3::H3WavFile *wav = nullptr;
    int sample = 0;
    if (c->ecx && !ByteAt(0x00699658))
    {
        wav = THISCALL_1(h3::H3WavFile *, 0x0055C930, c->ecx);
        if (wav)
        {
            h3::DwordAt(reinterpret_cast<DWORD>(wav) + 0x28) = 2; // Native LoadPlayWAVFile sample pool selection.
            sample = THISCALL_2(int, 0x0059A510, h3::H3SoundManager::Get(), wav);
        }
    }
    c->eax = reinterpret_cast<int>(wav);
    c->edx = sample;
    c->return_address = 0x004626EF;
    return NO_EXEC_DEFAULT;
}
static void CleanupBattleWav(h3::H3WavFile *wav, int sample)
{
    if (wav && !ByteAt(0x00699658))
    {
        THISCALL_2(void, 0x0059A180, h3::H3SoundManager::Get(), sample);
        wav->Dereference();
    }
}
static void ReleasePreBattleWav()
{
    h3::H3SoundManager *snd = h3::H3SoundManager::Get();
    if (!snd || ByteAt(0x00699658))
    {
        // Native cleanup also skips VDeref after shutdown; discard stale state.
        preBattleWav = nullptr;
        preBattleSample = 0;
        return;
    }
    EnterCriticalSection(Section(snd, 0x90));
    h3::H3WavFile *wav = preBattleWav;
    const int sample = preBattleSample;
    preBattleWav = nullptr;
    preBattleSample = 0;
    preBattleWavStartedAt = 0;
    if (wav)
        CleanupBattleWav(wav, sample);
    LeaveCriticalSection(Section(snd, 0x90));
}
static int __stdcall RetireReusedSample(LoHook *, HookContext *c)
{
    // EDI is the selected HSAMPLE, inside SoundMgr + 90. Leave the original
    // Miles call and its stack/register convention entirely untouched.
    if (c->edi == preBattleSample)
        preBattleSample = 0;
    return EXEC_DEFAULT;
}

static bool FanfareStillPlaying(h3::H3SoundManager *snd)
{
    EnterCriticalSection(Section(snd, 0x90));
    const bool playing = preBattleWav && preBattleSample && IntAt(0x006987B4) && !IntAt(0x006992E0) &&
                         IntAt(reinterpret_cast<int>(snd) + 0x3C) && GetTickCount() - preBattleWavStartedAt < 10000 &&
                         THISCALL_3(int, 0x0059A330, snd, preBattleSample, 4);
    LeaveCriticalSection(Section(snd, 0x90));
    return playing;
}
static void PumpCombatMusic()
{
    if (!combatMusic.hook)
        return;
    if (!battleActive || ByteAt(0x00699658) || h3::H3SoundManager::Get() != combatMusic.snd ||
        combatMusic.generation != InterlockedCompareExchange(&musicGeneration, 0, 0))
    {
        CancelCombatMusic();
        ReleasePreBattleWav();
        return;
    }
    // Focus loss pauses samples (status 8); that is not the end of the intro.
    // Native focus handling resumes samples before clearing this flag.
    if (ByteAt(0x00699659))
        return;
    if (FanfareStillPlaying(combatMusic.snd))
        return;
    // Muting effects counts as stopping the fanfare, even if Miles is still
    // playing it at zero volume. Release it before requesting any music.
    ReleasePreBattleWav();
    if (!IntAt(0x006987B0))
        return; // Keep the selected combat track for a later music-volume change.
    const CombatMusicRequest request = combatMusic;
    CancelCombatMusic(); // Clear first: ERA redirections may reenter the UI.
    THISCALL_4(void, 0x0059AFB0, request.snd, request.track, request.positionMode,
               0); // Do not stop live combat SFX.
}
static VOID CALLBACK CombatMusicTimer(HWND, UINT, UINT_PTR timer, DWORD)
{
    if (combatMusicTimer && timer == combatMusicTimer)
        PumpCombatMusic();
}
static void __stdcall ContinueMusic(HiHook *h, h3::H3SoundManager *snd)
{
    if (combatMusic.hook && snd == combatMusic.snd && IntAt(0x006987B0))
    {
        // Native ContinueMP3 could resume adventure music/start another combat
        // track before the fanfare is finished. Keep the selected track instead.
        PumpCombatMusic();
        return;
    }
    THISCALL_1(void, h->GetDefaultFunc(), snd);
}

static void __stdcall ResumeMusic(HiHook *h, h3::H3SoundManager *snd)
{
    // WM_ACTIVATEAPP uses this path rather than ContinueMusic. Keep the queued
    // combat track and let native sample resume finish before the timer polls.
    if (combatMusic.hook && snd == combatMusic.snd)
        return;
    THISCALL_1(void, h->GetDefaultFunc(), snd);
}
// Both fade loops release SoundMgr + 90 before waiting, but retain + A8.
// Waking the wait lets the next loop hook finish the transition immediately.
static void __stdcall WaitForTransition(DWORD milliseconds)
{
    WaitForSingleObject(transitionWakeEvent, milliseconds);
}
static void __stdcall FadeOutWait(HiHook *, DWORD milliseconds)
{
    WaitForTransition(milliseconds);
}
static int __stdcall FadeOutStep(LoHook *, HookContext *c)
{
    if (!FastTransitions())
        return EXEC_DEFAULT;
    c->return_address = 0x0059B559; // Native pause, MP3Playing reset, unlocks.
    return NO_EXEC_DEFAULT;
}
static int __stdcall FadeInBegin(LoHook *, HookContext *c)
{
    if (FastTransitions())
        c->return_address = 0x0059AEB3; // Native NoSound check and final volume.
    else
    {
        c->edi = reinterpret_cast<int>(WaitForTransition);
        c->return_address = 0x0059AE48; // Replace MOV EDI,[Sleep], keep the loop.
    }
    return NO_EXEC_DEFAULT;
}
static int __stdcall FadeInStep(LoHook *, HookContext *c)
{
    if (!FastTransitions())
        return EXEC_DEFAULT;
    c->return_address = 0x0059AEB3;
    return NO_EXEC_DEFAULT;
}

struct PauseRequest
{
    h3::H3SoundManager *snd;
    LONG generation;
};
static void PauseIfCurrent(const PauseRequest &request)
{
    if (ByteAt(0x00699658)) // NoSound: the sound subsystem is shutting down.
        return;
    EnterCriticalSection(Section(request.snd, 0xA8));
    if (request.generation == InterlockedCompareExchange(&musicGeneration, 0, 0))
        THISCALL_1(void, 0x0059B380, request.snd);
    LeaveCriticalSection(Section(request.snd, 0xA8));
}
static void __cdecl PauseWorker(void *argument)
{
    PauseRequest request = *static_cast<PauseRequest *>(argument);
    HeapFree(GetProcessHeap(), 0, argument);
    PauseIfCurrent(request);
    // Returning lets the game's _beginthread wrapper perform CRT cleanup.
}

// All eight native MP3 _beginthread call sites go through this hook.
// Keep play requests asynchronous; tag pause requests so an old pause cannot
// stop a newer track. Inside combat, native pause has no fade and can run now.
static DWORD BeginMusicThread(DWORD procedure, unsigned int stackSize, void *argument)
{
    h3::H3SoundManager *snd = h3::H3SoundManager::Get();
    if (procedure == 0x0059AB40)
    {
        // Explicit requests supersede the intro. End its WAV before spawning
        // a worker; waiting for the next timer tick would allow brief overlap.
        if (combatMusic.hook)
        {
            CancelCombatMusic();
            ReleasePreBattleWav();
        }
        EnterCriticalSection(Section(snd, 0xC0));
        const LONG generation = InterlockedIncrement(&musicGeneration);
        if (requestingResult || (resultGeneration && !strcmp(resultTrack, PendingTrack())))
        {
            resultGeneration = generation;
            CopyTrack(resultTrack, PendingTrack()); // Includes ERA MP redirections.
        }
        else
            resultGeneration = 0;
        LeaveCriticalSection(Section(snd, 0xC0));
        return CDECL_3(DWORD, 0x0061A56C, procedure, stackSize, argument);
    }
    if (procedure != 0x0059AB30)
        return CDECL_3(DWORD, 0x0061A56C, procedure, stackSize, argument);

    PauseRequest request = {snd, InterlockedCompareExchange(&musicGeneration, 0, 0)};
    if (FastTransitions())
    {
        PauseIfCurrent(request);
        return 0; // Native callers discard _beginthread's return value.
    }
    PauseRequest *queued = static_cast<PauseRequest *>(HeapAlloc(GetProcessHeap(), 0, sizeof(PauseRequest)));
    if (!queued)
    {
        PauseIfCurrent(request);
        return 0;
    }
    *queued = request;
    const DWORD thread = CDECL_3(DWORD, 0x0061A56C, PauseWorker, stackSize, queued);
    if (thread == static_cast<DWORD>(-1))
    {
        HeapFree(GetProcessHeap(), 0, queued);
        PauseIfCurrent(request);
    }
    return thread;
}

static int __stdcall MusicThreadCall(LoHook *h, HookContext *c)
{
    // Keep the native cdecl argument cleanup and all saved registers intact.
    // In particular, avoid the EXTENDED/CDECL bridge in the focus-loss handler:
    // its synchronous-pause branch corrupts the following import operand.
    const DWORD *args = reinterpret_cast<const DWORD *>(c->esp);
    c->eax = BeginMusicThread(args[0], args[1], reinterpret_cast<void *>(args[2]));
    c->return_address = h->GetAddress() + 5;
    return NO_EXEC_DEFAULT;
}
static void RemoveHdCombatMusicHook()
{
    // HD may install its hook after this plugin. Remove only its combat call
    // wrapper, which can swallow the request; retain every other plugin hook.
    for (Patch *patch = gbf::patcher->GetLastPatchAt(0x00462C65); patch;)
    {
        Patch *previous = patch->GetAppliedBefore();
        if (patch->GetType() == HIHOOK_ && patch->IsApplied() && !strcmp(patch->GetOwner(), "HD.WoG"))
            patch->Undo();
        patch = previous;
    }
}
static void CancelResult()
{
    h3::H3SoundManager *snd = h3::H3SoundManager::Get();
    if (!snd || ByteAt(0x00699658))
        return;
    // Same lock order as native pause: change -> Miles -> names.
    EnterCriticalSection(Section(snd, 0xA8));
    EnterCriticalSection(Section(snd, 0x90));
    EnterCriticalSection(Section(snd, 0xC0));
    if (resultGeneration && resultGeneration == InterlockedCompareExchange(&musicGeneration, 0, 0))
    {
        // Native pause retains the resource/stream and records its position.
        // A running play worker has finished before we acquired + A8; queued
        // workers will see an empty pending name and cannot resurrect results.
        THISCALL_1(void, 0x0059B380, snd);
        PendingTrack()[0] = '\0';
        CurrentTrack()[0] = '\0';
        resultTrack[0] = '\0';
        restoreGeneration = InterlockedIncrement(&musicGeneration);
        restorePreviousTrack = true;
    }
    resultGeneration = 0;
    LeaveCriticalSection(Section(snd, 0xC0));
    LeaveCriticalSection(Section(snd, 0x90));
    LeaveCriticalSection(Section(snd, 0xA8));
}

static int __stdcall StartBattle(HiHook *h, h3::H3CombatManager *mgr, int zOrder)
{
    SetCombatTransitions(true); // Wake an already running adventure-map fade.
    RemoveHdCombatMusicHook();
    CancelCombatMusic();
    ReleasePreBattleWav();
    if (!battleActive)
    {
        h3::H3SoundManager *snd = h3::H3SoundManager::Get();
        EnterCriticalSection(Section(snd, 0xC0));
        const bool pending = PendingTrack()[0] != '\0';
        CopyTrack(previousTrack, pending ? PendingTrack() : CurrentTrack());
        previousTrackMode = IntAt(pending ? 0x0069FEFC : 0x0069FEF0);
        LeaveCriticalSection(Section(snd, 0xC0));
        battleActive = true;
        restorePreviousTrack = false;
        combatTrack[0] = '\0';
    }
    StopAdventureSounds(); // Clear loop bookkeeping before the native WAV starts.
    CancelResult();        // Also covers replay implementations that skip dialog return.
    return THISCALL_2(int, h->GetDefaultFunc(), mgr, zOrder);
}
static int __stdcall FinishBattle(HiHook *h, h3::H3CombatManager *mgr)
{
    CancelCombatMusic();
    ReleasePreBattleWav();
    CancelResult();
    const int result = THISCALL_1(int, h->GetDefaultFunc(), mgr);
    battleActive = false;
    SetCombatTransitions(false);
    h3::H3SoundManager *snd = h3::H3SoundManager::Get();
    bool stoppedCombatTrack = false;
    bool restore = false;
    if (snd && !ByteAt(0x00699658))
    {
        // A focus change can leave the combat MP3 selected when the native
        // battle teardown skips its normal adventure-music transition. Stop
        // only that track; if another track is already pending, let it win.
        EnterCriticalSection(Section(snd, 0xA8));
        EnterCriticalSection(Section(snd, 0x90));
        EnterCriticalSection(Section(snd, 0xC0));
        const bool pendingCombat = combatTrack[0] && !strcmp(PendingTrack(), combatTrack);
        const bool currentCombat = combatTrack[0] && !strcmp(CurrentTrack(), combatTrack);
        if (pendingCombat || (currentCombat && !PendingTrack()[0]))
        {
            InterlockedIncrement(&musicGeneration);
            THISCALL_1(void, 0x0059B380, snd);
            PendingTrack()[0] = '\0';
            CurrentTrack()[0] = '\0';
            resultGeneration = 0;
            stoppedCombatTrack = true;
        }
        const LONG generation = InterlockedCompareExchange(&musicGeneration, 0, 0);
        restore = previousTrack[0] && !PendingTrack()[0] && !CurrentTrack()[0] &&
                  (stoppedCombatTrack || (restorePreviousTrack && restoreGeneration == generation));
        restorePreviousTrack = false;
        LeaveCriticalSection(Section(snd, 0xC0));
        LeaveCriticalSection(Section(snd, 0x90));
        LeaveCriticalSection(Section(snd, 0xA8));
    }
    else
        restorePreviousTrack = false;
    if (restore)
        THISCALL_4(void, 0x0059AFB0, snd, previousTrack, previousTrackMode, 0);
    previousTrack[0] = '\0';
    combatTrack[0] = '\0';
    return result;
}
static void __stdcall StartCombatMusic(HiHook *h, h3::H3SoundManager *snd, char *name, int positionMode,
                                       int /*stopSamples*/)
{
    CancelCombatMusic();
    StopAdventureSounds();
    if (!snd || ByteAt(0x00699658))
        return;
    // Retire old pending play workers while the intro owns the soundtrack.
    // Same lock order as native pause; fast combat transitions are already set.
    EnterCriticalSection(Section(snd, 0xA8));
    EnterCriticalSection(Section(snd, 0x90));
    EnterCriticalSection(Section(snd, 0xC0));
    THISCALL_1(void, 0x0059B380, snd);
    PendingTrack()[0] = '\0';
    combatMusic.generation = InterlockedIncrement(&musicGeneration);
    LeaveCriticalSection(Section(snd, 0xC0));
    LeaveCriticalSection(Section(snd, 0x90));
    LeaveCriticalSection(Section(snd, 0xA8));
    combatMusic.hook = h;
    combatMusic.snd = snd;
    combatMusic.positionMode = positionMode;
    CopyTrack(combatMusic.track, name); // The caller's name is a stack buffer.
    if (battleActive)
        CopyTrack(combatTrack, name);
    PumpCombatMusic(); // No artificial delay if WAV playback is already over.
    if (combatMusic.hook)
    {
        combatMusicTimer = SetTimer(nullptr, 0, 25, CombatMusicTimer);
        if (!combatMusicTimer)
        {
            // Resource failure must not bring back synchronous WAV waiting.
            ReleasePreBattleWav();
            PumpCombatMusic();
            CancelCombatMusic();
        }
    }
}
static void __stdcall StartResultMusic(HiHook *h, h3::H3SoundManager *snd, char *name, int positionMode,
                                       int stopSamples)
{
    CancelCombatMusic();
    ReleasePreBattleWav();
    const bool previousRequestingResult = requestingResult;
    requestingResult = true;
    THISCALL_4(void, h->GetDefaultFunc(), snd, name, positionMode, stopSamples);
    requestingResult = previousRequestingResult;
}
static int __stdcall ResultClosed(LoHook *, HookContext *)
{
    CancelResult();
    return EXEC_DEFAULT;
}
static void __stdcall PreBattleWavNoWait(HiHook *, int /*milliseconds*/, h3::H3WavFile *wav, int sampleHandle)
{
    // Transfer the native reference to intro lifetime without ending playback.
    // No cleanup thread can race with a later reuse of the pooled sample.
    ReleasePreBattleWav();
    if (!wav || !sampleHandle)
    {
        CleanupBattleWav(wav, sampleHandle);
        return;
    }
    h3::H3SoundManager *snd = h3::H3SoundManager::Get();
    EnterCriticalSection(Section(snd, 0x90));
    preBattleWav = wav;
    preBattleSample = sampleHandle;
    preBattleWavStartedAt = GetTickCount();
    LeaveCriticalSection(Section(snd, 0x90));
}
void __stdcall OnBeforeReplay(Era::TEvent *)
{
    SetCombatTransitions(true);
    CancelCombatMusic();
    ReleasePreBattleWav();
    CancelResult();
}
void __stdcall OnLeave(Era::TEvent *)
{
    SetCombatTransitions(true);
    CancelCombatMusic();
    ReleasePreBattleWav();
    CancelResult();
    InterlockedIncrement(&musicGeneration);
    battleActive = false;
    restorePreviousTrack = false;
    previousTrack[0] = '\0';
    combatTrack[0] = '\0';
    SetCombatTransitions(false);
}

static void InstallBattleTransitions()
{
    // These hooks replace instructions inside native worker functions. Do not
    // overwrite an unknown replacement installed by another sound plugin.
    const unsigned char fadeStep[] = {0xA0, 0x58, 0x96, 0x69, 0x00};
    const unsigned char fadeInBegin[] = {0x8B, 0x3D, 0xA8, 0xA0, 0x63, 0x00};
    const unsigned char fadeOutWait[] = {0xFF, 0x15, 0xA8, 0xA0, 0x63, 0x00};
    const unsigned char battleWavCall[] = {0xE8, 0x81, 0x80, 0x13, 0x00};
    const unsigned char sampleArguments[] = {0x8B, 0x4B, 0x20, 0x6A, 0x00, 0x51};
    const unsigned char adventureArguments[] = {0x8B, 0x4D, 0x08, 0x85, 0xC9};
    const unsigned char adventureReturn[] = {0x5F, 0x5E, 0x5B, 0x5D, 0xC2, 0x14, 0x00};
    const DWORD threadSites[] = {0x005998AF, 0x005999A1, 0x0059A070, 0x0059A15F,
                                 0x0059AF90, 0x0059B177, 0x0059B202, 0x0059B362};
    bool compatible = !memcmp(reinterpret_cast<void *>(0x004626EA), battleWavCall, sizeof(battleWavCall)) &&
                      !memcmp(reinterpret_cast<void *>(0x0059B512), fadeStep, sizeof(fadeStep)) &&
                      !memcmp(reinterpret_cast<void *>(0x0059AE54), fadeStep, sizeof(fadeStep)) &&
                      !memcmp(reinterpret_cast<void *>(0x0059AE42), fadeInBegin, sizeof(fadeInBegin)) &&
                      !memcmp(reinterpret_cast<void *>(0x0059B543), fadeOutWait, sizeof(fadeOutWait)) &&
                      !memcmp(reinterpret_cast<void *>(0x0059A640), sampleArguments, sizeof(sampleArguments)) &&
                      !memcmp(reinterpret_cast<void *>(0x00418B78), adventureArguments, sizeof(adventureArguments)) &&
                      !memcmp(reinterpret_cast<void *>(0x00418CE2), adventureReturn, sizeof(adventureReturn));
    for (DWORD address : threadSites)
        compatible = compatible && ByteAt(address) == 0xE8 && address + 5 + IntAt(address + 1) == 0x0061A56C;
    if (!compatible)
    {
        //Era::WriteLog("GBFE", "Battle audio", "Unsupported music worker patches; audio fixes were not installed.");
         return;
    }
    transitionWakeEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!transitionWakeEvent)
    {
        //Era::WriteLog("GBFE", "Battle audio", "Cannot create music transition event; audio fixes were not installed.");
        return;
    }
    PatcherInstance *audio = gbf::patcher->CreateInstance("game bug fixes extended.battle audio");
    if (!audio)
    {
        CloseHandle(transitionWakeEvent);
        transitionWakeEvent = nullptr;
        return;
    }
    SetCombatTransitions(false);
    Patch *patches[] = {audio->WriteLoHook(0x0059B512, FadeOutStep),
                        audio->WriteHiHook(0x0059B543, CALL_, EXTENDED_, STDCALL_, FadeOutWait),
                        audio->WriteLoHook(0x0059AE42, FadeInBegin), audio->WriteLoHook(0x0059AE54, FadeInStep),
                        audio->WriteHiHook(0x00462600, SPLICE_, EXTENDED_, THISCALL_, StartBattle),
                        // audio->WriteHiHook(0x00462E40, SPLICE_, EXTENDED_, THISCALL_, FinishBattle),

                        // Amethyst hooks 462E42 inside Finish's prologue. Keep
                        // the code intact and intercept the manager's stop slot.
                        audio->WriteHiHook(0x0063D3EC, FUNCPTR_, EXTENDED_, THISCALL_, FinishBattle),
                        audio->WriteLoHook(0x004626EA, LoadBattleWav),
                        audio->WriteHiHook(0x00462C2B, CALL_, EXTENDED_, THISCALL_, PreBattleWavNoWait),
                        audio->WriteHiHook(0x00462C65, CALL_, EXTENDED_, THISCALL_, StartCombatMusic),
                        audio->WriteLoHook(0x0059A640, RetireReusedSample),
                        audio->WriteLoHook(0x00418B78, BlockAdventureSound),
                        audio->WriteHiHook(0x0059A4B0, SPLICE_, EXTENDED_, THISCALL_, ContinueMusic),
                        audio->WriteHiHook(0x0059AF00, SPLICE_, EXTENDED_, THISCALL_, ResumeMusic),
                        audio->WriteHiHook(0x00477235, CALL_, EXTENDED_, THISCALL_, StartResultMusic),
                        audio->WriteHiHook(0x004772E4, CALL_, EXTENDED_, THISCALL_, StartResultMusic),
                        audio->WriteLoHook(0x0047724F, ResultClosed), audio->WriteLoHook(0x004772FE, ResultClosed)};
    bool installed = true;
    for (Patch *patch : patches)
        installed = installed && patch && patch->IsApplied();
    for (DWORD address : threadSites)
    {
        LoHook *hook = audio->WriteLoHook(address, MusicThreadCall);
        installed = installed && hook && hook->IsApplied();
    }
    if (!installed)
    {
        audio->UndoAll();
        CloseHandle(transitionWakeEvent);
        transitionWakeEvent = nullptr;
        // Era::WriteLog("GBFE", "Battle audio", "Cannot install all audio hooks; audio fixes were rolled back.");
        return;
    }
    RemoveHdCombatMusicHook();
    Era::RegisterHandler(OnBeforeReplay, "OnBeforeBattleReplay");
    Era::RegisterHandler(OnLeave, "OnGameLeave");
    // Era::WriteLog("GBFE", "Battle audio",
    // PRESERVE_NON_COMBAT_MUSIC_FADES ? "Installed: preserve music fades outside combat."
    //: "Installed: immediate music transitions everywhere.");
}
static void Install(PatcherInstance *pi)
{
    // Existing sound fixes are independent of the worker compatibility check.
    pi->WriteHiHook(0x004626CF, CALL_, EXTENDED_, FASTCALL_, FanfareRandom);
    pi->WriteHiHook(0x00462C3A, CALL_, EXTENDED_, FASTCALL_, MusicRandom);
    pi->WriteHiHook(0x005998F8, CALL_, EXTENDED_, FASTCALL_, MusicRandom);
    pi->WriteLoHook(0x043D8DE, js_BattleStack_InitAssets_BeforeInitShootingSound);
    InstallBattleTransitions();
}
} // namespace BattleAudio
void InstallAudio(PatcherInstance *pi)
{
    BattleAudio::Install(pi);
}
} // namespace gbf
