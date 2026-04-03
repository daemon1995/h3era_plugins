#include "CreatureBankManager.h"

namespace defines
{
// this function saves single items of the stuct to savegame
#define SAVE_SIMPLE_FIELD(field)                                                                                       \
    libc::sprintf(h3_TextBuffer, field##Format, id);                                                                   \
    Era::SetAssocVarIntValue(h3_TextBuffer, field);

// this function saves array items of the stuct to savegame
#define SAVE_INDEXED_FIELD(field, size)                                                                                \
    for (size_t i = 0; i < size; ++i)                                                                                  \
    {                                                                                                                  \
        libc::sprintf(h3_TextBuffer, field##Format, id, i);                                                            \
        Era::SetAssocVarIntValue(h3_TextBuffer, field[i]);                                                             \
    }

// this function loads single items of the stuct from savegame and clears variable
#define LOAD_SIMPLE_FIELD(field)                                                                                       \
    libc::sprintf(h3_TextBuffer, field##Format, id);                                                                   \
    field = Era::GetAssocVarIntValue(h3_TextBuffer);                                                                   \
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);

// this function loads array items of the stuct from savegame and clears variables
#define LOAD_INDEXED_FIELD(field, size)                                                                                \
    for (size_t i = 0; i < size; ++i)                                                                                  \
    {                                                                                                                  \
        libc::sprintf(h3_TextBuffer, field##Format, id, i);                                                            \
        field[i] = eSpell(Era::GetAssocVarIntValue(h3_TextBuffer));                                                    \
        Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                                                    \
    }
// this function loads single items of the stuct from savegame and clears variable
#define CLEAR_SIMPLE_FIELD(field)                                                                                      \
    libc::sprintf(h3_TextBuffer, field##Format, id);                                                                   \
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);

// this function only clears variables
#define CLEAR_INDEXED_FIELD(field, size)                                                                               \
    for (size_t i = 0; i < size; ++i)                                                                                  \
    {                                                                                                                  \
        libc::sprintf(h3_TextBuffer, field##Format, id, i);                                                            \
        Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                                                    \
    }
} // namespace defines

inline int ReadJsonInt(LPCSTR format, const int arg, const int arg2)
{
    return EraJS::readInt(H3String::Format(format, arg, arg2).String());
}
inline int ReadJsonInt(LPCSTR format, const int arg, const int arg2, const int arg3)
{
    return EraJS::readInt(H3String::Format(format, arg, arg2, arg3).String());
}

// inits creature custom bank from predefined setup at the map start

INT CustomRewardSetupState::maxArtId = h3::limits::ARTIFACTS;

CustomRewardSetupState::CustomRewardSetupState(const INT creatureBankType, const UINT stateId) noexcept
    : stateId(stateId)
{

    bool readSuccess = false;
    LPCSTR customDefName = EraJS::read(
        H3String::Format("RMG.objectGeneration.16.%d.states.%d.customDef", creatureBankType, stateId).String(),
        readSuccess);

    if (readSuccess && libc::strcmpi(customDefName, h3_NullString))
    {
        this->customDefName = customDefName;
    }

    mithrilAmount =
        ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.resources.%d", creatureBankType, stateId, MITHRIL_ID);

    experience = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.experience", creatureBankType, stateId);
    spellPoints =
        Clamp(0, ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spellPoints", creatureBankType, stateId), 999);

    if (const int _luck = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.luck", creatureBankType, stateId))
    {
        luck = Clamp(-3, _luck, 3);
    }
    if (const int _morale = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.morale", creatureBankType, stateId))
    {
        morale = Clamp(-3, _morale, 3);
    }

    for (size_t i = 0; i < SPELLS_AMOUNT; i++)
    {
        const int artId = EraJS::readInt(
            H3String::Format("RMG.objectGeneration.16.%d.states.%d.artifactIds.%d", creatureBankType, stateId, i)
                .String(),
            readSuccess);
        if (readSuccess)
        {
            artifactIds[i] = eArtifact(Clamp(eArtifact::NONE, artId, maxArtId));
        }
        primarySkills[i] = Clamp(
            0, ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.skills.primary.%d", creatureBankType, stateId, i),
            INT8_MAX);

        const int spellId = EraJS::readInt(
            H3String::Format("RMG.objectGeneration.16.%d.states.%d.spells.%d.id", creatureBankType, stateId, i)
                .String(),
            readSuccess);
        if (readSuccess)
        {
            spellsRewards[i].spellId = eSpell(Clamp(eSpell::NONE, spellId, h3::limits::SPELLS));
        }
        else
        {
            UINT spellSchool = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spells.%d.bits.schools",
                                           creatureBankType, stateId, i);

            UINT spellLevels =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spells.%d.bits.levels", creatureBankType, stateId, i);

            UINT spellFlags =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spells.%d.bits.flags", creatureBankType, stateId, i);
            // if any of data is set then spell may be generated
            if (spellSchool || spellLevels || spellFlags)
            {
                spellsRewards[i].generate = true;
                if (spellSchool)
                {
                    spellsRewards[i].spellSchool = eSpellchool(Clamp(1, spellSchool, h3::eSpellchool::ALL));
                }
                if (spellLevels)
                {
                    spellsRewards[i].spellLevels = Clamp(2, spellLevels, spellsRewards[i].spellLevels);
                }
                if (spellFlags)
                {
                    spellsRewards[i].spellFlags = spellFlags;
                }
            }
        }
    }
}

CustomCreatureBank::CustomCreatureBank(const CustomRewardSetupState &setupState, const UINT creatureBankId)
{
    stateIndex = setupState.stateId;
    id = creatureBankId;
    mithrilAmount = setupState.mithrilAmount;

    if (setupState.enabled)
    {
        GenerateSpells(setupState);

        // init stats
        for (size_t i = 0; i < SKILLS_AMOUNT; i++)
        {
            primarySkills[i] = setupState.primarySkills[i];
        }
        experience = setupState.experience;
        spellPoints = setupState.spellPoints;
        luck = setupState.luck;
        morale = setupState.morale;
    }
}

void CustomCreatureBank::GenerateSpells(const CustomRewardSetupState &setup)
{
    const auto &bannedInfo = P_Game->Get()->disabledSpells;

    // init spells generation
    std::unordered_set<int> spellsSet;
    for (size_t i = 0; i < SPELLS_AMOUNT; i++)
    {
        const auto &data = setup.spellsRewards[i];

        const eSpell spell = data.spellId;
        if (spell != eSpell::NONE)
        {
            spellsSet.insert(spell);
        }
        else if (data.generate)
        {
            std::vector<UINT> spellsToSelect;
            spellsToSelect.reserve(h3::limits::SPELLS);
            INT spellId = 0;
            do
            {
                const auto &spell = P_Spell[spellId];
                // check if spell isn't added and isn't banned and has required flags
                if (spellsSet.find(spellId) == spellsSet.end() && !bannedInfo[spellId] &&
                    spell.flags & data.spellFlags && data.spellLevels & (1 << spell.level) &&
                    data.spellSchool & spell.school)
                {
                    spellsToSelect.emplace_back(spellId);
                }
            } while (++spellId < h3::limits::SPELLS);

            // add spell to set if exist
            if (const size_t goodSpellsAmount = spellsToSelect.size())
            {
                spellsSet.insert(spellsToSelect[H3Random::RandBetween(0, goodSpellsAmount - 1)]);
            }
        }
    }

    volatile int i = 0;
    for (const auto spell : spellsSet)
    {
        spells[i++] = eSpell(spell);
    }
    spellsSet.clear();
}

void CustomCreatureBank::WriteSaveData() const noexcept
{
    // SAVE_SIMPLE_FIELD(id)
    SAVE_SIMPLE_FIELD(stateIndex);
    SAVE_SIMPLE_FIELD(mithrilAmount)
    SAVE_SIMPLE_FIELD(experience);
    SAVE_SIMPLE_FIELD(spellPoints)
    SAVE_SIMPLE_FIELD(luck);
    SAVE_SIMPLE_FIELD(morale);

    SAVE_INDEXED_FIELD(primarySkills, SKILLS_AMOUNT);
    SAVE_INDEXED_FIELD(spells, SPELLS_AMOUNT);
}
void CustomCreatureBank::ClearAssocVariables() const noexcept
{
    // CLEAR_SIMPLE_FIELD(id)
    CLEAR_SIMPLE_FIELD(stateIndex);
    CLEAR_SIMPLE_FIELD(mithrilAmount)
    CLEAR_SIMPLE_FIELD(experience);
    CLEAR_SIMPLE_FIELD(spellPoints)
    CLEAR_SIMPLE_FIELD(luck);
    CLEAR_SIMPLE_FIELD(morale);

    CLEAR_INDEXED_FIELD(primarySkills, SKILLS_AMOUNT);
    CLEAR_INDEXED_FIELD(spells, SPELLS_AMOUNT);
}
// load creature bank from savegame by id and global assoc variables
CustomCreatureBank::CustomCreatureBank(const int creatureBankId)
{

    id = creatureBankId;
    LOAD_SIMPLE_FIELD(stateIndex)
    LOAD_SIMPLE_FIELD(mithrilAmount)
    LOAD_SIMPLE_FIELD(experience)
    LOAD_SIMPLE_FIELD(spellPoints)
    LOAD_SIMPLE_FIELD(luck)
    LOAD_SIMPLE_FIELD(morale)

    LOAD_INDEXED_FIELD(primarySkills, SKILLS_AMOUNT)
    LOAD_INDEXED_FIELD(spells, SPELLS_AMOUNT)
}

void CreatureBankManager::Resize(const size_t size) noexcept
{
    monsterAwards.resize(size);
    monsterGuards.resize(size);
    setups.resize(size);
    isBankSettings.resize(size);
    customPositions.resize(size);
    customRewardSetups.resize(size);

    // init customRewardSetups for default banks properly
    for (size_t creatureBankType = m_size; creatureBankType < size; ++creatureBankType)
    {
        for (size_t i = 0; i < STATES_AMOUNT; ++i)
        {
            customRewardSetups[creatureBankType][i] = CustomRewardSetupState(creatureBankType, i);
        }
    }
    m_size = size;
}

void CreatureBankManager::Reserve(const size_t size) noexcept
{
    monsterAwards.reserve(size);
    monsterGuards.reserve(size);
    setups.reserve(size);
    isBankSettings.reserve(size);
    customPositions.reserve(size);
    customRewardSetups.reserve(size);
}

void CreatureBankManager::ShrinkToFit() noexcept
{
    monsterAwards.shrink_to_fit();
    monsterGuards.shrink_to_fit();
    setups.shrink_to_fit();
    isBankSettings.shrink_to_fit();
    customPositions.shrink_to_fit();
    customRewardSetups.shrink_to_fit();
    m_size = monsterAwards.size();
}

int CreatureBankManager::LoadCreatureBanksFromJson(const INT16 defaultBanksNumber, const INT16 maxSubtype)
{

    int addedBanksNumber = 0;

    const int maxMonId = IntAt(0x4A1657) - 1;
    CustomRewardSetupState::maxArtId = IntAt(0x49DD8E + 2) / 4;

    // init  default positions
    std::array<int, 14> defaultPositions{};
    constexpr int positionsAddress = 0x063D0E0;
    libc::memcpy(defaultPositions.data(), reinterpret_cast<int *>(positionsAddress), sizeof(defaultPositions));

    const int banksAmount = maxSubtype + 1;
    Resize(banksAmount);

    CopyDefaultData(defaultBanksNumber, defaultPositions);

    bool trSuccess = false;
    int tempArr[GUARDES_AMOUNT] = {-1, -1, -1, -1, -1};

    //  allocate space for new creature banks
    for (INT16 cbId = 0; cbId < banksAmount; cbId++)
    {
        const int objectType = GetCreatureBankObjectType(cbId);
        const int objectSubtype = objectType == eObject::CREATURE_BANK ? cbId : 0;

        // states
        H3CreatureBankSetup &setup = setups[cbId]; // : H3CreatureBankSetup{};

        // for new banks added init clean data
        if (cbId >= defaultBanksNumber)
        {
            setup.states[0].chance = 100; // default.chance
            for (size_t i = 0; i < STATES_AMOUNT; i++)
            {
                for (size_t j = 0; j < ARTIFACTS_AMOUNT; j++)
                {
                    setup.states[i].artifactTypeCounts[j] = 0;
                }
                setup.states[i].creatureRewardCount = 0;
                setup.states[i].creatureRewardType = eCreature::UNDEFINED;
            }
        }

        // assign new CB name from json/default
        H3String name = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.name", objectType, objectSubtype).String(), trSuccess);
        if (trSuccess)
        {
            setup.name = name;
        }
        else if (setup.name.Empty())
        {
            setup.name = H3ObjectName::Get()[eObject::CREATURE_BANK];
        }

        // now load data from json
        for (size_t i = 0; i < STATES_AMOUNT; i++)
        {
            auto &state = setup.states[i];

            const int creatureRewardType =
                EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.creatureRewardType", objectType,
                                                objectSubtype, i)
                                   .String(),
                               trSuccess);

            if (trSuccess)
            {
                state.creatureRewardType = Clamp(eCreature::UNDEFINED, creatureRewardType, maxMonId);
            }

            if (state.creatureRewardType != eCreature::UNDEFINED)
            {
                const int creatureRewardCount =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.creatureRewardCount",
                                                    objectType, objectSubtype, i)
                                       .String(),
                                   trSuccess);
                if (trSuccess)
                {
                    state.creatureRewardCount = Clamp(0, creatureRewardCount, 127);
                }
            }

            const int chance = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.%d.%d.states.%d.chance", objectType, objectSubtype, i).String(),
                trSuccess);
            if (trSuccess)
            {
                state.chance = Clamp(0, chance, 100);
            }

            const int upgrade = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.%d.%d.states.%d.upgrade", objectType, objectSubtype, i).String(),
                trSuccess);
            if (trSuccess)
            {
                state.upgrade = Clamp(0, upgrade, 100);
            }

            for (size_t artLvl = 0; artLvl < 4; artLvl++)
            {
                const int artsNum =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.artifactTypeCounts.%d",
                                                    objectType, objectSubtype, i, artLvl)
                                       .String(),
                                   trSuccess);
                if (trSuccess)
                {
                    state.artifactTypeCounts[artLvl] = Clamp(0, artsNum, 127);
                }
            }

            for (size_t j = 0; j < 7; j++)
            {

                const int guardType =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.guardians.type.%d",
                                                    objectType, objectSubtype, i, j)
                                       .String(),
                                   trSuccess);
                if (trSuccess)
                {
                    state.guardians.type[j] = Clamp(eCreature::UNDEFINED, guardType, maxMonId); // creatureRewardType;
                }

                const int guardCount =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.guardians.count.%d",
                                                    objectType, objectSubtype, i, j)
                                       .String(),
                                   trSuccess);
                if (trSuccess)
                {
                    state.guardians.count[j] = Clamp(0, guardCount, INT32_MAX); // creatureRewardType;
                }

                const int resources =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.resources.%d", objectType,
                                                    objectSubtype, i, j)
                                       .String(),
                                   trSuccess);
                if (trSuccess)
                {
                    state.resources.asArray[j] = Clamp(0, resources, INT32_MAX); // creatureRewardType;
                }
            }
        }

        // init custom rewards
        std::array<CustomRewardSetupState, STATES_AMOUNT> customReward{[cbId]() {
            std::array<CustomRewardSetupState, STATES_AMOUNT> arr;
            for (size_t i = 0; i < STATES_AMOUNT; ++i)
            {
                arr[i] = CustomRewardSetupState(cbId, i);
            }
            return arr;
        }()};

        // placement troops in combat

        bool isBank = EraJS::readInt(
            H3String::Format("RMG.objectGeneration.%d.%d.troopPlacement.isBank", objectType, objectSubtype).String(),
            trSuccess);

        const bool isBankSetting = trSuccess ? isBank : true;

        std::array<int, 14> positions = defaultPositions;

        for (size_t i = 0; i < 7; i++)
        {
            if (const int jsonAttackerPosition =
                    ReadJsonInt("RMG.objectGeneration.%d.%d.troopPlacement.attackers.%d", objectType, objectSubtype, i))
            {

                const BOOL atackerPositionIsValid =
                    !(jsonAttackerPosition < 1 || jsonAttackerPosition > 186 || jsonAttackerPosition % 17 == 0 ||
                      jsonAttackerPosition % 17 == 16);

                if (atackerPositionIsValid)
                    positions[i] = jsonAttackerPosition;
            }

            if (const int jsonDefenderPosition =
                    ReadJsonInt("RMG.objectGeneration.%d.%d.troopPlacement.defenders.%d", objectType, objectSubtype, i))
            {
                const BOOL defenderPositionIsValid =
                    !(jsonDefenderPosition < 1 || jsonDefenderPosition > 186 || jsonDefenderPosition % 17 == 0 ||
                      jsonDefenderPosition % 17 == 16);
                if (defenderPositionIsValid)
                    positions[i + 7] = jsonDefenderPosition;
            }
        }

        setups[cbId] = setup;
        customRewardSetups[cbId] = customReward;
        isBankSettings[cbId] = isBankSetting;
        customPositions[cbId] = positions;

        if (cbId >= defaultBanksNumber)
        {
            libc::memcpy(tempArr, &setup.states[0].guardians.type[0], sizeof(tempArr));

            // increase number of added banks
            addedBanksNumber++;
        }
        monsterAwards[cbId] = setup.states[0].creatureRewardType;
    }
    // patch memory to use new arrays
    if (addedBanksNumber > 0)
    {
        ValueAt<int *>(0x47A4A8 + 3) = monsterAwards.data();
        ValueAt<int *>(0x47A4AF + 3) = monsterGuards[0].data();
    }

    ShrinkToFit();

    return addedBanksNumber;
}

const CustomCreatureBank *CreatureBankManager::GetCustomCreatureBank(const H3MapItem *mapItem) const noexcept
{
    return mapItem->creatureBank.id >= customCreatureBanksVector.size()
               ? nullptr
               : &customCreatureBanksVector[mapItem->creatureBank.id];
}

const CustomCreatureBank *CreatureBankManager::GetCustomCreatureBank(const H3CreatureBank *creatureBank) const noexcept
{
    const int creatureBankId = P_Game->creatureBanks.GetIndexOf(*creatureBank);
    return creatureBank ? &customCreatureBanksVector[creatureBankId] : nullptr;
}
void CreatureBankManager::CopyDefaultData(const size_t defaultBanksNumber, const std::array<int, 14> &defaultPositions)
{
    // copy original creature banks
    H3CreatureBankSetup *originalBanks = *reinterpret_cast<H3CreatureBankSetup **>(0x67029C);
    if (originalBanks)
    {
        const int *currentCreatureRewardsArray = ValueAt<int *>(0x47A4A8 + 3);
        memcpy(monsterAwards.data(), currentCreatureRewardsArray, sizeof(int) * defaultBanksNumber);
        const int *currentGuardiansArray = ValueAt<int *>(0x47A4AF + 3);
        memcpy(monsterGuards[0].data(), currentGuardiansArray, sizeof(int) * defaultBanksNumber * 5);
        // iterate default data and copy into current array
        for (size_t i = 0; i < defaultBanksNumber; i++)
        {
            customPositions[i] = defaultPositions;
            setups[i] = originalBanks[i];
            isBankSettings[i] = true;
        }
    }
}

eObject CreatureBankManager::GetCreatureBankObjectType(const int creatureBankId) noexcept
{
    // get CB object type for some edits later
    eObject objectType = eObject::NO_OBJ;
    if (creatureBankId >= 0)
    {
        switch (creatureBankId)
        {

        case eCrBank::SHIPWRECK:
            objectType = eObject::SHIPWRECK;
            break;
        case eCrBank::DERELICT_SHIP:
            objectType = eObject::DERELICT_SHIP;
            break;
        case eCrBank::CRYPT:
            objectType = eObject::CRYPT;
            break;
        case eCrBank::DRAGON_UTOPIA:
            objectType = eObject::DRAGON_UTOPIA;
            break;
        default:
            objectType = eObject::CREATURE_BANK;
            break;
        }
    }

    return objectType;
}
