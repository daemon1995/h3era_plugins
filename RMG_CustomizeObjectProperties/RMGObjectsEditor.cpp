#include "pch.h"
#include <thread>

std::vector<RMGObjectInfo> RMGObjectInfo::currentRMGObjectsInfoByType[h3::limits::OBJECTS];
std::vector<RMGObjectInfo> RMGObjectInfo::defaultRMGObjectsInfoByType[h3::limits::OBJECTS];

namespace editor
{
GeneratedInfo RMGObjectsEditor::generatedInfo;

RMGObjectsEditor::RMGObjectsEditor() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.RMG.ObjectEditor.daemon_n"))
{
    CreatePatches();
}

void RMGObjectsEditor::Init(const INT16 *maxSubtypes)
{
    auto &editor = RMGObjectsEditor::Get();

    // init data in the main
    editor.InitDefaultProperties(maxSubtypes);

    // load ini data in the separate thread
    std::thread th(&RMGObjectInfo::LoadUserProperties, maxSubtypes);
    th.detach();
}

int RMGObjectsEditor::MaxMapTypeLimit(const UINT objType) const noexcept
{
    return objType < H3_MAX_OBJECTS ? limitsInfo.mapTypesLimit[objType] : 0;
}
inline BOOL ObjectMayBeGenerated(H3RmgObjectGenerator *p_ObjGen)
{

    return wog::WoGObjectsExtender::IsWoGObject(p_ObjGen) ? wog::WoGObjectsExtender::WoGObjectHasOptionEnabled(p_ObjGen)
                                                          : true;
}
void RMGObjectsEditor::InitDefaultProperties(const INT16 *maxSubtypes)
{

    // set globalData array default size

    // get default limit from original code
    const int defaultLimit = IntAt(0x538232 + 1);

    std::fill(std::begin(limitsInfo.mapTypesLimit), std::end(limitsInfo.mapTypesLimit), defaultLimit);
    std::fill(std::begin(limitsInfo.zoneTypeLimits), std::end(limitsInfo.zoneTypeLimits), defaultLimit);

    // repeat original setting default code
    struct Limit
    {
        eObject type;
        INT32 value;
    };

    Limit *limit = reinterpret_cast<Limit *>(0x640728);
    for (size_t i = 0; i < 30; ++i)
    {
        limitsInfo.mapTypesLimit[limit[i].type] = limit[i].value;
    }

    limit = reinterpret_cast<Limit *>(0x640818);
    for (size_t i = 0; i < 24; ++i)
    {
        limitsInfo.zoneTypeLimits[limit[i].type] = limit[i].value;
    }

    RMGObjectInfo::InitDefaultProperties(limitsInfo, maxSubtypes);

    // init pseudoGanerator

    // skip pandora monster generation
    auto patch = _pi->WriteByte(0x5390B7, 0xEB);
    // pseudoH3RmgRandomMapGenerator.keyMasters.RemoveAll();

    isPseudoGeneration = true;
    // pseudoH3RmgRandomMapGenerator.objectPrototypes[eObject::KEYMASTER] =
    // P_Game->mainSetup.objectLists[eObject::KEYMASTER];

    THISCALL_1(void, 0x539000, &pseudoH3RmgRandomMapGenerator); // RMG__CreateObjectGenerators

    originalRMGObjectGenerators = &pseudoH3RmgRandomMapGenerator.objectGenerators;

    isPseudoGeneration = false;
    patch->Destroy();
}

const H3Vector<H3RmgObjectGenerator *> &RMGObjectsEditor::GetObjectGeneratorsList() const noexcept
{
    return *originalRMGObjectGenerators; // .objectGenerators;
}

RMGObjectsEditor &RMGObjectsEditor::Get() noexcept
{
    static RMGObjectsEditor instance;

    return instance;
    // new RMGObjectsEditor();
}

RMGObjectsEditor::~RMGObjectsEditor()
{
}

void __stdcall RMGObjectsEditor::RMG__CreateObjectGenerators(HiHook *h, H3RmgRandomMapGenerator *rmgStruct)
{

    // check WoG Dwellings
    const int dwellingsNumber = IntAt(0x0539C76 + 1);

    // if value is 80 (SoD value)
    if (dwellingsNumber == 80)
    {
        IntAt(0x0539C76 + 1) = 101;
    }

    // call default function to create rmg gen list first
    THISCALL_1(void, h->GetDefaultFunc(), rmgStruct);

    // then get objGen vector
    if (H3Vector<H3RmgObjectGenerator *> *rmgObjectsList = &rmgStruct->objectGenerators)
    {
        // and add objects by properly allocated memory m_size and types (this case is CB)

        extender::ObjectsExtender::AddObjectsToObjectGenList(rmgObjectsList);

        //	return;
        // edit current objects by type/subtype
        auto &editor = RMGObjectsEditor::Get();
        const BOOL isPseudoGeneration = Get().isPseudoGeneration;

        if (isPseudoGeneration)
        {

            for (auto &rmgObj : *rmgObjectsList)
            {
                RMGObjectInfo::InitFromRmgObjectGenerator(*rmgObj);
            }
        }
        else
        {
            // if map generation we affect the reall array with new data

            // create vector with assumed to create objects ;
            editor.editedRMGObjectGenerators.RemoveAll();

            // check if we have full random
            const BOOL result = Era::ReadStrFromIni("alwaysRandom", rmgdlg::RMG_SettingsDlg::SETTINGS_INI_SECTION,
                                                    rmgdlg::RMG_SettingsDlg::INI_FILE_PATH, h3_TextBuffer);
            const BOOL randomizeProperties = result && atoi(h3_TextBuffer);

            // affects only data from SettingsDlg
            if (randomizeProperties)
            {
                // and start to make dirt
                auto &allDlgObjects = rmgdlg::RMG_SettingsDlg::GetObjectAttributes();
                for (auto vec : allDlgObjects)
                {
                    for (auto &attr : *vec)
                    {
                        RMGObjectInfo info(attr.first.type, attr.first.subtype);
                        info.SetRandom();
                        info.MakeReal();
                    }
                }
            }

            for (auto &rmgObjGen : *rmgObjectsList)
            {

                switch (rmgObjGen->type)
                {

                case eObject::PANDORAS_BOX:
                case eObject::KEYMASTER:
                case eObject::PRISON:
                case eObject::SEER_HUT:
                case eObject::SPELL_SCROLL:

                    editor.editedRMGObjectGenerators.Add(rmgObjGen);
                    continue;

                default:
                    break;
                }

                const auto &rmgObjInfo = RMGObjectInfo::CurrentObjectInfo(rmgObjGen->type, rmgObjGen->subtype);
                // if this is first fucntion call with pseudo generator
                // we collect data

                // if object is enabled
                if (rmgObjInfo.enabled && ObjectMayBeGenerated(rmgObjGen))
                {

                    if (rmgObjInfo.density > 0)
                        rmgObjGen->density = rmgObjInfo.density;
                    if (rmgObjInfo.value != -1)
                        rmgObjGen->value = rmgObjInfo.value;

                    // add into list generated list
                    editor.editedRMGObjectGenerators.Add(rmgObjGen);
                }
            }

            // after list is created

            // swap vectors between each other to use edited values
            std::swap(rmgStruct->objectGenerators, editor.editedRMGObjectGenerators);
        }
    }
}

_LHF_(RMGObjectsEditor::RMG__ZoneGeneration__AfterObjectTypeZoneLimitCheck)
{

    // check if allowed generate that type by zone/map
    if (c->flags.SF != c->flags.OF)
    {
        if (auto *objGen = reinterpret_cast<H3RmgObjectGenerator *>(c->ecx))
        {

            // zone id where assumed to generate that object
            const int zoneId = IntAt(c->ebp - 0x34);

            // check if numer of placed this object'type/subtypes on the whole map is more than allowed
            if (generatedInfo.ObjectCantBeGenerated(objGen, zoneId))
            {
                // set flag that limit is exceeded
                c->flags.SF = c->flags.OF;
            }
        }
    }

    return EXEC_DEFAULT;
}

// Placed objects counter
_LHF_(RMGObjectsEditor::RMG__RMGObject_AtPlacement)
{
    if (generatedInfo.Inited())
    {
        // Get generated and placed RMG object from stack
        const H3RmgObject *obj = *reinterpret_cast<H3RmgObject **>(c->ebp + 0x8);
        // increase counter
        generatedInfo.IncreaseObjectsCounters(obj->properties->prototype, c->ecx);
    }

    return EXEC_DEFAULT;
}

int __stdcall RMG__RMGDwellingObject_AtGettingValue(HiHook *h, const H3RmgObjectGenerator *objGen,
                                                    const H3RmgZoneGenerator *zoneGen,
                                                    const H3RmgRandomMapGenerator *rmg)
{

    if (objGen->type == eObject::CREATURE_GENERATOR4)
    {
        // return -1; // resultValue / 2;

        const DWORD dwellings4Ptr = DwordAt(0x04B85B5 + 2);

        const int creatureValue = RMGObjectInfo::CurrentObjectInfo(objGen->type, objGen->subtype).value;
        int resultValue = -1;
        const int totalTownsCount = rmg->townsCount;
        for (size_t i = 0; i < 4; i++)
        {
            const int creatureType = DwordAt(dwellings4Ptr + (i << 2));

            if (creatureType != eCreature::UNDEFINED)
            {
                auto &info = P_CreatureInformation[creatureType];
                const int creatureTown = info.town;
                if (creatureTown != zoneGen->townType2)
                {
                    continue;
                }
                if (const int aiValue = info.aiValue)
                {
                    int dwellingSlotValue = aiValue * info.grow;

                    if (totalTownsCount && creatureTown != eTown::NEUTRAL)
                    {

                        if (const int totalCreatureTypeTowns = rmg->townsCountByType[creatureTown])
                        {
                            dwellingSlotValue += dwellingSlotValue * totalCreatureTypeTowns / totalTownsCount;
                        }
                    }
                    resultValue += dwellingSlotValue + (totalTownsCount * aiValue >> 1);
                }
            }
        }

        return resultValue >> 2; // resultValue / 2;
    }

    const DWORD dwellingsPtr = DwordAt(0x534CE7 + 3);
    const int creatureType = DwordAt(dwellingsPtr + 4 * objGen->subtype);

    const int storedCreatureAIValue = P_CreatureInformation[creatureType].aiValue;

    P_CreatureInformation[creatureType].aiValue = RMGObjectInfo::CurrentObjectInfo(objGen->type, objGen->subtype).value;

    const int objectValue = THISCALL_3(int, h->GetDefaultFunc(), objGen, zoneGen, rmg);

    P_CreatureInformation[creatureType].aiValue = storedCreatureAIValue;

    return objectValue;
}

void __stdcall RMGObjectsEditor::RMG__InitGenZones(HiHook *h, const H3RmgRandomMapGenerator *rmg,
                                                   const H3RmgTemplate *RmgTemplate)
{

#ifdef _DEBUG
    constexpr int TEST_SEED = 23432434;
    srand(GetTime());
    const unsigned char objectGenAttemps = rand() % 100 + 3;
    ByteAt(0x0546A6F + 2) = objectGenAttemps;
    H3Messagebox(Era::IntToStr(objectGenAttemps).c_str());

    const_cast<H3RmgRandomMapGenerator *>(rmg)->randomSeed = TEST_SEED;
    H3Random::SetRandomSeed(rmg->randomSeed);
    CDECL_1(void, 0x61841F, TEST_SEED);
#endif // _DEBUG

    THISCALL_2(void, h->GetDefaultFunc(), rmg, RmgTemplate);

    // auto var = RMGObjectInfo::currentRMGObjectsInfoByType;
    // create limits counters
    generatedInfo.Assign(rmg, RMGObjectInfo::CurrentObjectInfo());
}

void __stdcall RMGObjectsEditor::RMG__AfterMapGenerated(HiHook *h, H3RmgRandomMapGenerator *rmgStruct)
{
    // swap generators list back
    if (generatedInfo.Inited())
    {
        generatedInfo.Clear(rmgStruct);
    }
    auto &editedGeneratorsList = Get().editedRMGObjectGenerators;

    // swap rmgGenerators list back
    std::swap(rmgStruct->objectGenerators, editedGeneratorsList);
    editedGeneratorsList.RemoveAll();

    //  clear filled data;
    THISCALL_1(void, h->GetDefaultFunc(), rmgStruct);
}

//

void RMGObjectsEditor::CreatePatches()
{

    // m_isInited = true;
    // Restrain Armour damage reduction at 96% before level 868
    if (!m_isInited)
    {
        // hook used to edit generate objects list data
        _PI->WriteHiHook(0x539000, THISCALL_, RMG__CreateObjectGenerators);

        // hook is used to block object generation
        _pi->WriteLoHook(0x54676B, RMG__ZoneGeneration__AfterObjectTypeZoneLimitCheck);

        _pi->WriteHiHook(0x549FCE, THISCALL_, RMG__InitGenZones);
        _pi->WriteHiHook(0x5382E0, THISCALL_, RMG__AfterMapGenerated);

        _pi->WriteLoHook(0x540881, RMG__RMGObject_AtPlacement);
        _pi->WriteHiHook(0x0534CE0, THISCALL_, RMG__RMGDwellingObject_AtGettingValue);
        // Hook for the setting defaults

        m_isInited = true;
    }
}

} // namespace editor

inline int index3D(const int zoneId, const int objType, const int objSubtype, const int typesNum, const int lineSize)
{
    return zoneId * typesNum * lineSize + objType * lineSize + objSubtype;
}
inline int index2D(const int objType, const int objSubtype, const int lineSize)
{
    return objType * lineSize + objSubtype;
}

void GeneratedInfo::IncreaseObjectsCounters(const H3RmgObjectProperties *prop, const int zoneId)
{
    // increment number of zone generated subtypes of that obj type
    const int index3 = index3D(zoneId, prop->type, prop->subtype, H3_MAX_OBJECTS, maxObjectSubtype);
    eachZoneGeneratedBySubtype[index3]++;

    // increment number of map generated subtypes of that obj type
    const int index2 = index2D(prop->type, prop->subtype, maxObjectSubtype);
    mapGeneratedBySubtype[index2]++;
}

BOOL RMGObjectInfo::Clamp() noexcept
{
    BOOL dataChanged = false;
    // return dataChanged;

    const int globalMapLimit = editor::RMGObjectsEditor::Get().MaxMapTypeLimit(type);

    if (mapLimit > globalMapLimit)
    {
        mapLimit = globalMapLimit;
        dataChanged = true;
    }

    if (zoneLimit > mapLimit)
    {
        zoneLimit = mapLimit;
        dataChanged = true;
    }
    if (!zoneLimit || !mapLimit)
    {
        zoneLimit = mapLimit = 0;
        enabled = false;
        dataChanged = true;
    }
    if (density < -1)
    {
        density = -1;
        // sot we disable object that case

        enabled = false;
        dataChanged = true;
    }
    else if (density == 0) // it will crash the game otherwise

    {
        enabled = false;
        dataChanged = true;
    }

    if (value < -1)
    {
        value = -1;
        dataChanged = true;
    }

    // enabled= value > 0 && density > 0 && mapLimit > 0 && zoneLimit > 0;
    return dataChanged;
}

void RMGObjectInfo::RestoreDefault() noexcept
{
    if (type != eObject::NO_OBJ && subtype != eObject::NO_OBJ)
    {
        *this = defaultRMGObjectsInfoByType[type][subtype];
    }
}

void RMGObjectInfo::SetRandom() noexcept
{
    RMGObjectInfo tempObjInfo = *this;
    tempObjInfo.RestoreDefault();

    for (size_t i = 0; i < 5; i++)
    {
        if (tempObjInfo.data[i] < 2)
        {
            tempObjInfo.data[i] = 2;
        }
        //	temp.data[i] = rand() % temp.data[i];
    }
    tempObjInfo.enabled = rand() % 2;

    for (size_t i = 1; i < 3; i++)
    {
        if (tempObjInfo.data[i] > 1)
        {
            tempObjInfo.data[i] = tempObjInfo.data[i] > 100 ? rand() % 100 / i : rand() % tempObjInfo.data[i];
        }
        else
        {
            tempObjInfo.data[i] = rand() % 2;
        }
        // tempObjInfo.data[i] <<= i;
    }

    if (tempObjInfo.value > 0)
    {
        if (int t = rand() % 15)
        {
            tempObjInfo.value *= t;
            tempObjInfo.value >>= rand() % 5;
        }
    }
    if (tempObjInfo.density > 0)
    {
        if (int t = rand() % 15)
        {
            tempObjInfo.density *= t;
            tempObjInfo.density >>= rand() % 5;
        }
    }

    tempObjInfo.Clamp();

    *this = tempObjInfo;
}
void RMGObjectInfo::MakeReal() const noexcept
{
    if (type != eObject::NO_OBJ && subtype != eObject::NO_OBJ)
    {
        currentRMGObjectsInfoByType[type][subtype] = *this;
    }
}

RMGObjectInfo::RMGObjectInfo(const INT32 type, const INT32 subtype) : type(type), subtype(subtype)
{
    enabled = true;
}

RMGObjectInfo::RMGObjectInfo() : RMGObjectInfo(eObject::NO_OBJ, eObject::NO_OBJ)
{
}
LPCSTR RMGObjectInfo::GetName() const noexcept
{
    return GetObjectName(type, subtype);
}
BOOL RMGObjectInfo::WriteToINI() const noexcept
{
    BOOL success = true;
    constexpr int zoneType = 0;
    H3String sectionName = H3String::Format(OBJECT_INFO_INI_FORMAT, type, subtype, zoneType);
    // save changed settings only
    for (size_t i = 0; i < SIZE; i++)
    {
        if (defaultRMGObjectsInfoByType[type][subtype].data[i] != data[i])
        {
            if (!Era::WriteStrToIni(PROPERTY_NAMES[i], std::to_string(data[i]).c_str(), sectionName.String(),
                                    INI_FILE_PATH))
                success = false;
        }
    }

    return success;
}

inline void RMGObjectInfo::ReadFromINI() noexcept
{

    constexpr int zoneType = 0;

    H3String sectionName = H3String::Format(OBJECT_INFO_INI_FORMAT, type, subtype, zoneType);

    for (size_t i = 0; i < SIZE; i++)
    {
        if (Era::ReadStrFromIni(PROPERTY_NAMES[i], sectionName.String(), INI_FILE_PATH, h3_TextBuffer))
        {
            const int iniValue = atoi(h3_TextBuffer);
            if (data[i] != iniValue)
            {
                data[i] = iniValue;
            }
        }
    }
}
inline const RMGObjectInfo &RMGObjectInfo::DefaultObjectInfo(const int objType, const int subtype) noexcept
{
    return defaultRMGObjectsInfoByType[objType][subtype];
}
inline const RMGObjectInfo &RMGObjectInfo::CurrentObjectInfo(const int objType, const int subtype) noexcept
{
    return currentRMGObjectsInfoByType[objType][subtype];
}

inline const std::vector<RMGObjectInfo> (&RMGObjectInfo::CurrentObjectInfo())[limits::OBJECTS]
{

    return RMGObjectInfo::currentRMGObjectsInfoByType;
}

void RMGObjectInfo::InitFromRmgObjectGenerator(const H3RmgObjectGenerator &generator)
{

    const int objType = generator.type;
    const int objSubtype = generator.subtype;
    if (objSubtype >= defaultRMGObjectsInfoByType[objType].size())
    {
        defaultRMGObjectsInfoByType[objType].resize(objSubtype + 1);
        defaultRMGObjectsInfoByType[objType][objSubtype] = {objType, objSubtype};
    }
    auto &rmgObjInfo = defaultRMGObjectsInfoByType[objType][objSubtype];
    // if this is first fucntion call with pseudo generator
    // we collect data

    constexpr int UNDEFINED = -1;

    // only if data isn't taken from json before
    if (rmgObjInfo.density == UNDEFINED)
    {
        rmgObjInfo.density = generator.density;
    }

    if (rmgObjInfo.value == UNDEFINED)
    {
        rmgObjInfo.value = generator.value;
    }

    rmgObjInfo.Clamp();
}
LPCSTR RMGObjectInfo::GetObjectName(const H3MapItem *mapItem)
{
    if (mapItem)
    {
        return GetObjectName(mapItem->objectType, mapItem->objectSubtype);
    }
    return h3_NullString;
}

void RMGObjectInfo::InitDefaultProperties(const ObjectLimitsInfo &limitsInfo, const INT16 *maxSubtypes)
{
    RMGObjectInfo *objInfo = nullptr;
    bool readSucces = false;

    constexpr int INFO_PROPERTIES_NUMBER = 5;

    constexpr int UNDEFINED = -1;

    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    {
        const int maxSubtype = maxSubtypes[objType];
        defaultRMGObjectsInfoByType[objType].resize(maxSubtype);

        int typeData[5] = {true, limitsInfo.mapTypesLimit[objType], limitsInfo.zoneTypeLimits[objType], UNDEFINED,
                           UNDEFINED};

        for (size_t keyIndex = 0; keyIndex < INFO_PROPERTIES_NUMBER; keyIndex++)
        {

            // read default data from json for objTypes only
            const int data = EraJS::readInt(
                H3String::Format(OBJECT_TYPE_PROPERTY_JSON_KEY_FORMAT, objType, PROPERTY_NAMES[keyIndex]).String(),
                readSucces);

            // and if succes
            if (readSucces)
            {
                // put into temp types data array
                typeData[keyIndex] = data;
            }
        }

        // now iterate all the subtpyes of that type
        for (size_t objSubtype = 0; objSubtype < maxSubtype; objSubtype++)
        {
            // get RmgObjectInfo ptr
            objInfo = &defaultRMGObjectsInfoByType[objType][objSubtype];

            // assign IDs to the type and subtype
            objInfo->type = objType;
            objInfo->subtype = objSubtype;

            for (size_t keyIndex = 0; keyIndex < INFO_PROPERTIES_NUMBER; keyIndex++)
            {
                // read default data from json for the exact subtype
                const int subtypeData = EraJS::readInt(H3String::Format(OBJECT_SUBTYPE_PROPERTY_JSON_KEY_FORMAT,
                                                                        objType, objSubtype, PROPERTY_NAMES[keyIndex])
                                                           .String(),
                                                       readSucces);

                // set data according to struct offset
                objInfo->data[keyIndex] = readSucces ? subtypeData : typeData[keyIndex];
            }
        }
    }

    // dwellings value calculation
    const DWORD dwellings1Ptr = DwordAt(0x534CE7 + 3);
    for (auto &dwellingObjInfo : defaultRMGObjectsInfoByType[eObject::CREATURE_GENERATOR1])
    {
        const int dwellingCreatureType = DwordAt(dwellings1Ptr + (dwellingObjInfo.subtype << 2));
        const int creatureAIValue = P_CreatureInformation[dwellingCreatureType].aiValue;
        dwellingObjInfo.value = creatureAIValue;
    }

    const DWORD dwellings4Ptr = DwordAt(0x04B85B5 + 2);

    for (auto &dwellingObjInfo : defaultRMGObjectsInfoByType[eObject::CREATURE_GENERATOR4])
    {
        dwellingObjInfo.value = 0; // creatureAIValue;

        for (size_t i = 0; i < 4; i++)
        {
            const int dwellingCreatureType = DwordAt(dwellings4Ptr + (i << 2));
            if (dwellingCreatureType != eCreature::UNDEFINED)
            {
                dwellingObjInfo.value += P_CreatureInformation[dwellingCreatureType].aiValue;
            }
        }
    }
}

void RMGObjectInfo::LoadUserProperties(const INT16 *maxSubtypes)
{
    // copy default objects to the current one
    for (auto i = 0; i < h3::limits::OBJECTS; i++)
    {
        currentRMGObjectsInfoByType[i] = defaultRMGObjectsInfoByType[i]; // curentSettings;
    }

    constexpr int zoneType = 0;
    constexpr int SIZE = 5;

    for (auto &objInfoVec : currentRMGObjectsInfoByType)
    {
        for (auto &objectInfo : objInfoVec)
        {

            objectInfo.ReadFromINI();
            objectInfo.Clamp();
        }
    }
    Era::ClearIniCache(INI_FILE_PATH);
}

LPCSTR RMGObjectInfo::GetObjectName(const INT32 type, const INT32 subtype)
{
    LPCSTR result = h3_NullString;

    const int creatureBankId = cbanks::CreatureBanksExtender::GetCreatureBankId(type, subtype);
    if (creatureBankId >= 0)
    {
        return H3CreatureBankSetup::Get()[creatureBankId].name.String();
    }

    switch (type)
    {
    case eObject::ARTIFACT:
        result = P_Artifacts[subtype].name;
        break;
    case eObject::CREATURE_GENERATOR1:
        result = P_DwellingNames1[subtype];
        break;
    case eObject::CREATURE_GENERATOR4:
        result = P_DwellingNames4[subtype];
        break;
    case eObject::MINE:
        result = P_MineNames[subtype];
        break;
    case eObject::MONSTER:
    case eObject::RANDOM_MONSTER:
        result = P_Creatures[subtype].namePlural;
    case eObject::PYRAMID:
    case warehouses::WAREHOUSE_OBJECT_TYPE:
    case extender::HOTA_OBJECT_TYPE:
    case extender::HOTA_PICKUPABLE_OBJECT_TYPE:
    case 146:
        libc::sprintf(h3_TextBuffer, OBJECT_SUBTYPE_NAME_JSON_KEY_FORMAT, type, subtype);
        result = EraJS::read(h3_TextBuffer);
        break;
    case eObject::RESOURCE:
        result = P_ResourceName[subtype];
        break;
    case eObject::SHRINE_OF_MAGIC_INCANTATION:
        if (subtype)
        {
            libc::sprintf(h3_TextBuffer, OBJECT_SUBTYPE_NAME_JSON_KEY_FORMAT, type, subtype);
            result = EraJS::read(h3_TextBuffer);
            break;
        }
    case eObject::TOWN:
        result = P_TownNames[subtype];
        break;
    default:
        result = P_ObjectName[type];
        break;
    }

    return result;
}

int *create3DArray(int X, int Y, int Z)
{
    return new int[X * Y * Z];
}

int *create2DArray(int X, int Y)
{
    return new int[X * Y];
}

void GeneratedInfo::Assign(const H3RmgRandomMapGenerator *rmg,
                           const std::vector<RMGObjectInfo> (&userRmgInfoSet)[h3::limits::OBJECTS])
{

    const UINT zonesNum = rmg->zoneGenerators.Size();

    if (zonesNum)
    {
        maxObjectSubtype = 0;
        // create max subtype value for all object gens
        INT16 maxSubtypes[H3_MAX_OBJECTS] = {};
        // and init as -1 like if there is no objects
        // memset(maxSubtypes, -1, sizeof(maxSubtypes));

        for (auto *p_ObjGen : rmg->objectGenerators)
        {
            // each object gen of that type has hihgher subptype
            if (p_ObjGen->subtype >= maxSubtypes[p_ObjGen->type])
            {
                // assume that object t/s must be max limit array m_size
                maxSubtypes[p_ObjGen->type] = p_ObjGen->subtype + 1;
            }
            if (p_ObjGen->subtype > maxObjectSubtype)
            {
                maxObjectSubtype = p_ObjGen->subtype + 1;
            }
        }

        // create conters and limits
        eachZoneGeneratedBySubtype = create3DArray(zonesNum, H3_MAX_OBJECTS, maxObjectSubtype);
        mapGeneratedBySubtype = create2DArray(H3_MAX_OBJECTS, maxObjectSubtype);
        zoneLimitsBySubtype = create2DArray(H3_MAX_OBJECTS, maxObjectSubtype);
        mapLimitsBySubtype = create2DArray(H3_MAX_OBJECTS, maxObjectSubtype);

        const int arraylength = H3_MAX_OBJECTS * maxObjectSubtype * sizeof(int);

        memset(eachZoneGeneratedBySubtype, 0, zonesNum * arraylength);
        memset(mapGeneratedBySubtype, 0, arraylength);
        memset(zoneLimitsBySubtype, 0, arraylength);
        memset(mapLimitsBySubtype, 0, arraylength);

        // assign info from userData
        for (const auto &vec : userRmgInfoSet)
        {
            for (const auto &info : vec)
            {
                const int index2 = index2D(info.type, info.subtype, maxObjectSubtype);
                mapLimitsBySubtype[index2] = info.mapLimit;
                zoneLimitsBySubtype[index2] = info.zoneLimit;
            }
        }

        isInited = true;
    }
}

BOOL GeneratedInfo::ObjectCantBeGenerated(const H3RmgObjectGenerator *objGen, const int zoneId) const
{
    // increment number of map generated subtypes of that obj type
    const int index2 = index2D(objGen->type, objGen->subtype, maxObjectSubtype);
    // increment number of zone generated subtypes of that obj type
    const int index3 = index3D(zoneId, objGen->type, objGen->subtype, H3_MAX_OBJECTS, maxObjectSubtype);
    // return with that simple comparison function
    return mapGeneratedBySubtype[index2] >= mapLimitsBySubtype[index2]
               ? true
               : eachZoneGeneratedBySubtype[index3] >= zoneLimitsBySubtype[index2];
}
void GeneratedInfo::Clear(const H3RmgRandomMapGenerator *rmgStruct)
{
    // delete all the allocated arrays
    if (isInited)
    {
        for (auto arr : arrays)
        {
            if (arr)
            {
                delete[] arr;
                arr = nullptr;
            }
        }
        isInited = false;
        maxObjectSubtype = 0;
    }
}

BOOL GeneratedInfo::Inited() const noexcept
{
    return isInited;
}
