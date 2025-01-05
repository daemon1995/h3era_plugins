#include "emerald.h"


extern int GetLuckBonus(int art, int customdata);
extern int GetMoraleBonus(int art, int customdata);
extern int GetLuckBonusFromBP(int art, int customdata);
extern int GetMoraleBonusFromBP(int art, int customdata);


int __stdcall LuckTextHook(LoHook* h, HookContext* c)
{
	HERO *hero = (HERO*)(c->ebx);

	/*char *s = "\x0AВрожденное везение +1";
	CALL_3(void, __thiscall, 0x41B2A0, (int*)(c->ebp-0x28), s, strlen(s));
	*(int*)(c->ebp-0x10) += 1;*/

	char buf[256];

	int tmp[NEW_ARTS_AMOUNT];
	memset(tmp,0,sizeof(tmp));

	int l = 0;

	for (int i=0; i!=64; i++)
		if(hero->OArt[i][0]!=-1 && (l=GetLuckBonusFromBP(hero->OArt[i][0],hero->OArt[i][1])))
			tmp[hero->OArt[i][0]]=l;
	
	for (int i=0; i!=19; i++)
		if(hero->IArt[i][0]!=-1 && (l=GetLuckBonus(hero->IArt[i][0],hero->IArt[i][1])))
			tmp[hero->IArt[i][0]]=l;
	
	for(int i=0; i!=NEW_ARTS_AMOUNT; i++)
	{
		if (tmp[i]!=0)
		{
			sprintf(buf,"\x0A%s %s%i",GetArtifactRecord(i)->name, tmp[i]>0?"+":"",tmp[i]);
			CALL_3(void, __thiscall, 0x41B2A0, (int*)(c->ebp-0x28), buf, strlen(buf));
			*(int*)(c->ebp-0x10) += tmp[i];
		}
	}


	return EXEC_DEFAULT;
}


int __stdcall LuckValueHook(LoHook* h, HookContext* c)
{
	HERO *hero = (HERO*)(c->esi);

	int tmp[NEW_ARTS_AMOUNT];
	memset(tmp,0,sizeof(tmp));

	int l = 0;

	for (int i=0; i!=64; i++)
		if(hero->OArt[i][0]!=-1 && (l=GetLuckBonusFromBP(hero->OArt[i][0],hero->OArt[i][1])))
			tmp[hero->OArt[i][0]]=l;
	
	for (int i=0; i!=19; i++)
		if(hero->IArt[i][0]!=-1 && (l=GetLuckBonus(hero->IArt[i][0],hero->IArt[i][1])))
			tmp[hero->IArt[i][0]]=l;


	for(int i=0; i!=NEW_ARTS_AMOUNT; i++)
	{
		*(int*)(c->ebp+0x0C)+= tmp[i];
	}


	return EXEC_DEFAULT;
}



int __stdcall MoraleTextHook(LoHook* h, HookContext* c)
{
	HERO *hero = (HERO*)(c->ebx);
	char buf[256];

	int tmp[NEW_ARTS_AMOUNT];
	memset(tmp,0,sizeof(tmp));


	int l = 0;
	for (int i=0; i!=64; i++, l=0)
		if(hero->OArt[i][0]!=-1 && (l=GetMoraleBonusFromBP(hero->OArt[i][0],hero->OArt[i][1])))
			tmp[hero->OArt[i][0]]=l;

	for (int i=0; i!=19; i++, l=0)
		if(hero->IArt[i][0]!=-1 && (l=GetMoraleBonus(hero->IArt[i][0],hero->IArt[i][1])))
			tmp[hero->IArt[i][0]]=l;

	for(int i=0; i!=NEW_ARTS_AMOUNT; i++)
	{
		if (tmp[i]!=0)
		{
			sprintf(buf,"\x0A%s %s%i",GetArtifactRecord(i)->name, tmp[i]>0?"+":"",tmp[i]);
			CALL_3(void, __thiscall, 0x41B2A0, (int*)(c->ebp-0x28), buf, strlen(buf));
			*(int*)(c->ebp-0x10) += tmp[i];
		}
	}

	return EXEC_DEFAULT;
}


int __stdcall MoraleValueHook(LoHook* h, HookContext* c)
{
	int tmp[NEW_ARTS_AMOUNT];
	memset(tmp,0,sizeof(tmp));

	HERO *hero = (HERO*)(c->edi);
	
	int l = 0;

	for (int i=0; i!=64; i++)
		if(hero->OArt[i][0]!=-1 && (l=GetMoraleBonusFromBP(hero->OArt[i][0],hero->OArt[i][1])))
			tmp[hero->OArt[i][0]]=l;


	for (int i=0; i!=19; i++)
		if(hero->IArt[i][0]!=-1 && (l=GetMoraleBonus(hero->IArt[i][0],hero->IArt[i][1])))
			tmp[hero->IArt[i][0]]=l;

	
	for(int i=0; i!=NEW_ARTS_AMOUNT; i++)
	{
		*(int*)(c->ebp+0x0C)+= tmp[i];
	}

	return EXEC_DEFAULT;
}
