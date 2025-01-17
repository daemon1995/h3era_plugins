#include "..\pch.h"
namespace cbanks
{
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
    bankCombatCheck = false;

    CreatePatches();
}

CreatureBanksExtender &CreatureBanksExtender::Get()
{
    static CreatureBanksExtender _instance;
    return _instance;
}

_LHF_(CrBanksTxt_Ctor)
{

    return EXEC_DEFAULT;
}

void __stdcall CreatureBanksExtender::OnAfterReloadLanguageData(Era::TEvent *event)
{

    auto &creatureBanks = Get().creatureBanks;
    const int SZIE = Get().Size();
    const int defaultBanksNumber = Get().defaultBanksNumber;
    const int lastBankId = defaultBanksNumber + Get().addedBanksNumber;
    for (size_t i = 0; i < SZIE; i++)
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

void CreatureBanksExtender::CreatePatches()
{
    // m_isInited = true;

    if (m_isInited)
        return;

    m_isInited = true;

    _pi->WriteHiHook(0x04A13E6, FASTCALL_, CrBank_AskForVisitMessage); // 16 object type
    _pi->WriteHiHook(0x04A1E56, FASTCALL_, CrBank_AskForVisitMessage); // 25 object type

    _pi->WriteLoHook(0x04A1394, CrBank_DisplayPreCombatMessage);        // 16 object type
    _pi->WriteLoHook(0x04A1E29, CrBank_DisplayPreCombatMessage);        // 25 object type
    _pi->WriteLoHook(0x04AC19D, SpecialCrBank_DisplayPreCombatMessage); // crypt/ships object type
    //  _pi->WriteLoHook(0x041391C, AdvMgr_GetObjectRightClickDescr);

    _pi->WriteLoHook(0x4ABAD3, CrBank_BeforeCombatStart);

    _pi->WriteHiHook(0x4ABBCB, THISCALL_, CrBank_CombatStart);

    _pi->WriteHexPatch(0x040ABDE, "EB0C90909090"); // remove extra space in the guard description start
    //  _pi->WriteHexPatch(0x040AC7D, "EB0C90909090"); // remove extra space in the guard description between creatures

    _pi->WriteDword(0x0413E23 + 1, 0x6603B0); // ["\n\0" -> "\n\n\0" for getCrBank text]

    Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
}
// #define _UNIQUE
H3CreatureBank *currentCreatureBank = nullptr;
H3MapItem *currentMapItem = nullptr;
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
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ecx);
    if (mapItem && mapItem->objectType == eObject::CREATURE_BANK &&
        Get().creatureBanks.isNotBank[mapItem->objectSubtype])
        Get().bankCombatCheck = true;

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

signed int __stdcall CreatureBanksExtender::CrBank_CombatStart(HiHook *h, UINT AdvMan, UINT PosMixed, UINT attHero,
                                                               UINT attArmy, int PlayerIndex, UINT defTown,
                                                               UINT defHero, UINT defArmy, int seed, signed int a10,
                                                               int isBank)
{

    if (Get().bankCombatCheck)
    {
        isBank = false;
        Get().bankCombatCheck = false;
    }
    return THISCALL_11(signed int, h->GetDefaultFunc(), AdvMan, PosMixed, attHero, attArmy, PlayerIndex, defTown,
                       defHero, defArmy, seed, a10, isBank);
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
        cbId = 8;
        break;
    case eObject::DRAGON_UTOPIA:
        cbId = 10;
        break;
    case eObject::CRYPT:
        cbId = 9;
        break;
    case eObject::SHIPWRECK:
        cbId = 7;
        break;
    default:
        break;
    }

    return cbId;
}

eObject CreatureBanksExtender::GetCreatureBankObjectType(const int cbId) noexcept
{

    // get CB object type for some edits later
    eObject objectType = eObject::NO_OBJ;
    if (cbId >= 0)
    {
        switch (cbId)
        {
        case 7:
            objectType = eObject::SHIPWRECK;
            break;
        case 8:
            objectType = eObject::DERELICT_SHIP;
            break;
        case 9:
            objectType = eObject::CRYPT;
            break;
        case 10:
            objectType = eObject::DRAGON_UTOPIA;
            break;
        default:
            objectType = eObject::CREATURE_BANK;
            break;
        }
    }

    return objectType;
}

// SOUND FIND HERE 00418BB6

// H3WavFile* loop = trSuccess ? H3WavFile::Load(loooSoundName.String()) : nullptr;

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

const int CreatureBanksExtender::GetBankSetupsNumberFromJson(const INT16 maxSubtype)
{

    addedBanksNumber = 0;

    auto &banks = creatureBanks;
    // const int MAX_MON_ID = IntAt(0x4A1657);

    Reserve(30);

    bool trSuccess = false;

    // @todo add default creature banks editor
    //for (INT16 i = 0; i < defaultBanksNumber; i++)
    //{
    //}

    //// read new creature banks Data

    for (INT16 creatureBankId = defaultBanksNumber; creatureBankId < maxSubtype; creatureBankId++)
    {

        // states

        H3CreatureBankSetup setup;

        // assign new CB name from json/default
        H3String name =
            EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.name", creatureBankId).String(), trSuccess);
        if (!trSuccess && setup.name.Empty())
            name = H3ObjectName::Get()[eObject::CREATURE_BANK];
        setup.name = name;

        for (size_t state = 0; state < 4; state++)
        {

            setup.states[state].creatureRewardType = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.16.%d.states.%d.creatureRewardType", creatureBankId, state)
                    .String());
            setup.states[state].creatureRewardCount = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.16.%d.states.%d.creatureRewardCount", creatureBankId, state)
                    .String());
            setup.states[state].chance = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.16.%d.states.%d.chance", creatureBankId, state).String());
            setup.states[state].upgrade = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.16.%d.states.%d.upgrade", creatureBankId, state).String());

            for (size_t artLvl = 0; artLvl < 4; artLvl++)
                setup.states[state].artifactTypeCounts[artLvl] =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.artifactTypeCounts.%d",
                                                    creatureBankId, state, artLvl)
                                       .String());

            for (size_t j = 0; j < 7; j++)
            {

                setup.states[state].guardians.type[j] = EraJS::readInt(
                    H3String::Format("RMG.objectGeneration.16.%d.states.%d.guardians.type.%d", creatureBankId, state, j)
                        .String());
                setup.states[state].guardians.count[j] =
                    EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.guardians.count.%d",
                                                    creatureBankId, state, j)
                                       .String());
                setup.states[state].resources.asArray[j] = EraJS::readInt(
                    H3String::Format("RMG.objectGeneration.16.%d.states.%d.resources.%d", creatureBankId, state, j)
                        .String());
            }
        }

        int isNotBank =
            EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.isNotBank", creatureBankId).String());

        creatureBanks.isNotBank.emplace_back(isNotBank);
        creatureBanks.setups.emplace_back(setup);
        creatureBanks.monsterAwards.emplace_back(setup.states[0].creatureRewardType);

        std::array<int, 5> tempArr = {-1, -1, -1, -1, -1};
        memcpy(&tempArr[0], &setup.states[0].guardians.type[0], sizeof(tempArr));

        creatureBanks.monsterGuards.emplace_back(tempArr);

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
    // instance->_pi->WriteWord(0x47A4A8 + 3, (int)monsterAwards.data());

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

    creatureBanks.m_size = m_size;
}

void CreatureBanksExtender::Reserve(UINT16 m_size) noexcept
{
    creatureBanks.monsterAwards.reserve(m_size);
    creatureBanks.monsterGuards.reserve(m_size);
    creatureBanks.setups.reserve(m_size);
    creatureBanks.isNotBank.reserve(m_size);
}

void CreatureBanksExtender::ShrinkToFit() noexcept
{
    creatureBanks.monsterAwards.shrink_to_fit();
    creatureBanks.monsterGuards.shrink_to_fit();
    creatureBanks.setups.shrink_to_fit();
    creatureBanks.isNotBank.shrink_to_fit();

    creatureBanks.m_size = creatureBanks.monsterAwards.size();
}

} // namespace cbanks
