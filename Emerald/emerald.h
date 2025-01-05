#pragma once
//ƒанные, общие дл€ всего модул€.

//#include "pch.h"
//#include "..\..\include\era.h"




#define OLD_ARTS_AMOUNT	171

#define NEW_ARTS_AMOUNT 700
#define NEW_ARTS_AMOUNT 700

#define SPELLS_AMOUNT	128 //зачем жалеть пам€ть?

#define PINSTANCE_MAIN "emerald"

//Savegame
typedef struct
{
	char    used_artifacts[NEW_ARTS_AMOUNT];// 0x4E224
	char allowed_artifacts[NEW_ARTS_AMOUNT];// 0x4E2B4
}GAMEDATA;

//extern GAMEDATA save;
//extern Patcher * globalPatcher;
//extern PatcherInstance * emerald;

extern ART_RECORD newtable[NEW_ARTS_AMOUNT];
extern ART_BONUS newbtable[NEW_ARTS_AMOUNT];
extern char* arteventtable[NEW_ARTS_AMOUNT+1]; 
extern char  artspelltable[NEW_ARTS_AMOUNT];
//New tables


extern int luck_bonuses[NEW_ARTS_AMOUNT];
extern int morale_bonuses[NEW_ARTS_AMOUNT];
extern int luck_bonuses_bp[NEW_ARTS_AMOUNT];
extern int morale_bonuses_bp[NEW_ARTS_AMOUNT];


extern int allow_fly[NEW_ARTS_AMOUNT];
extern int allow_water[NEW_ARTS_AMOUNT];

extern int spell_immunity[NEW_ARTS_AMOUNT][SPELLS_AMOUNT];

extern int autocast[NEW_ARTS_AMOUNT];



extern int crattack_bonuses[NEW_ARTS_AMOUNT];
extern int crdefence_bonuses[NEW_ARTS_AMOUNT];
extern int dmgmin_bonuses[NEW_ARTS_AMOUNT];
extern int dmgmax_bonuses[NEW_ARTS_AMOUNT];
extern int speed_bonuses[NEW_ARTS_AMOUNT];
extern int hp_bonuses[NEW_ARTS_AMOUNT];
extern int shots_bonuses[NEW_ARTS_AMOUNT];
extern int casts_bonuses[NEW_ARTS_AMOUNT];

//bonuses


//events
extern char* erm_on_ae0[NEW_ARTS_AMOUNT]; 
extern char* erm_on_ae1[NEW_ARTS_AMOUNT];

extern char* erm_on_ba52[NEW_ARTS_AMOUNT]; 
extern char* erm_on_ba53[NEW_ARTS_AMOUNT];
extern char* erm_on_timer[NEW_ARTS_AMOUNT];

extern char* erm_on_creature_param_init[NEW_ARTS_AMOUNT];
extern char* erm_on_battlestart[NEW_ARTS_AMOUNT]; 


extern "C" __declspec(dllexport) void CastArtifactSpell(int artifact, int spell, int duration);
extern "C" __declspec(dllexport)void ExecErmSequence(char* command);
