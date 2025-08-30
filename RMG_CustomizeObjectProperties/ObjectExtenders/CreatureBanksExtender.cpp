#include "..\pch.h"
#include <unordered_set>
namespace cbanks
{
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

CreatureBanksExtender::Current CreatureBanksExtender::currentCreatureBank;

BOOL GetArmyMessage(const H3CreatureBank *creatureBank, H3String &customDescription,
                    const bool withoutBrackets = true) noexcept
{
    if (creatureBank)
    {
        FASTCALL_3(void, 0x040AB40, &customDescription, &creatureBank->guardians, withoutBrackets);
    }

    return customDescription.Length();
}
BOOL ShowMultiplePicsArmyMessage(const char *message, const int messageType, const int x, const int y,
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
        } storedCreatureNames[7];

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
                    storedCreatureNames[i].creature = monType;
                    LPCSTR armyGroupName = H3Creature::GroupName(creatureCount, 1);
                    storedCreatureNames[i].oldNamePtr = P_CreatureInformation[monType].namePlural;
                    storedCreatureNames[i].nameWithNumber =
                        H3String::Format("%s %s", armyGroupName, P_CreatureInformation[monType].namePlural);
                    P_CreatureInformation[monType].namePlural = storedCreatureNames[i].nameWithNumber.String();
                }
            }
        }

        // set proper multiple pic dlg type
        auto patch = _PI->WriteHexPatch(0x04F731D, hexPach.String());
        FASTCALL_5(void, 0x004F7D20, message, &armyPictures, x, y, 0);
        patch->Destroy();

        // restore creature names
        for (size_t i = 0; i < 7; ++i)
        {
            const int monType = storedCreatureNames[i].creature;

            if (monType != eCreature::UNDEFINED)
            {
                P_CreatureInformation[monType].namePlural = storedCreatureNames[i].oldNamePtr;
            }
        }
        return true;
    }

    return 0;
}
CreatureBanksExtender::CreatureBanksExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.CreatureBanksExtender.daemon_n"))
{
    CreatePatches();
}

CreatureBanksExtender *CreatureBanksExtender::instance = nullptr;

CreatureBanksExtender &CreatureBanksExtender::Get()
{
    if (!instance)
        instance = new CreatureBanksExtender();
    return *instance;
}

void __stdcall CreatureBanksExtender::OnAfterReloadLanguageData(Era::TEvent *event)
{
    auto &creatureBanks = Get().manager;
    const size_t SIZE = creatureBanks.m_size;
    const size_t defaultBanksNumber = Get().defaultBanksNumber;
    const size_t lastBankId = defaultBanksNumber + Get().addedBanksNumber;
    for (size_t i = 0; i < SIZE; i++)
    {
        const int objectType = GetCreatureBankObjectType(i);

        const int objectSubtype = objectType == eObject::CREATURE_BANK ? i : 0;
        bool trSuccess = false;

        H3String name = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.name", objectType, objectSubtype).String(), trSuccess);

        if (trSuccess)
        {
            creatureBanks.setups[i].name = name;
        }
    }
}

__int64 __stdcall CreatureBanksExtender::AIHero_GetMapItemWeight(HiHook *h, H3Hero *hero, int *moveDistance,
                                                                 UINT mixedPos)
{

    __int64 result = FASTCALL_3(__int64, h->GetDefaultFunc(), hero, moveDistance, mixedPos);

    if (result && result > IntAt(0x0528B2B + 2))
    {
        if (const auto mapItem = P_AdventureManager->GetMapItem(mixedPos))
        {
            const int creatureBankType = GetCreatureBankType(mapItem);
            if (creatureBankType != eObject::NO_OBJ) // && !mapItem->creatureBank.taken)
            {
                if (const auto customReward = Get().GetCustomCreatureBank(mapItem))
                {
                    int bonusValue = 0;
                    if (const UINT mithrilAmount = customReward->mithrilAmount)
                    {
                        const H3Player *player = THISCALL_1(H3Player *, 0x04E5920, hero);

                        bonusValue +=
                            static_cast<int>(player->resourceImportance[eResource::GOLD] * mithrilAmount * 1000);
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
                            bonusValue +=
                                static_cast<int>(skill * needExpoToNextLvl * hero->AI_experienceEffectiveness);
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

                            if (spellId == eSpell::NONE || hero->learnedSpells[spellId] ||
                                P_Spell[spellId].level > maxSpellLevel)
                            {
                                continue;
                            }

                            bonusValue += h3functions::GetAIHeroSpellValue(hero, spellId);
                        }
                    }

                    result += bonusValue;
                }
            }
        }
    }

    return result;
}
_LHF_(CreatureBanksExtender::CrBank_AfterCombatWon)
{

    if (const auto mapItem = ValueAt<H3MapItem *>(c->ebp + 0xC))
    {

        const int creatureBankType = GetCreatureBankType(mapItem);

        if (creatureBankType != eObject::NO_OBJ && !mapItem->creatureBank.taken &&
            mapItem->creatureBank.id < Get().manager.customCreatureBanksVector.size())
        {

            auto customBank = Get().GetCustomCreatureBank(mapItem);

            currentCreatureBank.spellsToLearn.fill(eSpell::NONE);
            currentCreatureBank.spellPointsToAdd = 0;
            currentCreatureBank.mithrilToAdd = 0;
            currentCreatureBank.experiencePointsToAdd = 0;
            currentCreatureBank.hero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);

            if (customBank)
            {
                // if mithril is enabled
                if (DwordAt(0x27F99AC))
                {
                    currentCreatureBank.mithrilToAdd = customBank->mithrilAmount;
                }

                if (auto &hero = currentCreatureBank.hero)
                {

                    currentCreatureBank.experiencePointsToAdd += static_cast<int>(
                        static_cast<float>(customBank->experience) * h3functions::GetHeroLearningPower(hero));

                    THISCALL_4(void, 0x4E3620, hero, currentCreatureBank.experiencePointsToAdd, false, true);
                    const UINT heroSpellPoints = hero->spellPoints;
                    const UINT maxSpellPoints = IntAt(0x049F99F + 2);
                    if (hero->spellPoints < maxSpellPoints)
                    {
                        currentCreatureBank.spellPointsToAdd =
                            Clamp(0, customBank->spellPoints, maxSpellPoints - heroSpellPoints);
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
                            if (spellId == eSpell::NONE || hero->learnedSpells[spellId] ||
                                P_Spell[spellId].level > maxSpellLevel)
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
                }
            }
        }
    }
    return EXEC_DEFAULT;
}

// works for Human Only
_LHF_(CreatureBanksExtender::CrBank_AfterDrawingResources)
{

    if (c->esi == MITHRIL_ID && currentCreatureBank.mithrilToAdd) // add one more iteration of next resource is mitrhil
    {
        // change  gold picture from money to gold if we have mithril
        ;
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

    if (const auto mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC))
    {
        const int creatureBankType = GetCreatureBankType(mapItem->objectType, mapItem->objectSubtype);
        if (creatureBankType != eObject::NO_OBJ)
        {

            if (const auto customCreatureBank = Get().GetCustomCreatureBank(mapItem))
            {
                H3PictureVector *pictureCategories = reinterpret_cast<H3PictureVector *>(c->ebp - 0x54);

                auto &message = currentCreatureBank.message;
                auto &hero = currentCreatureBank.hero;
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

                bool addSpells = false;
                int strIdx = 0;
                for (size_t i = 0; i < SPELLS_AMOUNT; i++)
                {
                    eSpell &spellId = currentCreatureBank.spellsToLearn[i];
                    if (spellId != eSpell::NONE)
                    {

                        H3PictureCategories pair = H3PictureCategories::Spell(spellId);
                        pictureCategories->Add(pair);
                        spellId = eSpell::NONE;
                        if (addSpells)
                        {
                            strIdx = 189;
                        }
                        else
                        {
                            addSpells = true;
                            strIdx = 185;
                        }
                    }
                }
                if (addSpells)
                {
                    libc::sprintf(h3_TextBuffer, P_AdveventText->GetText(strIdx), hero->name);
                    message.Append(' ').Append(h3_TextBuffer);
                }
            }
        }
    }
    // skip strings vector parsing if it is empty to avoid crash
    if (H3Vector<H3String> *strings = reinterpret_cast<H3Vector<H3String> *>(c->ebp - 0x44))
    {
        if (strings->IsEmpty())
        {
            c->esp -= 12;
            c->return_address = 0x04ABE51;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}

int __stdcall CreatureBanksExtender::CrBank_BeforeEndingText(HiHook *h, H3String *mes, const size_t len, const DWORD a3,
                                                             const DWORD a4) noexcept
{

    int result = THISCALL_4(int, h->GetDefaultFunc(), mes, len, a3, a4);
    //
    if (!mes->Empty() && !currentCreatureBank.message.Empty())
    {
        mes->Append('\n').Append(currentCreatureBank.message);
    }

    return result;
}

// works for Human and AI
_LHF_(CreatureBanksExtender::CrBank_BeforeGivingResources)
{
    if (auto &mithrilToAdd = currentCreatureBank.mithrilToAdd)
    {
        if (const auto hero = reinterpret_cast<H3Hero *>(c->ecx))
        {
            // add resource to hero
            THISCALL_3(void, 0x04E3870, hero, MITHRIL_ID, mithrilToAdd);
        }
        mithrilToAdd = 0;
    }
    currentCreatureBank = CreatureBanksExtender::Current();

    return EXEC_DEFAULT;
}

bool stateChagesDef = false;

_LHF_(CreatureBanksExtender::CrBank_BeforeSetupFromState)
{

    // init creature banks base data to assign to custom banks when they will be added into main array
    int &type = currentCreatureBank.type = c->ecx / 400;
    int &stateId = currentCreatureBank.stateId = c->edx;

    // check if we already in the game creature banks

    if (auto creatureBank = reinterpret_cast<H3CreatureBank *>(c->ebx))
    {
        auto &manager = Get().manager;
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
        auto &manager = Get().manager;

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

    auto &mapItem = currentCreatureBank.mapItem;
    auto &bank = currentCreatureBank.bank = nullptr;

    if (mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC))
    {
        if (GetCreatureBankType(mapItem) != eObject::NO_OBJ)
        {
            auto &creatureBanks = P_Game->Get()->creatureBanks;
            bank = &creatureBanks[mapItem->creatureBank.id];
        }
        else
        {
            bank = nullptr;
        }
    }
    return EXEC_DEFAULT;
}
_LHF_(CreatureBanksExtender::CrBank_BeforeCombatStart)
{
    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ecx))
    {
        currentCreatureBank.type = GetCreatureBankType(mapItem);
    }

    return EXEC_DEFAULT;
}
void CustomAskForCombatStartDlg(char *originalText, H3MapItem *mapItem, const int messageType = 2, const int x = -1,
                                const int y = -1)
{

    auto creatureBank = &P_Game->creatureBanks[mapItem->creatureBank.id];

    ShowMultiplePicsArmyMessage(originalText, 2, -1, -1, &creatureBank->guardians);
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

            CustomAskForCombatStartDlg(*reinterpret_cast<char **>(c->ebp + 0x10), mapItem, 2, -1, -1);
            c->return_address = 0x04AC1BE;
        }

        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
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

    int &type = currentCreatureBank.type;
    if (type != -1)
    {
        const auto &banks = Get().manager;

        if (isBank = banks.isBankSettings[type])
        {
            positionsPatch = _PI->WriteDword(0x04632A9 + 3, (int)banks.customPositions[type].data());
        }
        type = -1;
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
        if (objSubtype >= 0 && objSubtype < Get().addedBanksNumber + Get().defaultBanksNumber)
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

eObject CreatureBanksExtender::GetCreatureBankObjectType(const int creatureBankType) noexcept
{
    // get CB object type for some edits later
    eObject objectType = eObject::NO_OBJ;
    if (creatureBankType >= 0)
    {
        switch (creatureBankType)
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

void CreatureBanksExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
    // Get Default Banks Number from H3Vector<H3CreatureBankSetup::Ctor>::Size()
    defaultBanksNumber = ByteAt(0x47A3BA + 0x1);

    const int maxCreatureBankSubtype = maxSubtypes[eObject::CREATURE_BANK];

    // init vector sizes!
    addedBanksNumber = manager.LoadCreatureBanksFromJson(defaultBanksNumber, maxCreatureBankSubtype);

    if (addedBanksNumber)
    { // set new Creature Bank Setups data at native array address
        // IntAt(0x67029C) = (int)instance->creatureBanks.setups.data();
        const DWORD newCbArrayAddress = DWORD(Get().manager.setups.data());
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
        return ObjectsExtender::CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

inline int ReadJsonInt(LPCSTR format, const int arg, const int arg2)
{
    return EraJS::readInt(H3String::Format(format, arg, arg2).String());
}
inline int ReadJsonInt(LPCSTR format, const int arg, const int arg2, const int arg3)
{
    return EraJS::readInt(H3String::Format(format, arg, arg2, arg3).String());
}

CreatureBanksExtender::CustomRewardSetupState::CustomRewardSetupState(const INT creatureBankType,
                                                                      const UINT stateId) noexcept
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

    if (int _luck = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.luck", creatureBankType, stateId))
    {
        luck = Clamp(-3, _luck, 3);
    }
    if (int _morale = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.morale", creatureBankType, stateId))
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
            artifactIds[i] = eArtifact(Clamp(eArtifact::NONE, artId, currentCreatureBank.MAX_ART_ID));
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

int CreatureBanksExtender::CreatureBankManager::LoadCreatureBanksFromJson(const INT16 defaultBanksNumber,
                                                                          const INT16 maxSubtype)
{

    int addedBanksNumber = 0;

    const int MAX_MON_ID = IntAt(0x4A1657);
    currentCreatureBank.MAX_ART_ID = IntAt(0x49DD8E + 2) / 4;

    // init  default positions
    std::array<int, 14> defaultPositions{};
    constexpr int positionsAddress = 0x063D0E0;
    libc::memcpy(defaultPositions.data(), reinterpret_cast<int *>(positionsAddress), sizeof(defaultPositions));

    Resize(defaultBanksNumber);

    CopyDefaultData(defaultBanksNumber, defaultPositions);

    bool trSuccess = false;
    std::array<int, GUARDES_AMOUNT> tempArr = {};
    tempArr.fill(-1);

    //  allocate space for new creature banks
    Reserve(maxSubtype + 1 - defaultBanksNumber);
    for (INT16 creatureBankType = 0; creatureBankType < maxSubtype; creatureBankType++)
    {
        const int objectType = CreatureBanksExtender::GetCreatureBankObjectType(creatureBankType);
        const int objectSubtype = objectType == eObject::CREATURE_BANK ? creatureBankType : 0;
        if (objectType != eObject::NO_OBJ)
        {

            // states
            H3CreatureBankSetup setup;

            // copy original creature bank data
            if (creatureBankType < defaultBanksNumber)
            {
                setup = setups[creatureBankType];
            }
            else
            {
                setup.states[0].chance = 100; // default.chance
                for (size_t i = 0; i < STATES_AMOUNT; i++)
                {
                    for (size_t j = 0; j < ARTIFACTS_AMOUNT; j++)
                    {
                        setup.states[i].artifactTypeCounts[j] = 0;
                    }
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

            for (size_t i = 0; i < STATES_AMOUNT; i++)
            {
                auto &state = setup.states[i];

                const int creatureRewardType =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.creatureRewardType",
                                                    objectType, objectSubtype, i)
                                       .String(),
                                   trSuccess);

                if (trSuccess)
                {
                    state.creatureRewardType =
                        Clamp(eCreature::UNDEFINED, creatureRewardType, MAX_MON_ID); // creatureRewardType;
                }
                const int creatureRewardCount =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.states.%d.creatureRewardCount",
                                                    objectType, objectSubtype, i)
                                       .String(),
                                   trSuccess);
                if (trSuccess)
                {
                    state.creatureRewardCount = Clamp(0, creatureRewardCount, 127);
                }

                const int chance = EraJS::readInt(
                    H3String::Format("RMG.objectGeneration.%d.%d.states.%d.chance", objectType, objectSubtype, i)
                        .String(),
                    trSuccess);
                if (trSuccess)
                {
                    state.chance = Clamp(0, chance, 100);
                }

                const int upgrade = EraJS::readInt(
                    H3String::Format("RMG.objectGeneration.%d.%d.states.%d.upgrade", objectType, objectSubtype, i)
                        .String(),
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
                        state.guardians.type[j] =
                            Clamp(eCreature::UNDEFINED, guardType, MAX_MON_ID); // creatureRewardType;
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
            std::array<CustomRewardSetupState, STATES_AMOUNT> customReward{[creatureBankType]() {
                std::array<CustomRewardSetupState, STATES_AMOUNT> arr;
                for (size_t i = 0; i < STATES_AMOUNT; ++i)
                {
                    arr[i] = CustomRewardSetupState(creatureBankType, i);
                }
                return arr;
            }()};

            // placement troops in combat

            bool isBank = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.%d.%d.troopPlacement.isBank", objectType, objectSubtype)
                    .String(),
                trSuccess);

            const bool isBankSetting = trSuccess ? isBank : true;

            std::array<int, 14> positions = defaultPositions;

            for (size_t i = 0; i < 7; i++)
            {
                if (const int jsonAttackerPosition = ReadJsonInt(
                        "RMG.objectGeneration.%d.%d.troopPlacement.attackers.%d", objectType, objectSubtype, i))
                {

                    const BOOL atackerPositionIsValid =
                        !(jsonAttackerPosition < 1 || jsonAttackerPosition > 186 || jsonAttackerPosition % 17 == 0 ||
                          jsonAttackerPosition % 17 == 16);

                    if (atackerPositionIsValid)
                        positions[i] = jsonAttackerPosition;
                }

                if (const int jsonDefenderPosition = ReadJsonInt(
                        "RMG.objectGeneration.%d.%d.troopPlacement.defenders.%d", objectType, objectSubtype, i))
                {
                    const BOOL defenderPositionIsValid =
                        !(jsonDefenderPosition < 1 || jsonDefenderPosition > 186 || jsonDefenderPosition % 17 == 0 ||
                          jsonDefenderPosition % 17 == 16);
                    if (defenderPositionIsValid)
                        positions[i + 7] = jsonDefenderPosition;
                }
            }

            if (creatureBankType < defaultBanksNumber)
            {
                setups[creatureBankType] = setup;
                customRewardSetups[creatureBankType] = customReward;
                isBankSettings[creatureBankType] = isBankSetting;
                customPositions[creatureBankType] = positions;

                monsterAwards[creatureBankType] = setup.states[0].creatureRewardType;
            }
            else
            {
                setups.emplace_back(setup);
                customRewardSetups.emplace_back(customReward);
                isBankSettings.emplace_back(isBankSetting);
                customPositions.emplace_back(positions);

                monsterAwards.emplace_back(setup.states[0].creatureRewardType);
                memcpy(&tempArr[0], &setup.states[0].guardians.type[0], sizeof(tempArr));
                monsterGuards.emplace_back(tempArr);

                // increase number of added banks
                addedBanksNumber++;
            }
        }
    }
    if (addedBanksNumber > 0)
    {
        ValueAt<int *>(0x47A4A8 + 3) = monsterAwards.data();
        ValueAt<int *>(0x47A4AF + 3) = monsterGuards[0].data();
    }

    ShrinkToFit();

    return addedBanksNumber;
}

UINT CreatureBanksExtender::Size() const noexcept
{
    return manager.m_size;
}

void CreatureBanksExtender::CreatureBankManager::CopyDefaultData(const size_t defaultBanksNumber,
                                                                 const std::array<int, 14> &defaultPositions)
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

CreatureBanksExtender::~CreatureBanksExtender()
{
}

void CreatureBanksExtender::CreatureBankManager::Resize(const size_t size) noexcept
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

void CreatureBanksExtender::CreatureBankManager::Reserve(const size_t size) noexcept
{
    monsterAwards.reserve(size);
    monsterGuards.reserve(size);
    setups.reserve(size);
    isBankSettings.reserve(size);
    customPositions.reserve(size);
    customRewardSetups.reserve(size);
}

void CreatureBanksExtender::CreatureBankManager::ShrinkToFit() noexcept
{
    monsterAwards.shrink_to_fit();
    monsterGuards.shrink_to_fit();
    setups.shrink_to_fit();
    isBankSettings.shrink_to_fit();
    customPositions.shrink_to_fit();
    customRewardSetups.shrink_to_fit();
    m_size = monsterAwards.size();
}

const CreatureBanksExtender::CustomCreatureBank *CreatureBanksExtender::GetCustomCreatureBank(
    const H3MapItem *mapItem) noexcept
{
    return mapItem->creatureBank.id >= Size() ? nullptr : &manager.customCreatureBanksVector[mapItem->creatureBank.id];
}

void CreatureBanksExtender::CustomCreatureBank::GenerateSpells(const CustomRewardSetupState &setup)
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

// inits creature custom bank from predefined setup at the map start
CreatureBanksExtender::CustomCreatureBank::CustomCreatureBank(const CustomRewardSetupState &setupState,
                                                              const UINT creatureBankId)
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

void CreatureBanksExtender::CustomCreatureBank::WriteSaveData() const noexcept
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
void CreatureBanksExtender::CustomCreatureBank::ClearAssocVariables() const noexcept
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
CreatureBanksExtender::CustomCreatureBank::CustomCreatureBank(const int creatureBankId)
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

void __stdcall CreatureBanksExtender::OnBeforeSaveGame(Era::TEvent *Event)
{
    auto &manager = Get().manager;

    for (auto &i : manager.customCreatureBanksVector)
    {
        i.WriteSaveData();
    }
}
void __stdcall CreatureBanksExtender::OnAfterSaveGame(Era::TEvent *Event)
{
    auto &manager = Get().manager;

    for (auto &i : manager.customCreatureBanksVector)
    {
        i.ClearAssocVariables();
    }
}

void __stdcall CreatureBanksExtender::OnAfterLoadGame(Era::TEvent *Event)
{
    auto &manager = Get().manager;
    manager.customCreatureBanksVector.clear();
    const size_t length = P_Game->creatureBanks.Size();
    for (size_t i = 0; i < length; i++)
    {
        manager.customCreatureBanksVector.emplace_back(CustomCreatureBank(i)); // CustomCreatureBank)
    }
}
void __stdcall CreatureBanksExtender::OnGameLeave(Era::TEvent *Event)
{
    Get().manager.customCreatureBanksVector.clear();

    if (currentCreatureBank.positionsPatch)
    {
        currentCreatureBank.positionsPatch->Destroy();
    }
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

        _pi->WriteHiHook(0x0528520, FASTCALL_, AIHero_GetMapItemWeight); // creature bank new Rewards AI weight

        // chaging creature bank text patches
        {
            // Pre-combat message
            {
                _pi->WriteLoHook(0x04A1394, CrBank_DisplayPreCombatMessage);        // 16 object type
                _pi->WriteLoHook(0x04A1E29, CrBank_DisplayPreCombatMessage);        // 25 object type
                _pi->WriteLoHook(0x04AC19D, SpecialCrBank_DisplayPreCombatMessage); // crypt/ships object type

                _pi->WriteHiHook(0x04A13E6, FASTCALL_, CrBank_AskForVisitMessage); // 16 object type
                _pi->WriteHiHook(0x04A1E56, FASTCALL_, CrBank_AskForVisitMessage); // 25 object type
            }
            // Creature bank description for getting text
            {
                _pi->WriteDword(0x0413E23 + 1, 0x6603B0);      // ["\n\0" -> "\n\n\0" for getCrBank text]
                _pi->WriteHexPatch(0x040ABDE, "EB0C90909090"); // remove extra space in the guard description start
            }
        }

        //  _pi->WriteLoHook(0x041391C, AdvMgr_GetObjectRightClickDescr);

        _pi->WriteLoHook(0x4ABAD3, CrBank_BeforeCombatStart);
        _pi->WriteHiHook(0x4ABBCB, THISCALL_, CrBank_CombatStart);

        // giving custom rewards after combat won
        // if (0)
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
                _pi->WriteHiHook(0x04ABFD2, THISCALL_, CrBank_BeforeEndingText);
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
        }

        // renaming creature banks
        Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");

        m_isInited = true;
    }
}
} // namespace cbanks
