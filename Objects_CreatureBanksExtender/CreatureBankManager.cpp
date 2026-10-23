#include "CreatureBankManager.h"
#include <algorithm>

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
        field[i] = Era::GetAssocVarIntValue(h3_TextBuffer);                                                            \
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

static int ValidateSpellId(const int spellId, const int spellCount) noexcept
{
    // Invalid IDs disable the reward instead of selecting another spell.
    return spellId >= 0 && spellId < spellCount ? spellId : int(eSpell::NONE);
}

// inits creature custom bank from predefined setup at the map start

INT CustomRewardSetupState::maxArtId = h3::limits::ARTIFACTS - 1;

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

    mithrilAmount = Clamp(
        0, ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.resources.%d", creatureBankType, stateId, MITHRIL_ID),
        INT32_MAX);

    experience =
        Clamp(0, ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.experience", creatureBankType, stateId), INT32_MAX);
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
    revealRadius = Clamp(0, ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.revealRadius", creatureBankType, stateId),
                         INT32_MAX);

    for (size_t i = 0; i < ARTIFACTS_AMOUNT; i++)
    {
        const int artId = EraJS::readInt(
            H3String::Format("RMG.objectGeneration.16.%d.states.%d.artifactIds.%d", creatureBankType, stateId, i)
                .String(),
            readSuccess);
        if (readSuccess)
        {
            artifactIds[i] = eArtifact(Clamp(eArtifact::NONE, artId, maxArtId));
        }
    }
    for (size_t i = 0; i < SKILLS_AMOUNT; i++)
    {
        primarySkills[i] = Clamp(
            0, ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.skills.primary.%d", creatureBankType, stateId, i),
            UINT8_MAX);
    }

    // Use the same patched spell count as reward generation.
    const int spellCount = ValueAt<unsigned char>(0x4C170D);
    for (size_t i = 0; i < SPELLS_AMOUNT; i++)
    {
        const int spellId = EraJS::readInt(
            H3String::Format("RMG.objectGeneration.16.%d.states.%d.spells.%d.id", creatureBankType, stateId, i)
                .String(),
            readSuccess);
        if (readSuccess)
        {
            spellsRewards[i].spellId = ValidateSpellId(spellId, spellCount);
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
                    spellsRewards[i].spellSchool = eSpellchool(spellSchool & h3::eSpellchool::ALL);
                }
                if (spellLevels)
                {
                    spellsRewards[i].spellLevels = spellLevels & spellsRewards[i].spellLevels;
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
        revealRadius = setupState.revealRadius;
    }
}

void CustomCreatureBank::GenerateSpells(const CustomRewardSetupState &setup)
{

    const unsigned char bannedSpellsArrayOffset = ValueAt<unsigned char>(0x4C16ED);
    const auto *bannedInfo = reinterpret_cast<const unsigned char *>(P_Game->Get()) + bannedSpellsArrayOffset;
    const unsigned char bannedInfoSize = ValueAt<unsigned char>(0x4C170D);

    spells.fill(eSpell::NONE);
    size_t spellsCount = 0;
    // The byte-sized count cannot exceed 255; reuse a stack buffer for all reward slots.
    std::array<int, UINT8_MAX> spellsToSelect;
    for (size_t i = 0; i < SPELLS_AMOUNT; i++)
    {
        const auto &data = setup.spellsRewards[i];

        const int spell = data.spellId;
        if (spell != eSpell::NONE)
        {
            if (std::find(spells.begin(), spells.begin() + spellsCount, spell) == spells.begin() + spellsCount)
                spells[spellsCount++] = spell;
        }
        else if (data.generate)
        {
            size_t goodSpellsAmount = 0;
            for (int spellId = 0; spellId < bannedInfoSize; ++spellId)
            {
                const auto &spell = P_Spell[spellId];
                // check if spell isn't added and isn't banned and has required flags
                if (!bannedInfo[spellId] && (spell.flags & data.spellFlags) &&
                    (data.spellLevels & (1u << spell.level)) && (data.spellSchool & spell.school) &&
                    std::find(spells.begin(), spells.begin() + spellsCount, spellId) == spells.begin() + spellsCount)
                {
                    spellsToSelect[goodSpellsAmount++] = spellId;
                }
            }

            // Keep one random draw per generated reward, including a single available candidate.
            if (goodSpellsAmount)
            {
                spells[spellsCount++] = spellsToSelect[H3Random::RandBetween(0, goodSpellsAmount - 1)];
            }
        }
    }
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
    SAVE_SIMPLE_FIELD(revealRadius);

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
    CLEAR_SIMPLE_FIELD(revealRadius);

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
    LOAD_SIMPLE_FIELD(revealRadius)

    LOAD_INDEXED_FIELD(primarySkills, SKILLS_AMOUNT)
    LOAD_INDEXED_FIELD(spells, SPELLS_AMOUNT)

    for (auto &skill : primarySkills)
        skill = Clamp(0, static_cast<int>(skill), UINT8_MAX);

    const int spellCount = ValueAt<unsigned char>(0x4C170D);
    for (auto &spell : spells)
        spell = ValidateSpellId(spell, spellCount);
}

void CreatureBankManager::Resize(const size_t size) noexcept
{
    monsterAwards.resize(size);
    monsterGuards.resize(size);
    setups.resize(size);
    isBankSettings.resize(size);
    customPositions.resize(size);
    customRewardSetups.resize(size);

    m_size = size;
}

int CreatureBankManager::LoadCreatureBanksFromJson(const INT16 defaultBanksNumber, const INT16 maxSubtype)
{

    int addedBanksNumber = 0;

    const int maxMonId = IntAt(0x4A1657) - 1;
    CustomRewardSetupState::maxArtId = IntAt(0x49DD8E + 2) / 4 - 1;

    // init  default positions
    std::array<int, 14> defaultPositions{};
    constexpr int positionsAddress = 0x063D0E0;
    libc::memcpy(defaultPositions.data(), reinterpret_cast<int *>(positionsAddress), sizeof(defaultPositions));

    const int banksAmount = maxSubtype + 1;
    Resize(banksAmount);

    CopyDefaultData(defaultBanksNumber, defaultPositions);

    bool trSuccess = false;

    //  allocate space for new creature banks
    for (int cbId = 0; cbId < banksAmount; cbId++)
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

        // Read each reward state once, directly into the manager's settings array.
        for (size_t i = 0; i < STATES_AMOUNT; ++i)
        {
            customRewardSetups[cbId][i] = CustomRewardSetupState(cbId, i);
        }

        // placement troops in combat

        const bool isBank = EraJS::readInt(
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

        isBankSettings[cbId] = isBankSetting;
        customPositions[cbId] = positions;

        if (cbId >= defaultBanksNumber)
        {
            libc::memcpy(monsterGuards[cbId].data(), &setup.states[0].guardians.type[0], sizeof(int) * GUARDES_AMOUNT);
            // increase number of added banks
            addedBanksNumber++;
        }
        monsterAwards[cbId] = setup.states[0].creatureRewardType;
    }
    // The game keeps these addresses: do not resize or shrink the arrays after publishing them.
    if (addedBanksNumber > 0)
    {
        ValueAt<int *>(0x47A4A8 + 3) = monsterAwards.data();
        ValueAt<int *>(0x47A4AF + 3) = monsterGuards[0].data();
    }

    return addedBanksNumber;
}

const CustomCreatureBank *CreatureBankManager::GetCustomCreatureBank(const H3MapItem *mapItem) const noexcept
{
    return !mapItem || mapItem->creatureBank.id >= customCreatureBanksVector.size()
               ? nullptr
               : &customCreatureBanksVector[mapItem->creatureBank.id];
}

const CustomCreatureBank *CreatureBankManager::GetCustomCreatureBank(const H3CreatureBank *creatureBank) const noexcept
{
    if (!creatureBank)
        return nullptr;

    // GetIndexOf only calculates an index; the custom array may not be populated yet.
    const UINT creatureBankId = P_Game->creatureBanks.GetIndexOf(*creatureBank);
    return creatureBankId >= customCreatureBanksVector.size() ? nullptr : &customCreatureBanksVector[creatureBankId];
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
            // clear original data
            originalBanks[i].name.Dereference();
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
