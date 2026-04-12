#pragma once
#include "framework.h"
class CombatEmulator
{

    static CombatEmulator *instance;

  public:
    void BeforeCombatStart();
    void AfterCombatEnd();
    static char __stdcall GameMgr_CreateSaveGameFile(HiHook *h, H3Game *gameMgr, LPCSTR saveName, const DWORD a3,
                                                     const DWORD a4, const DWORD a5, const DWORD a6);

    static void CreatePatches(PatcherInstance *_pi);
    static void Init();
};
