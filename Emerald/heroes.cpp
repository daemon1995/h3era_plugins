//#include "stdafx.h"
#include "heroes.h"


//функции
//функции
int(__thiscall *HasArtifact)(void *hero, int art) =
      (int(__thiscall *)(void*, int))(0x4D9460);
		//проверка на наличие артефакта

int(__thiscall *HasArtifactInBackpack)(void *hero, int art) =
      (int(__thiscall *)(void*, int))(0x4D9420);
		//проверка на наличие артефакта

int(__thiscall *HasCreature)(void *hero, int creature) =
      (int(__thiscall *)(void*, int))(0x44AA90);
		//проверка на наличие существа. Герой берется со смещением в 91h

int(__thiscall *ShowGrailMap)(void *advManager) =
      (int(__thiscall *)(void*))(0x41A750);
		//показать граальную карту

int(__thiscall *ShowThiefGuild)(void *_this, int amount) =
      (int(__thiscall *)(void*, int))(0x5C8590);
		//показать граальную карту

char(__thiscall *sub_5A43E0)(void* combatManager, int a2, int a3, int a4, int a5, int a6)=
      (char(__thiscall *)(void* ,int , int , int , int , int))(0x5A43E0);
	
int (__thiscall *CastBattleSpell)(void* combatManager, int spell, signed int target, unsigned int source, int a5, int mastery, int spellpower) =
      (int (__thiscall *)(void* ,int , signed int , unsigned int , int, int , int ))(0x5A0140);
		//каст спелла в бою

int (__thiscall *SummonCreature)(void *combat_manager, int spell, int creature, int spellpower, int unk) =
      (int (__thiscall *)(void*, int, int, int, int))(0x5A7390);
      //вызов существа спеллом

int (__fastcall *ShowMSG)(const char *text, int type, int f1, int f2, int f3, int f4, int f5, int f6, int f7, int f8, int f9, int f10) =
      (int (__fastcall *)(const char*, int, int, int, int, int, int, int, int, int, int, int))(0x4F6C00);

void (__cdecl *hprintf)(int _69D800, char* pattern, ...) =
        (void (__cdecl *)(int, char*, ...))(0x553C40);

void (__cdecl *CallERM)(int num) =
        (void (__cdecl *)(int))(0x74CE30);


int(__thiscall *RandArtGenerator)(int _this, int _class) =
      (int(__thiscall *)(int, int))(0x4C9190);


//переходники
void* HeroStructPointer(char hero)
{
	return (void*)((*(int*)0x00699538) + 0x21620 + 0x492*hero);
}

int* BattleStackParam(int stack, int param_offset)
{
        char* combatManager = (char*)(*((int*)0x699420));
        return( (int*)(combatManager + 0x54CC + stack*0x548 + 4*param_offset ));
}

ART_RECORD* GetArtifactRecord(int number)
{
	return (ART_RECORD*)((*(int*)0x660B68)+number*sizeof(ART_RECORD));
}

MONSTER_PROP* GetMonsterRecord(int number)
{
	return (MONSTER_PROP*)((*(int*)0x47ADD1)+number*sizeof(MONSTER_PROP));
}

char* GetHeroRecord(int number)
{
	return (char*)(( *(int*)0x699538)+ 0x21620 + number*0x492);
}

//работа с памятью
void WriteHook( void *pOriginal, void *pNew, char type )
{
    *(char*)pOriginal = (type==HOOKTYPE_JUMP)?OPCODE_JUMP:OPCODE_CALL;
    *(void**)( (char*)pOriginal + 1 ) = (void*)( (char*)pNew - (char*)pOriginal - HOOK_SIZE );
}


