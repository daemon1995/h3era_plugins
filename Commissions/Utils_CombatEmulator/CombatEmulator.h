#pragma once
#define _WOG_
#include "framework.h"
class CombatEmulator
{

    BOOL isIinited = false;
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

  public:
    static CombatEmulator *instance;

  public:
    void AfterCombatEnd();
    void BeforeCombatStart(const BOOL blockCombatResultDlg, const BOOL blockMagic, const BOOL blockRunning);
    static char __stdcall GameMgr_CreateSaveGameFile(HiHook *h, H3Game *gameMgr, LPCSTR saveName, const DWORD a3,
                                                     const DWORD a4, const DWORD a5, const DWORD a6);

    static Patch **GetPatchesArray()
    {
        return instance->patches;
    }
    static void Init();
};
