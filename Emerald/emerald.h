#pragma once

// #include "..\__include\era.h"
#include <windows.h>
#include <cstdio>
#include <stdlib.h>
//#include "Tchar.h"
#include <io.h>
#include "..\__include\era.h"

#include "..\__include\heroes.h"

#include "..\__include\patcher_x86_commented.hpp"
//#include "lib\patcher_x86.hpp"
// #include "lib\H3API.hpp"
#include "../../__include__/H3API/single_header/H3API.hpp"


////using Era::;
#define PEvent Era::TEvent*
#define ErmV Era::v
#define ErmF Era::f
// using Era::ExecErmCmd;
// using Era::ConnectEra;
// using Era::RegisterHandler;
// using Era::ReadSavegameSection;
// using Era::WriteSavegameSection;


inline bool FileExists(const char* chPath) {
	return(_access(chPath, 0) == 0);
}

//#define OLD_ARTS_AMOUNT	171
//#define NON_BLANK_ARTS_AMOUNT	161
// #define NEW_ARTS_AMOUNT 1000
//#define char_table_size 512
//#define int_tuple_size 256

//#define SPELLS_AMOUNT	128		//зачем жалеть память?

#define PINSTANCE_MAIN "emerald3_3"

//Savegame
#include"MyTypes.h"
/*
inline void init_GAMEDATA_footer(void) {
	strcpy(save.footer, "here is emerald.");
}
*/

extern GAMEDATA save;
extern GAMEDATA2 no_save;
//static ART_RECORD* EmeraldArtBase = no_save.newtable;

extern ART_RECORD* EmeraldArtNewTable; // no_save.newtable;
extern ART_BONUS* EmeraldArtNewBTable; // no_save.newbtable;

extern Patcher * globalPatcher;
extern PatcherInstance * emerald;


extern int   (*isReceptive)(int);
extern int   (*Creature2Artifact)(int);
extern int   (*Creature2Artifact_2)(int);

extern void (*Grandmaster_Magic_Update_Spells)(void* hero);

extern "C" __declspec(dllexport) void CastArtifactSpell(int artifact, int spell, int duration);
extern "C" __declspec(dllexport) void ExecErmSequence(char* command);

inline void pause(void) {
	MessageBoxA(0, "Program Interuption" , "PAUSE" , 0);
}

#include<Windows.h>

#ifndef _LHF_
// * predefined macro for LoHook functions
// * LoHook *h and HookContext *c are pre-defined
// * e.g. _LHF_(MyFunction) {}
#define _LHF_(func) int __stdcall func(LoHook *h, HookContext *c)
#endif

//#define Disable_Emerald_Combos
//#define Disable_Emerald_AI
//#define Disable_Emerald_Events
//#define Disable_ResistanceHook
//#define Disable_Emerald_duplicate_cpp