#pragma once
#include "pch.h"

#include "map"
#include "set"
struct alignas(8) ArtifactsData
{


	int LoadJsonData();
	// first atg is art id, second data
	std::map<UINT, uint64_t> pathFindingData;
	std::map<UINT, int> landMultiplierData;
	std::map<UINT, int> waterMultiplierData;
	std::map<UINT, int> buildingCostChanger;

	std::map<UINT, int> waterMovepointsBonus;
	std::map<UINT, int> landMovepointsBonus;
	// first arg is spellId| schoolLevel, second art id
	std::multimap<UINT, UINT> artifactsBySpellSchoolBitSet;
	// first arg is art id, second spells id
	std::multimap<UINT, UINT> spellsAddedByArtifactsId;
	// first arg is spell id, second arts id
	std::multimap<UINT, UINT> artifactsWhichMakeThatSpellFree;

	std::multimap<UINT, UINT> artifactsWhichBanSpell;

	std::vector<UINT> artifactsWhichBanSpellLevel[6];
	
	std::vector<UINT> artifactsThatGiveAdditionalShot;
	std::vector<UINT> artifactsThatGiveFullTentHeal;
	std::map<UINT, int> artifactsWhichIncreaseTentHealing;

	struct AutoCastSpell
	{
		UINT spellID;
		UINT skillLevel;
		// -1 equal to heroe's
		INT power;
		bool affectOnlyEnemy;

	};

	std::multimap<UINT, AutoCastSpell> autoCastedSpellsByArtifactId;

	// first arg artId - second - given Gold
	std::map<UINT, INT> artifactsWhichAddGold;

	std::map<UINT, double> artifactsWhichScaleDamage;
	std::map<UINT, double> artifactsWhichScaleResurrection;
	std::map<UINT, double> artifactsWhichScaleCure;

	std::map<UINT, double> artifactsWhichDecreaseResistance;


	const UINT MAX_SKILL_LEVEL = eSecSkillLevel::EXPERT;

	void Clear();

};

struct alignas(sizeof(H3String)) PluginText
{

	struct Buffer
	{
		const char* spellIsForbiddenBy;
		const char* spellLevelForbiddenBy;


	} buffer;
	int LoadText();
};


