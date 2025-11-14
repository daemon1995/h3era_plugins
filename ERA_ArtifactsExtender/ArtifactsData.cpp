#include "pch.h"
#include "sstream"

inline float ReadArtFloat(LPCSTR format, const UINT artID)
{
    return EraJS::readFloat(H3String::Format(format, artID).String());
}

inline int ReadArtInt(LPCSTR format, const UINT artID)
{
    return EraJS::readInt(H3String::Format(format, artID).String());
}
inline int ReadArtInt(LPCSTR format, const UINT artID, const int parameter)
{
    return EraJS::readInt(H3String::Format(format, artID, parameter).String());
}
int ArtifactsData::LoadJsonData()
{

    bool readSuccess = false;
    double tempDouble = 1.f;
    int readResult = 0;

    std::string str;
    for (size_t artId = 0; artId < MAX_ARTIFACTS_AMOUNT; artId++)
    {
        auto &_artFeatures = artFeatures[artId];

        // General income data
        if (const int goldAdded = ReadArtInt("era.artifacts.%d.income", artId))
            artifactsWhichAddGold.emplace_back(ArtSimpleBonus{artId, goldAdded});

        std::string lands = EraJS::read(
            H3String::Format("era.artifacts.%d.pathfinding.ignorePenaltyLands", artId).String(), readSuccess);
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
            _artFeatures.ignorePenaltyLands = affectedLandBits;
        }

        _artFeatures.landStepDiscount = ReadArtInt("era.artifacts.%d.pathfinding.landStepDiscount", artId);
        _artFeatures.waterStepDiscount = ReadArtInt("era.artifacts.%d.pathfinding.waterStepDiscount", artId);

        _artFeatures.landMovepointsBonus = ReadArtInt("era.artifacts.%d.movepoints.land", artId);
        _artFeatures.waterMovepointsBonus = ReadArtInt("era.artifacts.%d.movepoints.water", artId);

        _artFeatures.buildingCostChanger = ReadArtInt("era.artifacts.%d.buildingCostDiscount", artId);

        const UINT16 freeUse = ReadArtInt("era.artifacts.%d.spells.give.freeUse", artId);

        if (ReadArtInt("era.artifacts.%d.spells.maximumExpertize", artId))
            artifactsWhichSetExpertMagiclevel.emplace_back(artId);

        std::set<UINT16> spellsSet;
        for (INT16 schoolLevel = eSecSkillLevel::EXPERT; schoolLevel > -1; --schoolLevel)
        {

            std::string schools = EraJS::read(
                H3String::Format("era.artifacts.%d.spells.give.%d", artId, schoolLevel).String(), readSuccess);

            if (readSuccess)
            {
                str = schools;

                std::stringstream ss(schools);
                UINT16 spellId;

                while (ss >> spellId)
                {

                    if (spellId >= 0 && spellId < 70)
                    {
                        artFeatures[artId].spellsData[spellId].addsSpellWithLevel[schoolLevel] = true;
                        P_ArtifactSetup[artId].newSpell = eSpell::SCUTTLE_BOAT;
                        artFeatures[artId].spellsData[spellId].free = freeUse;
                        spellsSet.insert(spellId);
                    }
                    if (ss.peek() == ',')
                        ss.ignore();
                }
            }
        }
        if (spellsSet.size())
        {
            _artFeatures.addedSpells.assign(spellsSet.begin(), spellsSet.end());
            _artFeatures.hasSpells = true;
        }

        constexpr const char *keyNames[2] = {"block", "immunities"};
        BOOL *containers[2] = {_artFeatures.levelsBanned, _artFeatures.levelImmunities};
        for (size_t i = 0; i < 2; ++i)
        {
            std::string levels = EraJS::read(
                H3String::Format("era.artifacts.%d.spells.%s.levels", artId, keyNames[i]).String(), readSuccess);

            if (readSuccess)
            {

                std::stringstream ss(levels);
                UINT16 spellLevel;
                //
                while (ss >> spellLevel)
                {
                    if (spellLevel >= 0 && spellLevel < MAX_SPELL_LEVELS_AMOUNT)
                        containers[i][spellLevel] = true;
                    if (ss.peek() == ',')
                        ss.ignore();
                }
            }
        }

        std::string spells =
            EraJS::read(H3String::Format("era.artifacts.%d.spells.block.ids", artId).String(), readSuccess);

        if (readSuccess)
        {

            std::stringstream ss(spells);
            UINT16 spellId;
            //
            while (ss >> spellId)
            {
                if (spellId >= 0)
                    _artFeatures.spellsData[spellId].ban =
                        true; // artifactsWhichBanSpell.insert(std::make_pair(spellId, artId));
                if (ss.peek() == ',')
                    ss.ignore();
            }
        }
        int keyIndexCounter = 0;
        do
        {
            UINT autoSpellId = EraJS::readInt(
                H3String::Format("era.artifacts.%d.combatAutoCast.%d.spellID", artId, keyIndexCounter).String(),
                readSuccess);

            if (readSuccess && autoSpellId < limits::TOTAL_SPELLS)
            {

                UINT skillLevel = EraJS::readInt(
                    H3String::Format("era.artifacts.%d.combatAutoCast.%d.skillLevel", artId, keyIndexCounter).String(),
                    readSuccess);
                if (skillLevel > MAX_SKILL_LEVELS_AMOUNT || !readSuccess)
                    skillLevel = MAX_SKILL_LEVELS_AMOUNT;
                INT power = EraJS::readInt(
                    H3String::Format("era.artifacts.%d.combatAutoCast.%d.power", artId, keyIndexCounter).String(),
                    readSuccess);
                if (!readSuccess)
                    power = -1;

                bool affectOnlyEnemy =
                    ReadArtInt("era.artifacts.%d.combatAutoCast.%d.affectOnlyEnemy", artId, keyIndexCounter);

                autoCastedSpellsByArtifactId.emplace_back(
                    ArtAutoCastSpellBonus{artId, autoSpellId, skillLevel, power, affectOnlyEnemy});
            }
            keyIndexCounter++;

        } while (readSuccess);

        _artFeatures.damage = ReadArtFloat("era.artifacts.%d.spells.scaling.damage", artId);
        _artFeatures.resurrection = ReadArtFloat("era.artifacts.%d.spells.scaling.resurrection", artId);
        _artFeatures.cure = ReadArtFloat("era.artifacts.%d.spells.scaling.cure", artId);

        if (ReadArtInt("era.artifacts.%d.combat.additionalShot", artId))
            artifactsThatGiveAdditionalShot.emplace_back(artId);

        if (ReadArtInt("era.artifacts.%d.combat.fullTentHeal", artId))
            artifactsThatGiveFullTentHeal.emplace_back(artId);

        readResult = ReadArtInt("era.artifacts.%d.combat.additionalHeal", artId);
        if (readResult)
            artifactsWhichIncreaseTentHealing.emplace_back(ArtSimpleBonus{artId, readResult});

        _artFeatures.spellCostChanger += ReadArtInt("era.artifacts.%d.spells.costChanger", artId);

        // Sec Skills sections
        for (size_t skillId = 0; skillId < h3::limits::SECONDARY_SKILLS; skillId++)
        {
            readResult = ReadArtInt("era.artifacts.%d.skills.%d", artId, skillId);
            if (readResult)
            {
                secondarySkillBonuses[skillId].emplace_back(ArtSimpleBonus{artId, readResult});
            }
        }

        readResult = ReadArtInt("era.artifacts.%d.spells.duration", artId);
        if (readResult)
        {
            artifactsWhichIncreaseSpellsDuration.emplace_back(ArtSimpleBonus{artId, readResult});
        }

        // NPC Section
        readResult = ReadArtInt("era.artifacts.%d.commander.isCommanderArt", artId);
        if (readResult)
        {
            _artFeatures.isCommanderArt = true;
            _artFeatures.npcData = new ArtFeature::NpcData();

            for (size_t i = 0; i < 7; i++)
            {
                _artFeatures.npcData->parameters.asArray[i] =
                    ReadArtInt("era.artifacts.%d.commander.primary.%d", artId, i);
            }
            _artFeatures.npcData->melee.attack = ReadArtInt("era.artifacts.%d.commander.melee.attack", artId);
            _artFeatures.npcData->melee.defence = ReadArtInt("era.artifacts.%d.commander.melee.defence", artId);
            _artFeatures.npcData->range.attack = ReadArtInt("era.artifacts.%d.commander.range.attack", artId);
            _artFeatures.npcData->range.defence = ReadArtInt("era.artifacts.%d.commander.range.defence", artId);
            const int buffSpell = ReadArtInt("era.artifacts.%d.commander.spells.init", artId);

            if (buffSpell > 0 && buffSpell < h3::limits::SPELLS)
            {
                _artFeatures.npcData->buffSpell = eSpell(buffSpell);
            }
        }
    }

    return false;
}

void ArtifactsData::AffectComboArts()
{
    bool artCounted[MAX_ARTIFACTS_AMOUNT]{};

    std::map<int, std::vector<UINT>> comboArts;

    for (size_t i = 0; i < MAX_ARTIFACTS_AMOUNT; i++)
    {
        int combinationArtifactId = P_Artifacts[i].combinationArtifactId;
        if (combinationArtifactId != eArtifact::NONE)
        {
            auto artsVecIt = comboArts.find(combinationArtifactId);
            if (artsVecIt != comboArts.end())
            {
                artsVecIt->second.emplace_back(i);
            }
            else
            {
                comboArts.emplace(combinationArtifactId, std::vector<UINT>{i});
            }
        }
    }

    for (auto &node : comboArts)
    {
        int artId = node.first;
        if (artId > -1)
        {
            //			artFeatures[artId].;
        }
    }
}

void ArtifactsData::Clear()
{

    artifactsWhichSetExpertMagiclevel.clear();

    artifactsThatGiveAdditionalShot.clear();
    artifactsThatGiveFullTentHeal.clear();
    artifactsWhichIncreaseTentHealing.clear();

    autoCastedSpellsByArtifactId.clear();

    artifactsWhichAddGold.clear();
    for (size_t i = 0; i < MAX_ARTIFACTS_AMOUNT; i++)
    {
        if (auto npcData = artFeatures[i].npcData)
        {
            delete npcData;
            npcData = nullptr;
        }
    }
}

int PluginText::LoadText()
{
    buffer.spellIsForbiddenBy = EraJS::read("era.artifacts.text.spellIsForbiddenBy");
    buffer.spellLevelForbiddenBy = EraJS::read("era.artifacts.text.spellLevelForbiddenBy");

    return 0;
}
