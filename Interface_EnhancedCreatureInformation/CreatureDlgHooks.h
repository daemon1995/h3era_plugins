#pragma once
#include "pch.h"

// Runtime callbacks used by widgets created by CreatureDlgHandler.
void __fastcall CreatureSkillsScrollbarProc(INT32 tick, H3BaseDlg *baseDlg);

// Hook installation entry points used from dllmain.cpp.
void Dlg_CreatureInfo_HooksInit(PatcherInstance *pi);
void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance *pi);
