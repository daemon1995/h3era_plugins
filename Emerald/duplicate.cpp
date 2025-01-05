#include "emerald.h"


extern int IsArtifactAllowWaterWalk(int id, int customdata);
extern int IsArtifactAllowFly(int id, int customdata);




int __stdcall NewHasArtifact(HiHook* h, HERO* hero, int art)
{
	int ret = CALL_2(int, __thiscall, h->GetDefaultFunc(), hero, art);

	if (h->GetReturnAddress() < 0x700000 && h->GetReturnAddress() > 0x400000) //морочим только SoD;
	{
		for (int i=0; i!=19; i++)
		{
			if (art==ARTIFACT_SPELLBOOK && hero->IArt[i][0]!=-1 &&  //если герои лезут за книжкой, то 
				GetArtifactRecord(hero->IArt[i][0])->rank & 0x40)	//мы проверяем наличие спеллбуков во всех слотоах и подменяем ответ 
			{
				//sprintf(buf,"NewHasArtifact::Spellbook found at slot %i", i);
				//WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),(const void*)buf,64,0,0);
				return true;
			}


			if (art==ARTIFACT_ANGELWINGS && hero->IArt[i][0]!=-1 && 
				IsArtifactAllowFly(hero->IArt[i][0],hero->IArt[i][1]))
			{
				return true;
			}


			if (art==ARTIFACT_WATERBOOTS && hero->IArt[i][0]!=-1 && 
				IsArtifactAllowWaterWalk(hero->IArt[i][0],hero->IArt[i][1]))
			{
				return true;
			}
		}
	}

	return ret;
}

int __stdcall NewHasArtifactInBP(HiHook* h, HERO* hero, int art)
{
	int ret = CALL_2(int, __thiscall, h->GetDefaultFunc(), hero, art);

	if (h->GetReturnAddress() < 0x700000 && h->GetReturnAddress() > 0x400000)//морочим только SoD;
	{
		for (int i=0; i!=19; i++)
		{
			if (art==ARTIFACT_SPELLBOOK && hero->IArt[i][0]!=-1 && (GetArtifactRecord(hero->IArt[i][0])->rank & 0x40))
			{
				//sprintf(buf,"NewHasArtifactInBP::Spellbook found at slot %i, artid:%i rank: %x", i, hero->IArt[i][0], GetArtifactRecord(hero->IArt[i][0]));
				//WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),(const void*)buf,64,0,0);
				return true;
			}

		}

		for (int i=0; i!=64; i++)
		{
			if (art==ARTIFACT_SPELLBOOK && hero->OArt[i][0]!=-1  && (GetArtifactRecord(hero->OArt[i][0])->rank & 0x40))
			{
				//sprintf(buf,"NewHasArtifactInBP::Spellbook found at bp slot %i", i);
				//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),(const void*)buf,64,0,0);
				return true;
			}
		}

		//собственно, спеллбуком полезность этого хука и ограничивается. 
		//Ну, вроде, еще машины где-то засветились.
	}
	return ret;
}