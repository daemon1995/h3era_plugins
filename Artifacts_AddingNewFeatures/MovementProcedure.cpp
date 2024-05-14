#include "pch.h"

extern PluginText pluginText;
extern ArtifactsData artifactsData;

int __stdcall GetStepCost(HiHook* h,
	H3MapItem* mip,
	char direction,
	signed int MovementLeft,
	int PathfindSkill,
	int road,
	int a6,
	int a7,
	int land,
	int NomadsCount)
{

	// Get Active Hero;
	auto* hero = P_ActivePlayer->GetActiveHero();
	if (hero)
	{
		// if hero
		INT8 targetLand = mip->land; // get target land type
		const auto& arts = artifactsData.pathFindingData;
		for (const auto& art : arts) // loop arts with pathfinding
		{
			if (hero->WearsArtifact(art.first) && art.second & (1 << targetLand)) // check if hero wears that art and land is correct
			{
				PathfindSkill = eSecSkillLevel::EXPERT; // set max level for PathfindSkill
				break;
			}
		}
	}

	// call native function
	int stepCost = FASTCALL_9(int, h->GetDefaultFunc(), mip, direction, MovementLeft, PathfindSkill, road, a6, a7, land, NomadsCount);
	if (hero)
	{

		//bool isWater = mip->land == 8;

		auto& arts = mip->land == 8 ? artifactsData.waterMultiplierData : artifactsData.landMultiplierData;


		int stepCostChanger = 0;


		for (const auto& art : arts) // loop arts with pathfinding
		{
			if (hero->WearsArtifact(art.first)) // check if hearo wears that art and land is correct
			{
				stepCostChanger += art.second;
			}
		}
		if (stepCostChanger)
		{
			double multiplier = 100 - stepCostChanger;
			if (multiplier < 0) multiplier = 0;
			multiplier *= 0.01;

			stepCost = static_cast<int>(std::ceil(static_cast<double>(stepCost) * multiplier));

		}

	}

	return stepCost;

}


_LHF_(Hero__GetLandMovepoints)
{

	int movepointBonus = 0;
	H3Hero* hero = reinterpret_cast<H3Hero*>(c->ebx);

	for (auto& it : artifactsData.landMovepointsBonus)
		if (hero->WearsArtifact(it.first))
			movepointBonus += it.second;

	c->edi += movepointBonus;


	return EXEC_DEFAULT;

}

int main( int argc , char** argv)
{
	return 0;
}

_LHF_(Hero__GetWaterMovepoints)
{

	int movepointBonus = 0;
	H3Hero* hero = reinterpret_cast<H3Hero*>(c->ebx);

	for (auto& it : artifactsData.waterMovepointsBonus)
		if (hero->WearsArtifact(it.first))
			movepointBonus += it.second;


	if (movepointBonus)
		c->esi += movepointBonus;

	return EXEC_DEFAULT;

}


void MovementProcedure::SetPatches(PatcherInstance* _PI)
{

	if (artifactsData.pathFindingData.size() + artifactsData.landMultiplierData.size() + artifactsData.waterMultiplierData.size())
	{
		_PI->WriteHiHook(0x4B14A0, FASTCALL_, GetStepCost);
	}

	if (artifactsData.landMovepointsBonus.size())
		_PI->WriteLoHook(0x4E4F47, Hero__GetLandMovepoints);

	if (artifactsData.waterMovepointsBonus.size())
		_PI->WriteLoHook(0x4E4C7E, Hero__GetWaterMovepoints);

}
