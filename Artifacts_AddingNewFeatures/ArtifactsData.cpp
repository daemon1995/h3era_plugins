#include "pch.h"
#include "sstream"

constexpr UINT MAX_SKILL_LEVEL = eSecSkillLevel::EXPERT;

int ArtifactsData::LoadJsonData()
{


	bool readSuccess = false;
	double tempDouble = 1.f;
	int readResult = 0;

	Era::ConnectEra();

	Clear();

	std::string str;
	for (size_t artId = 0; artId < 1024; artId++)
	{
		std::string lands = EraJS::read(H3String::Format("era.artifacts.%d.pathfinding.ignorePenaltyLands", artId).String(), readSuccess);
		if (readSuccess)
		{
			uint64_t affectedLandBits = 0;

			std::stringstream ss(lands);
			int num;

			while (ss >> num)
			{
				if (num > 0 && num < 64)
					affectedLandBits |= (1 << num);
				else if (num == -1)
				{
					affectedLandBits = -1;
					break;
				}
				if (ss.peek() == ',')
					ss.ignore();
			}
			if (affectedLandBits)
				pathFindingData.emplace(artId, affectedLandBits);
		}


		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.pathfinding.landStepDiscount", artId).String(), readSuccess);
		if (readSuccess && readResult)
			landMultiplierData.emplace(artId, readResult);

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.pathfinding.waterStepDiscount", artId).String(), readSuccess);
		if (readSuccess && readResult)
			waterMultiplierData.emplace(artId, readResult);

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.buildingCostDiscount", artId).String(), readSuccess);
		if (readSuccess && readResult)
			buildingCostChanger.emplace(artId, readResult);

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.movepoints.land", artId).String(), readSuccess);
		if (readSuccess && readResult)
			landMovepointsBonus.emplace(artId, readResult);

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.movepoints.water", artId).String(), readSuccess);
		if (readSuccess && readResult)
			waterMovepointsBonus.emplace(artId, readResult);

		UINT16 freeUse = false;

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.spells.freeUse", artId).String(), readSuccess);
		if (readSuccess && readResult)
			freeUse = true;

		for (INT16 schoolLevel = MAX_SKILL_LEVEL; schoolLevel > -1; --schoolLevel)
		{

			std::string schools = EraJS::read(H3String::Format("era.artifacts.%d.spells.give.%d", artId, schoolLevel).String(), readSuccess);

			if (readSuccess)
			{
				str = schools;

				std::stringstream ss(schools);
				UINT16 spellId;
				//	
				while (ss >> spellId)
				{


					if (spellId >= 0 && spellId < 70)
					{
						spellsAddedByArtifactsId.insert(std::make_pair(artId, spellId));

						UINT multimapKey = (static_cast<UINT>(schoolLevel) << 16) | spellId;
						artifactsBySpellSchoolBitSet.insert(std::make_pair(multimapKey, artId));

						// enable spells for artifact
						P_ArtifactSetup[artId].newSpell = eSpell::SCUTTLE_BOAT;
						if (freeUse)
							artifactsWhichMakeThatSpellFree.insert(std::make_pair(spellId, artId));


					}
					if (ss.peek() == ',')
						ss.ignore();
				}
			}
		}

		std::string levels = EraJS::read(H3String::Format("era.artifacts.%d.spells.block.levels", artId).String(), readSuccess);

		if (readSuccess)
		{

			str = levels;

			std::stringstream ss(str);
			UINT16 spellLevel;
			//	
			while (ss >> spellLevel)
			{
				if (spellLevel >= 0 && spellLevel < 6)
					artifactsWhichBanSpellLevel[spellLevel].emplace_back(artId);
				if (ss.peek() == ',')
					ss.ignore();
			}
		}

		std::string spells = EraJS::read(H3String::Format("era.artifacts.%d.spells.block.ids", artId).String(), readSuccess);

		if (readSuccess)
		{
			str = spells;

			std::stringstream ss(str);
			UINT16 spellId;
			//	
			while (ss >> spellId)
			{
				if (spellId >= 0)
					artifactsWhichBanSpell.insert(std::make_pair(spellId, artId));
				if (ss.peek() == ',')
					ss.ignore();
			}
		}
		int arrC = 0;
		do
		{
			UINT autoSpellId = EraJS::readInt(H3String::Format("era.artifacts.%d.combatAutoCast.%d.spellID", artId, arrC).String(), readSuccess);
			bool addAutoSpell = readSuccess;
			if (readSuccess)
			{
				UINT skillLevel = EraJS::readInt(H3String::Format("era.artifacts.%d.combatAutoCast.%d.skillLevel", artId, arrC).String(), readSuccess);
				if (skillLevel > MAX_SKILL_LEVEL || !readSuccess)
					skillLevel = MAX_SKILL_LEVEL;
				INT power = EraJS::readInt(H3String::Format("era.artifacts.%d.combatAutoCast.%d.power", artId, arrC).String(), readSuccess);
				if (!readSuccess)
					power = -1;

				bool affectOnlyEnemy = EraJS::readInt(H3String::Format("era.artifacts.%d.combatAutoCast.%d.affectOnlyEnemy", artId, arrC).String(), readSuccess);
				if (!readSuccess)
					affectOnlyEnemy = false;

				autoCastedSpellsByArtifactId.insert(std::make_pair(artId, AutoCastSpell{ autoSpellId, skillLevel,power,affectOnlyEnemy }));
			}
			arrC++;

		} while (readSuccess);

		tempDouble = EraJS::readFloat(H3String::Format("era.artifacts.%d.spells.scaling.damage", artId).String(), readSuccess);
		if (readSuccess && tempDouble !=1)
			artifactsWhichScaleDamage.insert(std::make_pair(artId, tempDouble));

		tempDouble = EraJS::readFloat(H3String::Format("era.artifacts.%d.spells.scaling.resurrection", artId).String(), readSuccess);
		if (readSuccess && tempDouble != 1)
			artifactsWhichScaleResurrection.insert(std::make_pair(artId, tempDouble));

		tempDouble = EraJS::readFloat(H3String::Format("era.artifacts.%d.spells.scaling.cure", artId).String(), readSuccess);
		if (readSuccess && tempDouble != 1)
			artifactsWhichScaleCure.insert(std::make_pair(artId, tempDouble));


		int goldAdded = EraJS::readInt(H3String::Format("era.artifacts.%d.income", artId).String(), readSuccess);
		if (readSuccess && goldAdded)
			artifactsWhichAddGold.insert(std::make_pair(artId, goldAdded));
		

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.combat.additionalShot", artId).String(), readSuccess);
		if (readSuccess && readResult)
			artifactsThatGiveAdditionalShot.emplace_back(artId);
		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.combat.fullTentHeal", artId).String(), readSuccess);
		if (readSuccess && readResult)
			artifactsThatGiveFullTentHeal.emplace_back(artId);

		readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.combat.additionalHeal", artId).String(), readSuccess);
		if (readSuccess && readResult)
			artifactsWhichIncreaseTentHealing.insert(std::make_pair(artId, readResult));


	}
	if (!spellsAddedByArtifactsId.empty())
	{
		UINT16 freeUse = false;

		for (auto& pair : spellsAddedByArtifactsId)
		{
			UINT artId = pair.first;
			readResult = EraJS::readInt(H3String::Format("era.artifacts.%d.spells.freeUse", artId).String(), readSuccess);
			if (readSuccess && readResult)
				artifactsWhichMakeThatSpellFree.insert(std::make_pair(pair.second, artId));
		}

	}
	//artifactsWhichBanSpell.insert(std::make_pair(eSpell::ARMAGEDDON, eArtifact::ANGEL_WINGS));
	//artifactsWhichBanSpell.insert(std::make_pair(eSpell::SLOW, eArtifact::ANGEL_WINGS));
	//artifactsWhichBanSpell.insert(std::make_pair(eSpell::AIR_ELEMENTAL, eArtifact::ANGEL_WINGS));

	return false;
}

void ArtifactsData::Clear()
{
	pathFindingData.clear();
	landMultiplierData.clear();
	waterMultiplierData.clear();
	buildingCostChanger.clear();

	waterMovepointsBonus.clear();
	landMovepointsBonus.clear();

	artifactsBySpellSchoolBitSet.clear();
	spellsAddedByArtifactsId.clear();

	artifactsWhichMakeThatSpellFree.clear();

	artifactsWhichBanSpell.clear();

	for (size_t i = 0; i < 6; i++)
		artifactsWhichBanSpellLevel[i].clear();

	artifactsThatGiveAdditionalShot.clear();
	artifactsThatGiveFullTentHeal.clear();
	artifactsWhichIncreaseTentHealing.clear();

	autoCastedSpellsByArtifactId.clear();

	artifactsWhichScaleDamage.clear();
	artifactsWhichScaleResurrection.clear();
	artifactsWhichScaleCure.clear();
	artifactsWhichDecreaseResistance.clear();

	artifactsWhichAddGold.clear();


}


int PluginText::LoadText()
{
	buffer.spellIsForbiddenBy = EraJS::read("era.artifacts.text.spellIsForbiddenBy");
	buffer.spellLevelForbiddenBy = EraJS::read("era.artifacts.text.spellLevelForbiddenBy");

	return 0;
}
