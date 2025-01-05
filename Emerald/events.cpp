#include "emerald.h"

extern void BattleStartAction(int art, int customdata);

extern int CalculateAttackBonus(int art, int customdata);
extern int CalculateDmgHighBonus(int art, int customdata);
extern int CalculateDmgLowBonus(int art, int customdata);
extern int CalculateDefenceBonus(int art, int customdata);
extern int CalculateHealthBonus(int art, int customdata);
extern int CalculateShotsBonus(int art, int customdata);
extern int CalculateSpeedBonus(int art, int customdata);
extern int CalculateSpellsBonus(int art, int customdata);

int __stdcall BattleStartHook(LoHook* h, HookContext* c)
{
	HERO *hero = (HERO*)c->edi;
	
	
	for (int i=0; i!=19; i++)
	{
		if(hero->IArt[i][0]!=-1)
		{
			BattleStartAction(hero->IArt[i][0], hero->IArt[i][1]);

			if(erm_on_battlestart[hero->IArt[i][0]]!=0)
				ExecErmSequence(erm_on_battlestart[hero->IArt[i][0]]);
		}
	}

	return EXEC_DEFAULT;
}



int __stdcall OnCreatureParamInit (HiHook* h, HERO* hero, int creature, MONSTER_PROP* monster)
{
	CALL_3(int ,__thiscall, h->GetDefaultFunc(), hero, creature, monster);

	for (int i=0; i!=19; i++)
	{
		if(hero->IArt[i][0]!=-1)
		{
			monster->attack+=CalculateAttackBonus		(hero->IArt[i][0], hero->IArt[i][1]);
			monster->damage_high+=CalculateDmgHighBonus	(hero->IArt[i][0], hero->IArt[i][1]);
			monster->damage_low+=CalculateDmgLowBonus	(hero->IArt[i][0], hero->IArt[i][1]);
			monster->defence+=CalculateDefenceBonus		(hero->IArt[i][0], hero->IArt[i][1]);
			monster->hp+=CalculateHealthBonus			(hero->IArt[i][0], hero->IArt[i][1]);
			monster->shots+=CalculateShotsBonus			(hero->IArt[i][0], hero->IArt[i][1]);
			monster->speed+=CalculateSpeedBonus			(hero->IArt[i][0], hero->IArt[i][1]);
			monster->spells+=CalculateSpellsBonus		(hero->IArt[i][0], hero->IArt[i][1]);

			
			if(erm_on_creature_param_init[hero->IArt[i][0]]!=0)
			{
				*(int*)0x27F9970 = (int)hero;
				ErmV[998] = hero->x;
				ErmV[999] = hero->y;
				ErmV[1000] = hero->l;

				ErmF[1000]= !CALL_1(int, __thiscall, 0x4BAA60, *(int*)0x69CCFC);

				ErmV[1]=hero->Id;
				ErmV[2]=creature;
				ErmV[3]=(int)monster;

				if(erm_on_creature_param_init[hero->IArt[i][0]]!=0)
					ExecErmSequence(erm_on_creature_param_init[hero->IArt[i][0]]);

			}
		}
	}

	return 0;
}


void __stdcall OnEquip (PEvent e)
{
	int artifact = *(int*)0x27F9964;

	if(erm_on_ae0[artifact])
	{
		ExecErmSequence(erm_on_ae0[artifact]);
	}
}

void __stdcall OnUnequip (PEvent e)
{
	int artifact = *(int*)0x27F9964;
	if(erm_on_ae1[artifact])
	{
		ExecErmSequence(erm_on_ae1[artifact]);
	}
}

void __stdcall OnBattleStart(PEvent e)
{
       //int combat_manager = (*((int*)0x699420));



	//HERO *attacker = (HERO*)(*(int*)(combat_manager+21452));
	//HERO *defender = (HERO*)(*(int*)(combat_manager+21456));
	
	//   ExecErmCmd("BA:H0/?v1;");
	//   ExecErmCmd("IF:L^%V1^;");


	//HERO *attacker = (HERO*)GetHeroRecord(ErmV[1]);
	
	/*for (int i=0; i!=19; i++)
	{
		if(attacker->IArt[i][0]!=-1)
		{
			ErmV[1]=0;
			if(erm_on_ba52[attacker->IArt[i][0]]!=0)
				ExecErmSequence(erm_on_ba52[attacker->IArt[i][0]]);
		}
	}

	   ExecErmCmd("BA:H1/?v1;");
	HERO *defender = (HERO*)GetHeroRecord(ErmV[1]);
	
	/if(ErmV[1]!=-2)
	{
	for (int i=0; i!=19; i++)
	{
		if(defender->IArt[i][0]!=-1)
		{
			ErmV[1]=1;
			if(erm_on_ba52[defender->IArt[i][0]]!=0)
				ExecErmSequence(erm_on_ba52[defender->IArt[i][0]]);
		}
	}
	}*/


}

void __stdcall OnBattleEnd(PEvent e)
{
	
	//   ExecErmCmd("BA:H0/?v1;");
	//   ExecErmCmd("IF:L^%V1^;");

     /*  char* combat_manager = (char*)(*((int*)0x699420));

	HERO *attacker = (HERO*)(*(int*)(combat_manager+21452));
	HERO *defender = (HERO*)(*(int*)(combat_manager+21456));*/
	
	
	/*for (int i=0; i!=19; i++)
	{
		if(attacker->IArt[i][0]!=-1)
		{
			ErmV[1]=0;
			if(erm_on_ba53[attacker->IArt[i][0]]!=0)
				ExecErmSequence(erm_on_ba53[attacker->IArt[i][0]]);
		}
	}


	if(defender)
	{
		for (int i=0; i!=19; i++)
		{
			if(defender->IArt[i][0]!=-1)
			{
				ErmV[1]=1;
				if(erm_on_ba53[defender->IArt[i][0]]!=0)
					ExecErmSequence(erm_on_ba53[defender->IArt[i][0]]);
			}
		}
	}*/
}

void __stdcall OnNewDay(PEvent e)
{
	int owner = *(int*)(e->Data);
	HERO* hero;

	for(int i=0; i!=156; i++)
	{
		if((hero = (HERO*)GetHeroRecord(i))->Owner == owner)
		{
			for (int j=0; j!=19; j++)
			{
				if(hero->IArt[j][0]!=-1)
				{
					ErmV[1]=owner;
					ErmV[2]=hero->Number;
					ErmV[3]=*(1+(int*)(e->Data));
					if(erm_on_timer[hero->IArt[j][0]]!=0)
						ExecErmSequence(erm_on_timer[hero->IArt[j][0]]);
				}
			}
		}
	}

}