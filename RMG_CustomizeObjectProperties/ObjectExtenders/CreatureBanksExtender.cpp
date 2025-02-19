#include "..\pch.h"
namespace cbanks
{
INT CreatureBanksExtender::currentCreatureBankId = -1;
H3MapItem *CreatureBanksExtender::currentMapItem = nullptr;
H3CreatureBank *CreatureBanksExtender::currentCreatureBank = nullptr;
UINT CreatureBanksExtender::mithrilToAdd = 0;
INT CreatureBanksExtender::creatureBankStateId = -1;
std::array<eSpell, CreatureBanksExtender::STATES_AMOUNT> CreatureBanksExtender::spellsToLearn;
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
    // bankCombatCheck = false;

    CreatePatches();
}

CreatureBanksExtender &CreatureBanksExtender::Get()
{
    static CreatureBanksExtender _instance;
    return _instance;
}

void __stdcall CreatureBanksExtender::OnAfterReloadLanguageData(Era::TEvent *event)
{

    auto &creatureBanks = Get().creatureBanks;
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

        auto &bankSetup = Get().creatureBanks.setups[i];
        if (trSuccess)
        {
            bankSetup.name = name;
        }
    }
}
_LHF_(CreatureBanksExtender::CrBank_AfterCombatWon)
{
    // if mitrhil option is enabled

    if (const auto mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC))

    {
        const int creatureBankId = GetCreatureBankId(mapItem->objectType, mapItem->objectSubtype);

        if (creatureBankId != eObject::NO_OBJ)
        {

            spellsToLearn.fill(eSpell::NONE);

            libc::sprintf(h3_TextBuffer, CustomReward::hasCustomSetupFormat, mapItem->creatureBank.id);
            const int hasSetup = Era::GetAssocVarIntValue(h3_TextBuffer);

            //  const auto &reward = Get().creatureBanks.customRewards[creatureBankId][stateId];
            if (hasSetup)
            {
                // reset state about reward
                Era::SetAssocVarIntValue(h3_TextBuffer, 0);

                if (auto hero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8))
                {
                    if (hero->WearsArtifact(eArtifact::SPELLBOOK))
                    {
                        const int maxSpellLevel = hero->secSkill[eSecondary::WISDOM] + 2;
                        for (size_t i = 0; i < STATES_AMOUNT; i++)
                        {
                            libc::sprintf(h3_TextBuffer, CustomReward::creatureBankSpellsFormat,
                                          mapItem->creatureBank.id, i);

                            const eSpell spellId = eSpell(Era::GetAssocVarIntValue(h3_TextBuffer));
                            if (spellId == eSpell::NONE || hero->learnedSpells[spellId] ||
                                P_Spell[spellId].level > maxSpellLevel)
                            {
                                continue;
                            }
                            spellsToLearn[i] = spellId;
                        }
                    }
                }
            }
            // if mithril is enabled
            if (DwordAt(0x27F99AC))
            {
                libc::sprintf(h3_TextBuffer, creatureBankStateFormat, mapItem->creatureBank.id);
                const int stateId = Era::GetAssocVarIntValue(h3_TextBuffer);
                mithrilToAdd = Get().creatureBanks.mithrilAmount[creatureBankId][stateId];
            }
        }
    }
    return EXEC_DEFAULT;
}

// works for Human Only
_LHF_(CreatureBanksExtender::CrBank_AfterDrawingResources)
{
    if (c->esi == MITHRIL_ID && mithrilToAdd) // add one more iteration of next resource is mitrhil
    {
        c->eax = mithrilToAdd;
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
        const int creatureBankId = GetCreatureBankId(mapItem->objectType, mapItem->objectSubtype);
        if (creatureBankId != eObject::NO_OBJ)
        {
            const int cbUniqueId = mapItem->creatureBank.id;
            libc::sprintf(h3_TextBuffer, creatureBankStateFormat, cbUniqueId);

            const int stateId = Era::GetAssocVarIntValue(h3_TextBuffer);

            libc::sprintf(h3_TextBuffer, CustomReward::hasCustomSetupFormat, cbUniqueId);

            // if taht state has custom setup
            if (Era::GetAssocVarIntValue(h3_TextBuffer))
            {
                H3PictureVector *pictureCategories = reinterpret_cast<H3PictureVector *>(c->ebp - 0x54);
                // const auto &setup = Get().creatureBanks.customRewards[creatureBankId][stateId];

                for (size_t i = 0; i < CustomReward::SPELLS_AMOUNT; i++)
                {
                    const eSpell spellId = spellsToLearn[i];
                    if (spellId != eSpell::NONE)
                    {
                        H3PictureCategories pair = H3PictureCategories::Spell(spellId);

                        pictureCategories->Add(pair);
                    }
                }
            }
        }
    }

    return EXEC_DEFAULT;
}
int __stdcall CreatureBanksExtender::CrBank_BeforeEndingText(HiHook *h, H3String *mes, const size_t len, const DWORD a3,
                                                             const DWORD a4) noexcept
{

    int result = THISCALL_4(int, h->GetDefaultFunc(), mes, len, a3, a4);
    //
    if (mes->Empty())
    {
    }
    mes->Append("\n\ntetetet");

    return result;
}

// works for Human and AI
_LHF_(CreatureBanksExtender::CrBank_BeforeGivingResources)
{
    if (mithrilToAdd)
    {
        if (const auto hero = reinterpret_cast<H3Hero *>(c->ecx))
        {
            // add resource to hero
            THISCALL_3(void, 0x04E3870, hero, MITHRIL_ID, mithrilToAdd);
        }
        mithrilToAdd = 0;
    }

    return EXEC_DEFAULT;
}
_LHF_(CreatureBanksExtender::CrBank_BeforeSetupFromState)
{
    creatureBankStateId = c->edx;

    return EXEC_DEFAULT;
}
_LHF_(CreatureBanksExtender::CrBank_BeforeAddingToGameList)
{
    if (creatureBankStateId != -1)
    {
        if (const auto mapItem = reinterpret_cast<H3MapItem *>(c->esi))
        {
            const UINT creatureBankId = mapItem->creatureBank.id;
            libc::sprintf(h3_TextBuffer, creatureBankStateFormat, creatureBankId);
            Era::SetAssocVarIntValue(h3_TextBuffer, creatureBankStateId);

            // add custom spells
            const auto &bannedInfo = P_Game->Get()->disabledSpells;
            const auto &customReward = Get().creatureBanks.customRewards[mapItem->objectSubtype][creatureBankStateId];
            if (customReward.enabled)
            {
                // init custom setup
              //  libc::sprintf(h3_TextBuffer, CustomReward::hasCustomSetupFormat, creatureBankId);
               // Era::SetAssocVarIntValue(h3_TextBuffer, true);

                // init spells generation
                static std::unordered_set<int> spellsSet;
                for (size_t i = 0; i < CustomReward::SPELLS_AMOUNT; i++)
                {
                    const auto &data = customReward.spellsRewards[i];

                    // init no spells for that creature bank
                    libc::sprintf(h3_TextBuffer, CustomReward::creatureBankSpellsFormat, creatureBankId, i);
                    Era::SetAssocVarIntValue(h3_TextBuffer, eSpell::NONE);
                    const eSpell spell = data.spellId;
                    if (spell != eSpell::NONE)
                    {
                        spellsSet.insert(spell);
                    }
                    else if (data.generate)
                    {
                        static std::vector<UINT> spellsToSelect;
                        spellsToSelect.reserve(h3::limits::SPELLS);
                        INT spellId = 0;
                        do
                        {
                            const auto &spell = P_Spell[spellId];

                            if (!bannedInfo[spellId] && spell.flags & data.spellFlags &&
                                data.spellLevels & (1 << spell.level) && data.spellSchool & spell.school)
                            {
                                spellsToSelect.emplace_back(spellId);
                            }
                        } while (spellId++ < h3::limits::SPELLS);

                        // add spell to set if exist
                        if (const size_t goodSpells = spellsToSelect.size())
                        {
                            spellsSet.insert(spellsToSelect[H3Random::RandBetween(0, goodSpells - 1)]);
                            spellsToSelect.clear();
                        }
                    }
                }

                volatile int i = 0;
                for (const auto spell : spellsSet)
                {
                    libc::sprintf(h3_TextBuffer, CustomReward::creatureBankSpellsFormat, creatureBankId, i++);
                    Era::SetAssocVarIntValue(h3_TextBuffer, spell);
                }
                spellsSet.clear();
            }
        }

        creatureBankStateId = -1;
    }

    return EXEC_DEFAULT;
}

void CreatureBanksExtender::CreatePatches()
{
    // m_isInited = true;

    if (m_isInited)
        return;

    m_isInited = true;

    _pi->WriteLoHook(0x04C0C5B, CrBank_BeforeAddingToGameList);
    _pi->WriteLoHook(0x047A70D, CrBank_BeforeSetupFromState);

    _pi->WriteHiHook(0x04A13E6, FASTCALL_, CrBank_AskForVisitMessage); // 16 object type
    _pi->WriteHiHook(0x04A1E56, FASTCALL_, CrBank_AskForVisitMessage); // 25 object type

    _pi->WriteLoHook(0x04A1394, CrBank_DisplayPreCombatMessage);        // 16 object type
    _pi->WriteLoHook(0x04A1E29, CrBank_DisplayPreCombatMessage);        // 25 object type
    _pi->WriteLoHook(0x04AC19D, SpecialCrBank_DisplayPreCombatMessage); // crypt/ships object type
    //  _pi->WriteLoHook(0x041391C, AdvMgr_GetObjectRightClickDescr);

    _pi->WriteLoHook(0x4ABAD3, CrBank_BeforeCombatStart);

    _pi->WriteHiHook(0x4ABBCB, THISCALL_, CrBank_CombatStart);

    // Adding mithril to player
    _pi->WriteLoHook(0x04ABBFF, CrBank_AfterCombatWon);
    _pi->WriteLoHook(0x04ABE3C, CrBank_AfterDrawingResources);
    _pi->WriteLoHook(0x04ABE4C, CrBank_BeforeShowingRewardMessage);
    _pi->WriteHiHook(0x04ABFD2, THISCALL_, CrBank_BeforeEndingText);

    _pi->WriteLoHook(0x04AC13B, CrBank_BeforeGivingResources);

    _pi->WriteHexPatch(0x040ABDE, "EB0C90909090"); // remove extra space in the guard description start
    //  _pi->WriteHexPatch(0x040AC7D, "EB0C90909090"); // remove extra space in the guard description between creatures

    _pi->WriteDword(0x0413E23 + 1, 0x6603B0); // ["\n\0" -> "\n\n\0" for getCrBank text]

    Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
}

_LHF_(CreatureBanksExtender::CrBank_DisplayPreCombatMessage)
{
    currentCreatureBank = nullptr;

    if (currentMapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC))
    {
        currentCreatureBank =
            GetCreatureBankId(currentMapItem->objectType, currentMapItem->objectSubtype) != eObject::NO_OBJ
                ? &P_Game->creatureBanks[currentMapItem->creatureBank.id]
                : nullptr;
    }

    return EXEC_DEFAULT;
}
_LHF_(CreatureBanksExtender::CrBank_BeforeCombatStart)
{
    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ecx))
    {
        currentCreatureBankId = GetCreatureBankId(mapItem->objectType, mapItem->objectSubtype);
    }

    return EXEC_DEFAULT;
}
void CustomAskForCombatStartDlg(char *originalText, H3MapItem *mapItem, const int messageType = 2, const int x = -1,
                                const int y = -1)
{

    auto creatureBank = &P_Game->creatureBanks[mapItem->creatureBank.id];
    //  Era::y[1] = creatureBank->c
    //  Era::ExecErmCmd("IF:L^%y1^");
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

signed int __stdcall CreatureBanksExtender::CrBank_CombatStart(HiHook *h, UINT AdvMan, UINT PosMixed, UINT attHero,
                                                               UINT attArmy, int PlayerIndex, UINT defTown,
                                                               UINT defHero, UINT defArmy, int seed, signed int a10,
                                                               int isBank)
{

    static Patch *positionsPatch = nullptr;
    if (currentCreatureBankId != -1)
    {
        auto &banks = Get().creatureBanks;
        isBank = ~(banks.isNotBank[currentCreatureBankId]);

        positionsPatch = _PI->WriteDword(0x04632A9 + 3, (int)banks.customPositions[currentCreatureBankId].data());

        currentCreatureBankId = -1;
    }

    const int result = THISCALL_11(signed int, h->GetDefaultFunc(), AdvMan, PosMixed, attHero, attArmy, PlayerIndex,
                                   defTown, defHero, defArmy, seed, a10, isBank);

    if (positionsPatch)
    {
        positionsPatch->Undo();
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

    char *customVisitText = EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.visit",
                                                         currentMapItem->objectType, currentMapItem->objectSubtype)
                                            .String(),
                                        readSuccess);

    if (readSuccess)
    {
        resultMessage = customVisitText;
    }

    H3String finalMessage;
    if (EraJS::readInt("RMG.settings.creatureBanks.displayName") &&
        currentMapItem->objectType == eObject::CREATURE_BANK) // other CB has name by default
    {
        finalMessage = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(currentMapItem));
    }

    if (finalMessage.Length())
    {
        finalMessage.Append("\n\n");
    }
    finalMessage.Append(resultMessage);

    H3String armySizeMessage;
    GetArmyMessage(currentCreatureBank, armySizeMessage);

    if (finalMessage.Length() && !armySizeMessage.Empty())
    {
        finalMessage.Append("\n\n");
    }
    finalMessage.Append(armySizeMessage);

    if (EraJS::readInt("RMG.settings.creatureBanks.extendedDlgInformation"))
    {
        ShowMultiplePicsArmyMessage(finalMessage.String(), messageType, x, y, &currentCreatureBank->guardians);
    }
    else
    {
        FASTCALL_12(void, h->GetDefaultFunc(), finalMessage.String(), messageType, x, y, picType1, picSubtype1,
                    picType2, picSubtype2, par, time, picType3, picSubtype3);
    }
}

int CreatureBanksExtender::GetCreatureBankId(const int objType, const int objSubtype) noexcept
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

eObject CreatureBanksExtender::GetCreatureBankObjectType(const int creatureBankId) noexcept
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

void CreatureBanksExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
    // Get Default Banks Number from H3Vector<H3CreatureBankSetup::Ctor>::Size()
    defaultBanksNumber = ByteAt(0x47A3BA + 0x1);

    Resize(defaultBanksNumber);
    creatureBanks.CopyDefaultData(defaultBanksNumber);

    const int maxCreatureBankSubtype = maxSubtypes[eObject::CREATURE_BANK];

    // init vector sizes!
    const int banksAdded = GetBankSetupsNumberFromJson(maxCreatureBankSubtype);

    if (banksAdded)
    { // set new Creature Bank Setups data at native array address
        // IntAt(0x67029C) = (int)instance->creatureBanks.setups.data();
        const DWORD newCbArrayAddress = DWORD(Get().creatureBanks.setups.data());
        // set new address for the using CB array ptr

        _pi->WriteDword(0x67029C, newCbArrayAddress);
        //_pi->WriteDword(0x047A4B6 +3, newCbArrayAddress + 4);

        //_pi->WriteByte(0x47A3BA +1, defaultBanksNumber + banksAdded);
        // set new address for the H3CreatureBankSetup init vector to use H3CreatureBankSetup::Get(); properly with new
        // setup
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
inline int ReadJsonInt(LPCSTR format, const int arg)
{
    return EraJS::readInt(H3String::Format(format, arg).String());
}
inline int ReadJsonInt(LPCSTR format, const int arg, const int arg2)
{
    return EraJS::readInt(H3String::Format(format, arg, arg2).String());
}
inline int ReadJsonInt(LPCSTR format, const int arg, const int arg2, const int arg3)
{
    return EraJS::readInt(H3String::Format(format, arg, arg2, arg3).String());
}
void CreatureBanksExtender::CustomReward::ReadStateFromJson(const INT16 creatureBankId, const UINT stateId) noexcept
{

    experience = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.experience", creatureBankId, stateId);
    spellPoints = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spellPoints", creatureBankId, stateId);

    if (int _luck = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.luck", creatureBankId, stateId))
    {
        luck = Clamp(-3, _luck, 3);
    }
    if (int _morale = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.morale", creatureBankId, stateId))
    {
        morale = Clamp(-3, _morale, 3);
    }

    bool readSuccess = false;
    for (size_t i = 0; i < SPELLS_AMOUNT; i++)
    {
        primarySkills[i] =
            ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.skills.primary.%d", creatureBankId, stateId, i);

        int spellId = EraJS::readInt(
            H3String::Format("RMG.objectGeneration.16.%d.states.%d.spells.%d.id", creatureBankId, stateId, i).String(),
            readSuccess);
        if (readSuccess)
        {
            spellsRewards[i].spellId = eSpell(Clamp(0, spellId, h3::limits::SPELLS));
        }
        else
        {
            UINT spellSchool =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spells.%d.bits.schools", creatureBankId, stateId, i);

            UINT spellLevels =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spells.%d.bits.levels", creatureBankId, stateId, i);

            UINT spellFlags =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.spells.%d.bits.flags", creatureBankId, stateId, i);
            // if any of data is set then spell may be generated

            if (spellSchool || spellLevels || spellFlags)
            {
                spellsRewards[i].generate = true;
                spellsRewards[i].spellSchool =
                    spellSchool ? eSpellchool(Clamp(0, spellSchool, h3::eSpellchool::ALL)) : eSpellchool::ALL;

                spellsRewards[i].spellLevels = spellLevels ? Clamp(0, spellLevels, 31) : 31;

                spellsRewards[i].spellFlags = spellFlags ? spellFlags : -1;
            }
        }

        static CustomReward defaultStateReward;
        // set "enabled" if object is not default
        enabled = memcmp(this, &defaultStateReward, sizeof(defaultStateReward));
    }
}

int CreatureBanksExtender::GetBankSetupsNumberFromJson(const INT16 maxSubtype)
{

    addedBanksNumber = 0;

    auto &banks = creatureBanks;
    // const int MAX_MON_ID = IntAt(0x4A1657);

    Reserve(30);

    bool trSuccess = false;
    static std::array<int, 14> defaultPositions;

    constexpr int postionsAddress = 0x063D0E0;

    libc::memcpy(defaultPositions.data(), reinterpret_cast<int *>(postionsAddress), sizeof(defaultPositions));

    // @todo add default creature banks editor
    for (INT16 i = 0; i < defaultBanksNumber; i++)
    {
        creatureBanks.customPositions[i] = defaultPositions;
    }

    for (INT16 creatureBankId = defaultBanksNumber; creatureBankId < maxSubtype; creatureBankId++)
    {

        // states
        static H3CreatureBankSetup setup;

        // assign new CB name from json/default
        H3String name =
            EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.name", creatureBankId).String(), trSuccess);
        if (!trSuccess && setup.name.Empty())
            name = H3ObjectName::Get()[eObject::CREATURE_BANK];
        setup.name = name;

        // read custom postions
        std::array<int, 14> positions = defaultPositions;

        for (size_t i = 0; i < 7; i++)
        {
            const int jsonAttackerPosition =
                ReadJsonInt("RMG.objectGeneration.16.%d.attackerPositions.%d", creatureBankId, i);

            const BOOL atackerPositionIsValid = !(jsonAttackerPosition < 1 || jsonAttackerPosition > 186 ||
                                                  jsonAttackerPosition % 17 == 0 || jsonAttackerPosition % 17 == 16);

            if (atackerPositionIsValid)
                positions[i] = jsonAttackerPosition;

            const int jsonDefenderPosition =
                ReadJsonInt("RMG.objectGeneration.16.%d.defenderPositions.%d", creatureBankId, i);

            const BOOL defenderPositionIsValid = !(jsonDefenderPosition < 1 || jsonDefenderPosition > 186 ||
                                                   jsonDefenderPosition % 17 == 0 || jsonDefenderPosition % 17 == 16);
            if (defenderPositionIsValid)
                positions[i + 7] = jsonDefenderPosition;
        }

        static std::array<UINT, STATES_AMOUNT> mithrilRewards{};
        mithrilRewards.fill(0);

        static std::array<CustomReward, STATES_AMOUNT> customReward{};
        customReward.fill(CustomReward{});

        for (size_t i = 0; i < STATES_AMOUNT; i++)
        {
            auto &state = setup.states[i];

            state.creatureRewardType =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.creatureRewardType", creatureBankId, i);
            state.creatureRewardCount =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.creatureRewardCount", creatureBankId, i);
            state.chance = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.chance", creatureBankId, i);
            state.upgrade = ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.upgrade", creatureBankId, i);

            for (size_t artLvl = 0; artLvl < 4; artLvl++)
            {
                state.artifactTypeCounts[artLvl] = ReadJsonInt(
                    "RMG.objectGeneration.16.%d.states.%d.artifactTypeCounts.%d", creatureBankId, i, artLvl);
            }

            for (size_t j = 0; j < 7; j++)
            {
                state.guardians.type[j] =
                    ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.guardians.type.%d", creatureBankId, i, j);
                state.guardians.count[j] =
                    ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.guardians.count.%d", creatureBankId, i, j);
                state.resources.asArray[j] =
                    ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.resources.%d", creatureBankId, i, j);
            }
            mithrilRewards[i] =
                ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.resources.%d", creatureBankId, i, MITHRIL_ID);
            if (ReadJsonInt("RMG.objectGeneration.16.%d.states.%d.customReward", creatureBankId, i))
            {
                customReward[i].ReadStateFromJson(creatureBankId, i);
            }
        }

        const int isNotBank = ReadJsonInt("RMG.objectGeneration.16.%d.isNotBank", creatureBankId);

        creatureBanks.customPositions.emplace_back(positions);
        creatureBanks.isNotBank.emplace_back(isNotBank);
        creatureBanks.setups.emplace_back(setup);
        creatureBanks.monsterAwards.emplace_back(setup.states[0].creatureRewardType);
        creatureBanks.customRewards.emplace_back(customReward);
        static std::array<int, GUARDES_AMOUNT> tempArr;
        tempArr.fill(-1);
        memcpy(&tempArr[0], &setup.states[0].guardians.type[0], sizeof(tempArr));

        creatureBanks.monsterGuards.emplace_back(tempArr);
        creatureBanks.mithrilAmount.emplace_back(mithrilRewards);
        addedBanksNumber++;
    }

    ShrinkToFit();

    return addedBanksNumber;
}

UINT CreatureBanksExtender::Size() const noexcept
{
    return creatureBanks.m_size;
}

void CreatureBanksExtender::CreatureBank::CopyDefaultData(const size_t defaultSize)
{

    int *currentCreatureRewardsArray = reinterpret_cast<int *>(IntAt(0x47A4A8 + 3));
    memcpy(monsterAwards.data(), currentCreatureRewardsArray, sizeof(int) * defaultSize);
    IntAt(0x47A4A8 + 3) = (int)monsterAwards.data();

    int *currentGuardiansArray = reinterpret_cast<int *>(IntAt(0x47A4AF + 3));
    memcpy(monsterGuards[0].data(), currentGuardiansArray, sizeof(int) * defaultSize * 5);
    IntAt(0x47A4AF + 3) = (int)monsterGuards[0].data();

    // get original bank setups data array
    H3CreatureBankSetup *originalBanks = *reinterpret_cast<H3CreatureBankSetup **>(0x67029C);
    if (originalBanks)
    {
        // iterate default data and copy into current array
        for (size_t i = 0; i < defaultSize; i++)
        {
            setups[i] = originalBanks[i];
        }
    }
}

CreatureBanksExtender::~CreatureBanksExtender()
{
}

void CreatureBanksExtender::Resize(UINT16 m_size) noexcept
{
    creatureBanks.monsterAwards.resize(m_size);
    creatureBanks.monsterGuards.resize(m_size);
    creatureBanks.setups.resize(m_size);
    creatureBanks.isNotBank.resize(m_size);
    creatureBanks.customPositions.resize(m_size);
    creatureBanks.mithrilAmount.resize(m_size);
    creatureBanks.customRewards.resize(m_size);
    creatureBanks.m_size = m_size;
}

void CreatureBanksExtender::Reserve(UINT16 m_size) noexcept
{
    creatureBanks.monsterAwards.reserve(m_size);
    creatureBanks.monsterGuards.reserve(m_size);
    creatureBanks.setups.reserve(m_size);
    creatureBanks.isNotBank.reserve(m_size);
    creatureBanks.customPositions.reserve(m_size);
    creatureBanks.mithrilAmount.reserve(m_size);
    creatureBanks.customRewards.reserve(m_size);
}

void CreatureBanksExtender::ShrinkToFit() noexcept
{
    creatureBanks.monsterAwards.shrink_to_fit();
    creatureBanks.monsterGuards.shrink_to_fit();
    creatureBanks.setups.shrink_to_fit();
    creatureBanks.isNotBank.shrink_to_fit();
    creatureBanks.customPositions.shrink_to_fit();
    creatureBanks.mithrilAmount.shrink_to_fit();
    creatureBanks.customRewards.shrink_to_fit();

    creatureBanks.m_size = creatureBanks.monsterAwards.size();
}

} // namespace cbanks
