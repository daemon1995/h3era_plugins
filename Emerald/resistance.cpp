#include "emerald.h"

extern double AbsorbDamage(int id, int customdata, int spell);

double __stdcall ResistanceHook(HiHook* h, int spell, int creature, HERO* caster, HERO* victim)
{
	//типичная проверка выглядит как 
	// if (spell == .. && victim && HasArtifact(victim, ..) return 0.0; else return 1.0;

	double o = CALL_4(double, __fastcall, h->GetDefaultFunc(), spell, creature, caster, victim);

	if (victim)
	{
		for (int i=0; i!=19; i++)
		{
			if(victim->IArt[i][0]!=-1)
			{
				o-=AbsorbDamage(victim->IArt[i][0],victim->IArt[i][1],spell);
			}
		}
	}
	
	return o<0.0?0.0:o;
}