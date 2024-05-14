#include "pch.h"

extern ArtifactsData artifactsData;


int __stdcall Game__GetPlayerGoldIncome(HiHook*h, const H3Game* game, const UINT playerId, char newWeek)
{

	int income = THISCALL_3(int, h->GetDefaultFunc(), game, playerId, newWeek);

	income -= 22220;

	return income;
}
_LHF_(Game__GetPlayerGoldIncome_Low)
{
	int currentIncome = IntAt(c->ebp - 0x4);
	if (currentIncome>0)
	{
		int newIncome = 0;

		auto* player = reinterpret_cast<H3Player*>(c->ebx);
		int playerId = player->ownerID;
		int thisPlayerTeam = P_Game->mapInfo.playerTeam[playerId];

		for (auto& it : artifactsData.artifactsWhichAddGold)
		{
			UINT artId = it.first;
			INT income = it.second;
			if (income)
			{
				if (income > 0)
				{
					int artsNum = THISCALL_2(int, 0x4BA890, player, artId);
					newIncome += artsNum * income;
				}
				else
				{

					for (size_t i = 0; i < 8; i++)
					{
						if (!P_Game->isDead[i] && P_Game->mapInfo.playerTeam[i] != thisPlayerTeam)
						{
							int artsNum = THISCALL_2(int, 0x4BA890, &P_Game->players[i], artId);
							newIncome += artsNum * income;

						}
					}
				}
			}
		}
		currentIncome += newIncome;
		IntAt(c->ebp - 0x4) = currentIncome;
	}

	return EXEC_DEFAULT;

}
_LHF_(Game__NewDay)
{
	for (size_t i = 0; i < 8; i++)
		if (P_Game->players[i].playerResources[eResource::GOLD] < 0)
			P_Game->players[i].playerResources[eResource::GOLD] = 0;


	return EXEC_DEFAULT;

}
void IncomeProcedure::SetPatches(PatcherInstance* _PI)
{

	//_PI->WriteHiHook(0x4C75F0, THISCALL_, Game__GetPlayerGoldIncome);
	if (artifactsData.artifactsWhichAddGold.size())
	{
		_PI->WriteLoHook(0x4C77A3, Game__GetPlayerGoldIncome_Low);
		_PI->WriteLoHook(0x4C7EE7, Game__NewDay);
	
	}

	
}
