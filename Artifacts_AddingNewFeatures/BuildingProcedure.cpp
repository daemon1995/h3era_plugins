#include "pch.h"

extern ArtifactsData artifactsData;

void Town_AffectBuildingCost(const H3Town* town, int* costArray)
{
	int discount = 0;
	H3Hero* heroes[2] = { town->GetGarrisonHero(),town->GetVisitingHero() };

	const auto& arts = artifactsData.buildingCostChanger;

	for (size_t i = 0; i < 2; i++)
	{

		H3Hero* hero = heroes[i];
		hero->level;
		if (hero)
			for (const auto& art : arts)
				if (hero->WearsArtifact(art.first))
					discount += art.second;

	}
	if (discount)
	{
		double multiplier = 100 - discount;
		if (multiplier < 0) multiplier = 0;
		multiplier *= 0.01;
		for (size_t i = 0; i < 7; i++)
		{
			double price = static_cast<double>(costArray[i]) * multiplier;
			costArray[i] = static_cast<int>(std::ceil(price));
		}
	}

}


_LHF_(AITown_BuildingBuilt)
{
	Town_AffectBuildingCost(reinterpret_cast<H3Town*>(c->esi), reinterpret_cast<int*>(c->edi));
	return EXEC_DEFAULT;
}
void __stdcall Town_GetBuildingCost(HiHook* h, H3Town* town, signed int build_id, int* ret_array)
{
	THISCALL_3(void, h->GetDefaultFunc(), town, build_id, ret_array);
	Town_AffectBuildingCost(town, ret_array);
}



__int16 __stdcall Town_GetDisplayedBuildingCost(HiHook* h, H3Town* town, signed int build_id, int* resources, int* costs)
{
	__int16 result = THISCALL_4(__int16, h->GetDefaultFunc(), town, build_id, resources, costs);
	Town_AffectBuildingCost(town, costs);

	return result;
}

int Ai_buildingCostArray[7] = {};
int* __stdcall AITown_GetBuildingCost(HiHook* h, H3Town* town, int* build_id)
{
	int* result = THISCALL_2(int*, h->GetDefaultFunc(), town, build_id);
	memcpy(Ai_buildingCostArray, result, 28);
	Town_AffectBuildingCost(town, Ai_buildingCostArray);

	return Ai_buildingCostArray;
}



void BuildingProcedure::SetPatches(PatcherInstance* _PI)
{

	if (artifactsData.buildingCostChanger.size())
	{
		_PI->WriteHiHook(0x5C14F0, THISCALL_, Town_GetBuildingCost);
		_PI->WriteHiHook(0x5C1580, THISCALL_, Town_GetDisplayedBuildingCost);
		_PI->WriteHiHook(0x5C1480, THISCALL_, AITown_GetBuildingCost);
		_PI->WriteLoHook(0x5BF865, AITown_BuildingBuilt);
	}
}
