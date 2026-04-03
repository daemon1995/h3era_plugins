#include "CreatureBanksExtender.h"

namespace cbanks
{

CreatureBanksExtender *CreatureBanksExtender::instance = nullptr;
CreatureBanksExtender::Current CreatureBanksExtender::currentCreatureBank = CreatureBanksExtender::Current{};

static BOOL GetArmyMessage(const H3CreatureBank *creatureBank, H3String &customDescription,
                           const bool withoutBrackets = true) noexcept
{
    if (creatureBank)
    {
        FASTCALL_3(H3String *, 0x040AB40, &customDescription, &creatureBank->guardians, withoutBrackets);
    }

    return customDescription.Length();
}
static BOOL ShowMultiplePicsArmyMessage(const char *message, const int messageType, const int x, const int y,
                                        H3Army *army) noexcept
{

    if (army->HasCreatures())
    {
        H3String hexPach;
        switch (messageType)
        {
        case 2: // yes/no
            hexPach = "EB 0B 90909090";
            break;
        case 4: // rmc (no buttons)
            hexPach = "E9 C2040000";
            break;
        default:
            break;
        }
        if (hexPach.Empty())
        {
            return false;
        }

        H3Army armyCopy;
        libc::memcpy(&armyCopy, army, sizeof(armyCopy));

        H3Vector<std::array<int, 2>> armyPictures;

        struct CreatureName
        {
            int creature = eCreature::UNDEFINED;
            LPCSTR oldNamePtr = 0;
            H3String nameWithNumber;
        } creatureNames[7];

        for (size_t i = 0; i < 7; ++i)
        {
            const int monType = armyCopy.type[i];
            if (monType != eCreature::UNDEFINED)
            {
                int creatureCount = armyCopy.count[i];
                for (size_t j = i + 1; j < 7; ++j)
                {
                    if (armyCopy.type[j] == monType)
                    {
                        creatureCount += armyCopy.count[j];
                        armyCopy.type[j] = eCreature::UNDEFINED;
                    }
                }
                if (creatureCount)
                {
                    std::array<int, 2> arr = {ePictureCategories::CREATURE, monType};
                    armyPictures.Push(arr);
                }
                if (monType != eCreature::UNDEFINED)
                {
                    creatureNames[i].creature = monType;
                    LPCSTR armyGroupName = H3Creature::GroupName(creatureCount, 1);
                    creatureNames[i].oldNamePtr = P_CreatureInformation[monType].namePlural;
                    creatureNames[i].nameWithNumber =
                        H3String::Format("%s %s", armyGroupName, P_CreatureInformation[monType].namePlural);
                    P_CreatureInformation[monType].namePlural = creatureNames[i].nameWithNumber.String();
                }
            }
        }

        // set proper multiple pic dlg type
        auto patch = _PI->WriteHexPatch(0x04F731D, hexPach.String());
        // display actual dialog with correct creature names and multiple pics
        FASTCALL_5(void, 0x004F7D20, message, &armyPictures, x, y, 0);
        patch->Destroy();

        // restore creature names
        for (size_t i = 0; i < 7; ++i)
        {
            const int monType = creatureNames[i].creature;
            if (monType != eCreature::UNDEFINED)
            {
                P_CreatureInformation[monType].namePlural = creatureNames[i].oldNamePtr;
            }
        }
        return true;
    }

    return 0;
}
CreatureBanksExtender::CreatureBanksExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.CreatureBanksExtender.daemon_n"))
{
    this->objectType = eObject::CREATURE_BANK;
    CreatePatches();
}

CreatureBanksExtender &CreatureBanksExtender::Get()
{
    if (!instance)
    {
        instance = new CreatureBanksExtender();
    }
    return *instance;
}

void __stdcall CreatureBanksExtender::OnAfterReloadLanguageData(Era::TEvent *event)
{
    auto &creatureBanksManager = instance->manager;
    const size_t SIZE = creatureBanksManager.m_size;
    for (size_t i = 0; i < SIZE; i++)
    {
        const int objectType = creatureBanksManager.GetCreatureBankObjectType(i);
        const int objectSubtype = objectType == eObject::CREATURE_BANK ? i : 0;

        bool trSuccess = false;
        H3String name = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.name", objectType, objectSubtype).String(), trSuccess);

        if (trSuccess)
        {
            creatureBanksManager.setups[i].name = name;
        }
    }
}
_LHF_(CreatureBanksExtender::AIHero_GetCreatureBankItemWeight)
{
    auto bank = reinterpret_cast<H3CreatureBank *>(c->edi);
    if (!bank)
    {
        return EXEC_DEFAULT;
    }

    const auto customReward = instance->manager.GetCustomCreatureBank(bank);
    if (!customReward)
    {
        return EXEC_DEFAULT;
    }

    int bonusValue = 0;
    auto hero = reinterpret_cast<H3Hero *>(c->ebx);
    if (const UINT mithrilAmount = customReward->mithrilAmount)
    {
        const H3Player *player = THISCALL_1(H3Player *, 0x04E5920, hero);
        bonusValue += static_cast<int>(player->resourceImportance[eResource::GOLD] * mithrilAmount * 1000);
    }
    if (const auto experience = customReward->experience)
    {
        float fExperience = static_cast<float>(experience) * h3functions::GetHeroLearningPower(hero);
        bonusValue += static_cast<int>(fExperience * hero->AI_experienceEffectiveness);
    }

    const int needExpoToNextLvl = h3functions::NeedExpoToNextLevel(hero->level);

    for (size_t i = 0; i < SKILLS_AMOUNT; i++)
    {
        if (const auto skill = customReward->primarySkills[i])
        {
            bonusValue += static_cast<int>(skill * needExpoToNextLvl * hero->AI_experienceEffectiveness);
        }
    }
    // if (const int luck = customReward->luck)
    //{
    //     double tempdouble = FASTCALL_2(double, 0x04355B0, hero->GetLuckBonus(nullptr, 0, 1), luck);
    //     float tempfloat = FASTCALL_2(float, 0x04355B0, hero->GetLuckBonus(nullptr, 0, 1), luck);
    //     __int64 temp__int64 = FASTCALL_2(__int64, 0x04355B0, hero->GetLuckBonus(nullptr, 0, 1),
    //     luck); int tempint = FASTCALL_2(int, 0x04355B0, hero->GetLuckBonus(nullptr, 0, 1), luck);
    //     bonusValue += FASTCALL_2(__int64, 0x04355B0, hero->GetLuckBonus(nullptr, 0, 1), luck);
    // }
    // if (const int morale = customReward->morale)
    //{
    //     float temp = FASTCALL_2(float, 0x0435480, hero->GetMoraleBonus(nullptr, 0, 1), morale);

    //    bonusValue += FASTCALL_2(__int64, 0x0435480, hero->GetMoraleBonus(nullptr, 0, 1), morale);
    //}
    if (hero->WearsArtifact(eArtifact::SPELLBOOK))
    {
        const int maxSpellLevel = hero->secSkill[eSecondary::WISDOM] + 2;
        for (size_t i = 0; i < SPELLS_AMOUNT; i++)
        {
            const eSpell &spellId = customReward->spells[i];

            if (spellId == eSpell::NONE || hero->learnedSpells[spellId] || P_Spell[spellId].level > maxSpellLevel)
            {
                continue;
            }

            bonusValue += h3functions::GetAIHeroSpellValue(hero, spellId);
        }
    }
    if (bonusValue)
        IntAt(c->ebp + 0x8) += bonusValue;

    return EXEC_DEFAULT;
}

_LHF_(CreatureBanksExtender::CrBank_AfterCombatWon)
{

    const auto mapItem = currentCreatureBank.mapItem;
    const int creatureBankType = currentCreatureBank.type;
    auto customBank = instance->manager.GetCustomCreatureBank(mapItem);

    if (creatureBankType == eObject::NO_OBJ || mapItem->creatureBank.taken || customBank == nullptr)
    {
        return EXEC_DEFAULT;
    }
    auto &hero = currentCreatureBank.hero;

    currentCreatureBank.spellsToLearn.fill(eSpell::NONE);
    currentCreatureBank.spellPointsToAdd = 0;
    currentCreatureBank.mithrilToAdd = 0;
    currentCreatureBank.experiencePointsToAdd = 0;

    // if mithril is enabled
    if (DwordAt(0x27F99AC))
    {
        currentCreatureBank.mithrilToAdd = customBank->mithrilAmount;
    }

    currentCreatureBank.experiencePointsToAdd +=
        static_cast<int>(static_cast<float>(customBank->experience) * h3functions::GetHeroLearningPower(hero));
    // add experience points
    THISCALL_4(void, 0x4E3620, hero, currentCreatureBank.experiencePointsToAdd, false, true);

    const UINT heroSpellPoints = hero->spellPoints;
    const UINT maxSpellPoints = IntAt(0x049F99F + 2);
    if (hero->spellPoints < maxSpellPoints)
    {
        currentCreatureBank.spellPointsToAdd = Clamp(0, customBank->spellPoints, maxSpellPoints - heroSpellPoints);
        hero->spellPoints += currentCreatureBank.spellPointsToAdd;
    }

    hero->moraleBonus += customBank->morale;
    hero->luckBonus += customBank->luck;
    if (hero->WearsArtifact(eArtifact::SPELLBOOK))
    {
        const int maxSpellLevel = hero->secSkill[eSecondary::WISDOM] + 2;
        for (size_t i = 0; i < SPELLS_AMOUNT; i++)
        {

            const eSpell &spellId = customBank->spells[i];
            if (spellId == eSpell::NONE || hero->learnedSpells[spellId] || P_Spell[spellId].level > maxSpellLevel)
            {
                continue;
            }
            currentCreatureBank.spellsToLearn[i] = spellId;
            // learn spell
            h3functions::HeroLearnSpell(hero, spellId);
        }
    }
    for (size_t i = 0; i < SKILLS_AMOUNT; i++)
    {
        hero->primarySkill[i] += customBank->primarySkills[i];
    }

    return EXEC_DEFAULT;
}

// works for Human Only
_LHF_(CreatureBanksExtender::CrBank_AfterDrawingResources)
{
    if (c->esi == MITHRIL_ID && currentCreatureBank.mithrilToAdd) // add one more iteration of next resource is mitrhil
    {
        // change  gold picture from money to gold if we have mithril
        if (H3PictureVector *pictureCategories = reinterpret_cast<H3PictureVector *>(c->ebp - 0x54))
        {
            if (!pictureCategories->IsEmpty())
            {
                H3PictureCategories *lastCategory = pictureCategories->Last();
                if (lastCategory && lastCategory->type == ePictureCategories::MONEY)
                {
                    lastCategory->type = ePictureCategories::GOLD;
                }
            }
        }

        c->eax = currentCreatureBank.mithrilToAdd;
        // return to one more iteration
        c->return_address = 0x04ABDA6;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

_LHF_(CreatureBanksExtender::CrBank_BeforeShowingRewardMessage)
{

    BOOL returnType = EXEC_DEFAULT;
    // skip strings vector parsing if it is empty to avoid crash
    if (H3Vector<H3String> *strings = reinterpret_cast<H3Vector<H3String> *>(c->ebp - 0x44))
    {
        if (strings->IsEmpty())
        {
            c->esp -= 12;
            c->return_address = 0x04ABE51;
            returnType = NO_EXEC_DEFAULT;
        }
    }

    const auto mapItem = currentCreatureBank.mapItem;
    auto customCreatureBank = instance->manager.GetCustomCreatureBank(mapItem);

    if (mapItem->creatureBank.taken || customCreatureBank == nullptr)
    {
        return returnType;
    }

    H3PictureVector *pictureCategories = reinterpret_cast<H3PictureVector *>(c->ebp - 0x54);

    auto &message = currentCreatureBank.message;
    bool experienceAdded = false;
    bool primarySkillAdded = false;
    if (const UINT experience = currentCreatureBank.experiencePointsToAdd)
    {
        H3PictureCategories pair = H3PictureCategories::Experience(experience);
        pictureCategories->Add(pair);
        experienceAdded = true;
    }

    for (size_t i = 0; i < SKILLS_AMOUNT; i++)
    {
        if (const UINT skillNum = customCreatureBank->primarySkills[i])
        {
            H3PictureCategories pair = H3PictureCategories::PrimarySkill(ePrimary(i), skillNum);
            pictureCategories->Add(pair);
            primarySkillAdded = true;
        }
    }

    auto &hero = currentCreatureBank.hero;

    if (experienceAdded || primarySkillAdded)
    {
        libc::sprintf(h3_TextBuffer, P_AdveventText->GetText(176), hero->name);
        message.Append(' ').Append(h3_TextBuffer);
    }

    if (const UINT spellPoints = currentCreatureBank.spellPointsToAdd)
    {
        H3PictureCategories pair = H3PictureCategories::SpellPoints(spellPoints);
        pictureCategories->Add(pair);

        libc::sprintf(h3_TextBuffer, P_AdveventText->GetText(178), hero->name);
        message.Append(' ').Append(h3_TextBuffer);
    }

    if (const INT luck = customCreatureBank->luck)
    {
        H3PictureCategories pair = H3PictureCategories::Luck(luck);
        pictureCategories->Add(pair);

        libc::sprintf(h3_TextBuffer, P_AdveventText->GetText(luck > 0 ? 182 : 181), hero->name);
        message.Append(' ').Append(h3_TextBuffer);
    }

    if (const INT morale = customCreatureBank->morale)
    {
        H3PictureCategories pair = H3PictureCategories::Morale(morale);
        pictureCategories->Add(pair);

        libc::sprintf(h3_TextBuffer, P_AdveventText->GetText(morale > 0 ? 180 : 179), hero->name);
        message.Append(' ').Append(h3_TextBuffer);
    }

    size_t addedSpells = 0;
    for (size_t i = 0; i < SPELLS_AMOUNT; i++)
    {
        const eSpell spellId = currentCreatureBank.spellsToLearn[i];
        if (spellId == eSpell::NONE)
            continue;

        H3PictureCategories pair = H3PictureCategories::Spell(spellId);
        pictureCategories->Add(pair);
        addedSpells++;
    }

    if (addedSpells)
    {
        libc::sprintf(h3_TextBuffer, P_AdveventText->GetText(addedSpells > 1 ? 189 : 185), hero->name);
        message.Append(' ').Append(h3_TextBuffer);
    }

    return returnType;
}

H3String *__cdecl CreatureBanksExtender::CrBank_AwardMessageFormatReadingFromTxt(HiHook *h, char *buffer,
                                                                                 const char *textFormat,
                                                                                 const char *creatureNames,
                                                                                 const char *rewardText) noexcept
{

    bool readSuccess = false;
    const auto mapItem = currentCreatureBank.mapItem;

    char *customVictoryText = EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.text.victory", mapItem->objectType, mapItem->objectSubtype)
            .String(),
        readSuccess);

    if (readSuccess)
    {
        textFormat = customVictoryText;
    }

    H3String finalMessage;
    if (EraJS::readInt("RMG.settings.creatureBanks.displayName") &&
        mapItem->objectType == eObject::CREATURE_BANK) // other CB has name by default
    {
        finalMessage = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));
    }

    if (finalMessage.Length() > 0)
    {
        finalMessage.Append("\n\n").Append(textFormat);
        textFormat = finalMessage.String();
    }

    auto result = CDECL_4(H3String *, h->GetDefaultFunc(), buffer, textFormat, creatureNames, rewardText);

    if (result && !result->Empty() && !currentCreatureBank.message.Empty())
    {
        result->Append('\n').Append(currentCreatureBank.message);
    }

    return result;
}

// works for Human and AI
_LHF_(CreatureBanksExtender::CrBank_BeforeGivingResources)
{
    if (const auto mithrilToAdd = currentCreatureBank.mithrilToAdd)
    {
        h3functions::HeroAddResource(reinterpret_cast<H3Hero *>(c->ecx), MITHRIL_ID, mithrilToAdd);
    }

    return EXEC_DEFAULT;
}

_LHF_(CreatureBanksExtender::CrBank_BeforeSetupFromState)
{

    // init creature banks base data to assign to custom banks when they will be added into main array
    int &type = currentCreatureBank.type = c->ecx / 400;
    int &stateId = currentCreatureBank.stateId = c->edx;

    // check if we already in the game creature banks

    if (auto creatureBank = reinterpret_cast<H3CreatureBank *>(c->ebx))
    {
        auto &manager = instance->manager;
        const auto &customRewardSetups = manager.customRewardSetups;
        if (type < manager.customRewardSetups.size() && stateId < manager.customRewardSetups[type].size())
        {
            const auto &customRewardSetupState = customRewardSetups[type][stateId];

            currentCreatureBank.customDefName = customRewardSetupState.customDefName;

            for (size_t i = 0; i < ARTIFACTS_AMOUNT; i++)
            {
                const eArtifact artifact = customRewardSetupState.artifactIds[i];
                if (artifact != eArtifact::NONE)
                {
                    creatureBank->artifacts.AddOne(artifact);
                }
            }

            // find bank in creatureBanks array
            const auto &game = H3Game::Get();
            const auto &creatureBanks = game->creatureBanks;
            for (size_t i = 0; i < creatureBanks.Size(); i++)
            {
                // if this creature bank already exist
                if (creatureBank == &creatureBanks[i])
                {
                    const auto &adventure = P_AdventureManager->Get();

                    // get current creature bank presetup
                    const auto &currentCustomBank = manager.customCreatureBanksVector[i];
                    const auto &currentStateSetup = manager.customRewardSetups[type][currentCustomBank.stateIndex];

                    // init creature bank graphics
                    if (currentCreatureBank.customDefName != currentStateSetup.customDefName &&
                        currentCreatureBank.customDefName)
                    {
                        const int mapSize = H3MapSize::Get();
                        bool breakLoop[2] = {false, false};
                        for (size_t z = 0; z < game->mainSetup.hasUnderground; z++)
                        {
                            for (size_t y = 0; y < mapSize; y++)
                            {
                                for (size_t x = 0; x < mapSize; x++)
                                {
                                    if (const auto mapItem = adventure->GetMapItem(x, y, z))
                                    {
                                        if (CreatureBanksExtender::GetCreatureBankType(mapItem) == type &&
                                            mapItem->creatureBank.id == i)
                                        {
                                            h3functions::SetMapItemDef(mapItem);
                                            breakLoop[0] = true;
                                            breakLoop[1] = true;
                                            break;
                                        }
                                    }
                                }
                                if (breakLoop[1])
                                    break;
                            }
                            if (breakLoop[0])
                                break;
                        }
                    }

                    // init custom reward
                    // set new creature bank setup - required cause data may be changed and spells too
                    manager.customCreatureBanksVector[i] = CustomCreatureBank(customRewardSetupState, i);

                    stateId = -1;
                    type = -1;
                    break;
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

// place creature bank with script/ plugin /else
int __stdcall CreatureBanksExtender::WoG_PlaceObject(HiHook *h, const int x, const int y, const int z,
                                                     const int objType, const int objSubtype, const int objType2,
                                                     const int objSubtype2, const DWORD a8)
{

    const int sizeBeforeAdding = P_Game->creatureBanks.Size();

    const int result = CDECL_8(int, h->GetDefaultFunc(), x, y, z, objType, objSubtype, objType2, objSubtype2, a8);
    if (sizeBeforeAdding < P_Game->creatureBanks.Size())
    {
        if (currentCreatureBank.customDefName)
        {
            if (const auto mapItem = P_AdventureManager->GetMapItem(x, y, z))
            {
                h3functions::SetMapItemDef(mapItem);
            }
            currentCreatureBank.customDefName = nullptr;
        }
    }

    return result;
}

void __stdcall CreatureBanksExtender::CrBank_AddToGameList(HiHook *h, H3Vector<H3CreatureBank> *creatureBanks,
                                                           const H3CreatureBank *end, const UINT number,
                                                           const H3CreatureBank *creatureBank)
{

    const int sizeBeforeAdding = creatureBanks->Size();

    THISCALL_4(void, h->GetDefaultFunc(), creatureBanks, end, number, creatureBank);

    int &type = currentCreatureBank.type;
    int &stateId = currentCreatureBank.stateId;
    // if we inited data in the CrBank_BeforeSetupFromState
    if (sizeBeforeAdding < creatureBanks->Size() && stateId != -1 && type != -1)
    {
        auto &manager = instance->manager;

        const auto &customRewardSetupState = manager.customRewardSetups[type][stateId];
        manager.customCreatureBanksVector.emplace_back(CustomCreatureBank(customRewardSetupState, sizeBeforeAdding));

        type = -1;
        stateId = -1;
    }
}
_LHF_(CreatureBanksExtender::Game_AfterInitMapItem)
{
    if (currentCreatureBank.customDefName && c->esi)
    {
        h3functions::SetMapItemDef(reinterpret_cast<H3MapItem *>(c->esi));
    }

    return EXEC_DEFAULT;
}
_LHF_(CreatureBanksExtender::Game_SetMapItemDef)
{
    if (auto &customDefName = currentCreatureBank.customDefName)
    {
        libc::sprintf(reinterpret_cast<char *>(c->ebp - 0x84), "%s", customDefName);
        customDefName = nullptr;
    }

    return EXEC_DEFAULT;
}

_LHF_(CreatureBanksExtender::CrBank_DisplayPreCombatMessage)
{
    auto mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC);
    currentCreatureBank.mapItem = mapItem;
    currentCreatureBank.type = GetCreatureBankType(mapItem);
    if (currentCreatureBank.type != eObject::NO_OBJ)
    {
        currentCreatureBank.bank = &P_Game->creatureBanks[mapItem->creatureBank.id];
    }
    return EXEC_DEFAULT;
}

_LHF_(CreatureBanksExtender::SpecialCrBank_DisplayPreCombatMessage)
{
    if (H3MapItem *mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC))
    {
        if (mapItem->creatureBank.taken)
        {
            c->return_address = 0x04AC1E5;
        }
        else
        {
            currentCreatureBank.mapItem = mapItem;

            auto creatureBank = &P_Game->creatureBanks[mapItem->creatureBank.id];
            LPCSTR originalText = *reinterpret_cast<LPCSTR *>(c->ebp + 0x10);
            ShowMultiplePicsArmyMessage(originalText, 2, -1, -1, &creatureBank->guardians);

            c->return_address = 0x04AC1BE;
        }

        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

signed int __stdcall CreatureBanksExtender::CrBank_CombatAndRewardProc(HiHook *h, H3AdventureManager *advMan,
                                                                       const H3Hero *attHero, const H3MapItem *mapItem,
                                                                       LPCSTR text, const UINT PisMixed,
                                                                       const BOOL isPlayer)
{
    currentCreatureBank.mapItem = mapItem;
    currentCreatureBank.type = GetCreatureBankType(mapItem);
    currentCreatureBank.hero = const_cast<H3Hero *>(attHero);
    const int result = THISCALL_6(int, h->GetDefaultFunc(), advMan, attHero, mapItem, text, PisMixed, isPlayer);
    currentCreatureBank = {};

    return result;
}
signed int __stdcall CreatureBanksExtender::CrBank_CombatStart(HiHook *h, H3AdventureManager *advMan, UINT PosMixed,
                                                               const H3Hero *attHero, UINT attArmy, int PlayerIndex,
                                                               UINT defTown, UINT defHero, UINT defArmy, int seed,
                                                               signed int a10, int isBank)
{

    auto &positionsPatch = currentCreatureBank.positionsPatch;
    if (positionsPatch)
    {
        positionsPatch->Destroy();
        positionsPatch = nullptr;
    }

    const int type = currentCreatureBank.type;
    if (type != -1)
    {
        if (isBank = instance->manager.isBankSettings[type])
        {
            positionsPatch = _PI->WriteDword(0x04632A9 + 3, (DWORD)instance->manager.customPositions[type].data());
        }
    }

    const int result = THISCALL_11(signed int, h->GetDefaultFunc(), advMan, PosMixed, attHero, attArmy, PlayerIndex,
                                   defTown, defHero, defArmy, seed, a10, isBank);

    if (positionsPatch)
    {
        positionsPatch->Destroy();
        positionsPatch = nullptr;
    }

    return result;
}

void __stdcall CreatureBanksExtender::CrBank_AskForVisitMessage(HiHook *h, char *mes, const int messageType,
                                                                const int x, const int y, const int picType1,
                                                                const int picSubtype1, const int picType2,
                                                                const int picSubtype2, const int par, const int time,
                                                                const int picType3, const int picSubtype3)
{
    char *resultMessage = mes;
    bool readSuccess = false;

    auto &mapItem = currentCreatureBank.mapItem;
    auto &bank = currentCreatureBank.bank;

    char *customVisitText = EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype).String(),
        readSuccess);

    if (readSuccess)
    {
        resultMessage = customVisitText;
    }

    H3String finalMessage;
    if (EraJS::readInt("RMG.settings.creatureBanks.displayName") &&
        mapItem->objectType == eObject::CREATURE_BANK) // other CB has name by default
    {
        finalMessage = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));
    }

    if (finalMessage.Length())
    {
        finalMessage.Append("\n\n");
    }
    finalMessage.Append(resultMessage);

    H3String armySizeMessage;
    GetArmyMessage(bank, armySizeMessage);

    if (finalMessage.Length() && !armySizeMessage.Empty())
    {
        finalMessage.Append("\n\n");
    }
    finalMessage.Append(armySizeMessage);

    if (EraJS::readInt("RMG.settings.creatureBanks.extendedDlgInformation"))
    {
        ShowMultiplePicsArmyMessage(finalMessage.String(), messageType, x, y, &bank->guardians);
    }
    else
    {
        FASTCALL_12(void, h->GetDefaultFunc(), finalMessage.String(), messageType, x, y, picType1, picSubtype1,
                    picType2, picSubtype2, par, time, picType3, picSubtype3);
    }
}

int CreatureBanksExtender::GetCreatureBankType(const H3MapItem *mapItem) noexcept
{
    return mapItem ? GetCreatureBankType(mapItem->objectType, mapItem->objectSubtype) : eObject::NO_OBJ;
}
int CreatureBanksExtender::GetCreatureBankType(const int objType, const int objSubtype) noexcept
{
    int cbId = -1;
    switch (objType)
    {
    case eObject::CREATURE_BANK:
        if (objSubtype >= 0 && objSubtype < instance->addedBanksNumber + instance->defaultBanksNumber)
        {
            cbId = objSubtype;
        }
        break;
    case eObject::DERELICT_SHIP:
        cbId = eCrBank::DERELICT_SHIP;
        break;
    case eObject::DRAGON_UTOPIA:
        cbId = eCrBank::DRAGON_UTOPIA;
        break;
    case eObject::CRYPT:
        cbId = eCrBank::CRYPT;
        break;
    case eObject::SHIPWRECK:
        cbId = eCrBank::SHIPWRECK;
        break;
    default:
        break;
    }

    return cbId;
}

void CreatureBanksExtender::AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes)
{
    // Get Default Banks Number from H3Vector<H3CreatureBankSetup::Ctor>::Size()
    defaultBanksNumber = ByteAt(0x47A3BA + 0x1);

    const int maxCreatureBankSubtype = maxSubtypes[eObject::CREATURE_BANK];

    // init vector sizes!
    addedBanksNumber = manager.LoadCreatureBanksFromJson(defaultBanksNumber, maxCreatureBankSubtype);

    if (addedBanksNumber)
    { // set new Creature Bank Setups data at native array address
        // IntAt(0x67029C) = (int)instance->creatureBanks.setups.data();
        const DWORD newCbArrayAddress = DWORD(manager.setups.data());
        // set new address for the using CB array ptr

        _pi->WriteDword(0x67029C, newCbArrayAddress);
        //_pi->WriteDword(0x047A4B6 +3, newCbArrayAddress + 4);

        //_pi->WriteByte(0x47A3BA +1, defaultBanksNumber + banksAdded);
        // set new address for the H3CreatureBankSetup init vector to use H3CreatureBankSetup::Get(); properly with
        // new setup
        _pi->WriteDword(0x47A3C1 + 1, newCbArrayAddress);
    }
}

H3RmgObjectGenerator *CreatureBanksExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == eObject::CREATURE_BANK)
    {
        return extender::ObjectExtenderManager::CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

UINT CreatureBanksExtender::Size() const noexcept
{
    return manager.m_size;
}

CreatureBanksExtender::~CreatureBanksExtender()
{
}

void __stdcall CreatureBanksExtender::OnBeforeSaveGame(Era::TEvent *Event)
{
    auto &manager = instance->manager;

    for (auto &i : manager.customCreatureBanksVector)
    {
        i.WriteSaveData();
    }
}
void __stdcall CreatureBanksExtender::OnAfterSaveGame(Era::TEvent *Event)
{
    auto &manager = instance->manager;

    for (auto &i : manager.customCreatureBanksVector)
    {
        i.ClearAssocVariables();
    }
}

void __stdcall CreatureBanksExtender::OnAfterLoadGame(Era::TEvent *Event)
{
    auto &manager = instance->manager;
    manager.customCreatureBanksVector.clear();
    const size_t length = P_Game->creatureBanks.Size();
    for (size_t i = 0; i < length; i++)
    {
        manager.customCreatureBanksVector.emplace_back(CustomCreatureBank(i)); // CustomCreatureBank)
    }
}
void __stdcall CreatureBanksExtender::OnGameLeave(Era::TEvent *Event)
{
    instance->manager.customCreatureBanksVector.clear();

    if (currentCreatureBank.positionsPatch)
    {
        currentCreatureBank.positionsPatch->Destroy();
    }
    // reset current creature bank data
    currentCreatureBank = {};
}

void CreatureBanksExtender::CreatePatches()
{
    //  m_isInited = true;

    if (!m_isInited)
    {
        // creature bank initialization
        _pi->WriteLoHook(0x047A70D, CrBank_BeforeSetupFromState);
        // ObjectPlacement by WoG
        _pi->WriteHiHook(0x07133E2, CDECL_, WoG_PlaceObject);

        // adding creature bank into array
        _pi->WriteHiHook(0x04D22B0, THISCALL_, CrBank_AddToGameList);
        _pi->WriteLoHook(0x04C1E32, Game_AfterInitMapItem);
        _pi->WriteLoHook(0x04C96D1, Game_SetMapItemDef);

        // new Rewards AI weight
        // _pi->WriteHiHook(0x0528520, FASTCALL_, AIHero_GetMapItemWeight); // any item
        _pi->WriteLoHook(0x0528B8D, AIHero_GetCreatureBankItemWeight); // bank/utopia/derelict ship
        // chaging creature bank text patches
        {
            // Pre-combat message
            {
                _pi->WriteLoHook(0x04A1394, CrBank_DisplayPreCombatMessage);        // 16 object type
                _pi->WriteLoHook(0x04A1E29, CrBank_DisplayPreCombatMessage);        // 25 object type
                _pi->WriteLoHook(0x04AC19D, SpecialCrBank_DisplayPreCombatMessage); // 24 / 84 / 85 object types

                _pi->WriteHiHook(0x04A13E6, FASTCALL_, CrBank_AskForVisitMessage); // 16 object type
                _pi->WriteHiHook(0x04A1E56, FASTCALL_, CrBank_AskForVisitMessage); // 25 object type
            }
            // Creature bank description for getting text
            {
                _pi->WriteDword(0x0413E23 + 1, 0x06603B0);     // ["\n\0" -> "\n\n\0" for getCrBank text]
                _pi->WriteHexPatch(0x040ABDE, "EB0C90909090"); // remove extra space in the guard description start
            }
        }

        _pi->WriteHiHook(0x04ABAB0, THISCALL_, CrBank_CombatAndRewardProc);
        _pi->WriteHiHook(0x04ABBCB, THISCALL_, CrBank_CombatStart);

        // giving custom rewards after combat won
        {
            // Adding custom rewards to winner (except mithril)
            _pi->WriteLoHook(0x04ABBFF, CrBank_AfterCombatWon);

            // only for Human Player
            {
                // adding mitrhil picture with text by game's function
                _pi->WriteLoHook(0x04ABE3C, CrBank_AfterDrawingResources);
                // adding custom rewards pictures into main H3PictureVector
                // creating custom rewards text taken from Pandora's Box award
                _pi->WriteLoHook(0x04ABE4C, CrBank_BeforeShowingRewardMessage);
                // adding custom text to original reward message (appending it to the end)
                _pi->WriteHiHook(0x04ABFBA, CDECL_, CrBank_AwardMessageFormatReadingFromTxt);
            }

            // giving mithril
            _pi->WriteLoHook(0x04AC13B, CrBank_BeforeGivingResources);
        }

        //  _pi->WriteHexPatch(0x040AC7D, "EB0C90909090"); // remove extra space in the guard description between
        //  creatures

        // saving data in the save game at ERA' events with assocs int variables
        {
            Era::RegisterHandler(OnBeforeSaveGame, "OnBeforeSaveGame");
            Era::RegisterHandler(OnAfterSaveGame, "OnAfterSaveGame");
            Era::RegisterHandler(OnAfterLoadGame, "OnAfterLoadGame");
            Era::RegisterHandler(OnGameLeave, "OnGameLeave");
            //_REH_(OnBeforeSaveGame);
            //_REH_(OnAfterSaveGame);
            //_REH_(OnAfterLoadGame);
            //_REH_(OnGameLeave);
        }

        // renaming creature banks
        Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");

        m_isInited = true;
    }
}
} // namespace cbanks
