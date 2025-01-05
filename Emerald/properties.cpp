#include "emerald.h"


int luck_bonuses[NEW_ARTS_AMOUNT];
int morale_bonuses[NEW_ARTS_AMOUNT];
int luck_bonuses_bp[NEW_ARTS_AMOUNT];
int morale_bonuses_bp[NEW_ARTS_AMOUNT];

int allow_fly[NEW_ARTS_AMOUNT];
int allow_water[NEW_ARTS_AMOUNT];

int spell_immunity[NEW_ARTS_AMOUNT][SPELLS_AMOUNT];

int autocast[NEW_ARTS_AMOUNT];

int crattack_bonuses[NEW_ARTS_AMOUNT];
int crdefence_bonuses[NEW_ARTS_AMOUNT];
int dmgmin_bonuses[NEW_ARTS_AMOUNT];
int dmgmax_bonuses[NEW_ARTS_AMOUNT];
int speed_bonuses[NEW_ARTS_AMOUNT];
int hp_bonuses[NEW_ARTS_AMOUNT];
int shots_bonuses[NEW_ARTS_AMOUNT];
int casts_bonuses[NEW_ARTS_AMOUNT];



char* erm_on_battlestart[NEW_ARTS_AMOUNT]; 
char* erm_on_creature_param_init[NEW_ARTS_AMOUNT]; 

char* erm_on_ae0[NEW_ARTS_AMOUNT]; 
char* erm_on_ae1[NEW_ARTS_AMOUNT]; 
char* erm_on_ba52[NEW_ARTS_AMOUNT]; 
char* erm_on_ba53[NEW_ARTS_AMOUNT]; 
char* erm_on_timer[NEW_ARTS_AMOUNT]; 

int GetLuckBonus(int id, int customdata)
{

	return luck_bonuses[id];
}

int GetMoraleBonus(int id, int customdata)
{
	return morale_bonuses[id];
}

int GetLuckBonusFromBP(int id, int customdata)
{
	return luck_bonuses_bp[id];
}

int GetMoraleBonusFromBP(int id, int customdata)
{
	return morale_bonuses_bp[id];
}

int IsArtifactAllowWaterWalk(int id, int customdata)
{
	return false;
}

int IsArtifactAllowFly(int id, int customdata)
{
	return false;
}

//результат - шанс отражения заклятия. 0 - нифига, 1 - иммунитет.
double AbsorbDamage(int id, int customdata, int spell)
{
	//if (spell==SPELL_ARMAGEDDON)
	//	return 1;

	return spell_immunity[id][spell];
}

void BattleStartAction(int id, int customdata)
{
	if(autocast[id]!=0) CastArtifactSpell(id,autocast[id],50);

	return;
}


int CalculateAttackBonus(int art, int customdata)
{
	return crattack_bonuses[art];
}

int CalculateDmgHighBonus(int art, int customdata)
{
	return dmgmax_bonuses[art];
}

int CalculateDmgLowBonus(int art, int customdata)
{
	return dmgmin_bonuses[art];
}

int CalculateDefenceBonus(int art, int customdata)
{
	return crdefence_bonuses[art];
}

int CalculateHealthBonus(int art, int customdata)
{
	return hp_bonuses[art];
}

int CalculateShotsBonus(int art, int customdata)
{
	return shots_bonuses[art];
}

int CalculateSpeedBonus(int art, int customdata)
{
	return speed_bonuses[art];
}

int CalculateSpellsBonus(int art, int customdata)
{
	return casts_bonuses[art];
}
