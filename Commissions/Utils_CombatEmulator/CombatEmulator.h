#pragma once
#define _WOG_
#include "framework.h"
class CombatEmulator
{

    BOOL isInited = false;
    BOOL blockingPatchesApplied = false;
    INT32 mouseNoChangePointer = 0;
    INT32 combatShowCursorShadow = 0;
    union {
        struct
        {
            Patch *patchToBlockDlgScreen;
            Patch *patchToBlockAiMagic;
            Patch *patchToBlockRun;
            Patch *patchToAfterCombatProc;
        };
        Patch *patches[4] = {};
    };

  protected:
    void CreatePatches(PatcherInstance *_pi);
    void RestoreMouseState();
    static int __stdcall CombatMgr_SkipResults(LoHook *h, HookContext *c);
    static void __stdcall MouseMgr_SetCursor(HiHook *h, H3MouseManager *mouse, const int frame, const int type);

  public:
    static CombatEmulator *instance;

  public:
    void AfterCombatEnd();
    void BeforeCombatStart(const BOOL blockCombatResultDlg, const BOOL blockMagic, const BOOL blockRunning);
    static char __stdcall GameMgr_CreateSaveGameFile(HiHook *h, H3Game *gameMgr, LPCSTR saveName, const DWORD a3,
                                                     const DWORD a4, const DWORD a5, const DWORD a6);

    static Patch **GetPatchesArray()
    {
        return instance ? instance->patches : nullptr;
    }
    static void Init();
};
