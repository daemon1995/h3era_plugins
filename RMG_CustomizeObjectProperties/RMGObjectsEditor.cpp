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

    // init global defaults for all objects
    bool readSucces = false;
    const UINT mapGlobalLimitDefault = EraJS::readInt("RMG.objectGeneration.map", readSucces);
    if (readSucces)
    {
        std::fill(std::begin(limitsInfo.mapTypesLimit), std::end(limitsInfo.mapTypesLimit), mapGlobalLimitDefault);
    }

    const UINT zoneGlobalLimitDefault = EraJS::readInt("RMG.objectGeneration.zone", readSucces);
    if (readSucces)
    {
        std::fill(std::begin(limitsInfo.zoneTypeLimits), std::end(limitsInfo.zoneTypeLimits), zoneGlobalLimitDefault);
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

        // add scrolls level 6
        if (H3RmgObjectGenerator *objScrollGen = H3ObjectAllocator<_RMGObjGenScroll_>().allocate(1))
        {
            THISCALL_3(H3RmgObjectGenerator *, 0x0535390, objScrollGen, _RMGObjGenScroll_::MAP_CONTROL_SPELL_LEVEL,
                       _RMGObjGenScroll_::MAP_CONTROL_SPELL_VALUE);
            rmgObjectsList->Push(objScrollGen);
        }

        auto &editor = RMGObjectsEditor::Get();
        editor.SetMapControlSpellLevels(true);

        // edit current objects by type/subtype
        if (editor.isPseudoGeneration)
        {

            for (auto &rmgObjGen : *rmgObjectsList)
            {
                if (rmgObjGen->type == eObject::SPELL_SCROLL)
                {
                    const int spellLevel = reinterpret_cast<const _RMGObjGenScroll_ *>(rmgObjGen)->spellLevel;
                    if (spellLevel < 7)
                        rmgObjGen->subtype = reinterpret_cast<const _RMGObjGenScroll_ *>(rmgObjGen)->spellLevel;
                }

                RMGObjectInfo::InitFromRmgObjectGenerator(*rmgObjGen);
            }
        }
        else
        {
            // if map generation we affect the reall array with new data

            // create vector with assumed to create objects ;
            editor.editedRMGObjectGenerators.RemoveAll();

            // check if we have full random
            const BOOL result = Era::ReadStrFromIni(rmgdlg::RMG_SettingsDlg::INI_ALWAYS_RANDOM,
                                                    rmgdlg::RMG_SettingsDlg::SETTINGS_INI_SECTION,
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

            // init variables;
            for (auto &rmgObjGen : *rmgObjectsList)
            {
                // skip changes for some objects to not break native map generation
                switch (rmgObjGen->type)
                {
                    // add these objects w/o any restrictions
                case eObject::PANDORAS_BOX:
                case eObject::KEYMASTER:
                case eObject::PRISON:
                case eObject::SEER_HUT:

                    editor.editedRMGObjectGenerators.Add(rmgObjGen);
                    continue;

                case eObject::SPELL_SCROLL:

                    rmgObjGen->subtype = reinterpret_cast<const _RMGObjGenScroll_ *>(rmgObjGen)->spellLevel;
                    break;

                default:
                    break;
                }

                const RMGObjectInfo &rmgObjInfo = RMGObjectInfo::CurrentObjectInfo(rmgObjGen->type, rmgObjGen->subtype);

                // const auto &rmgObjInfo = RMGObjectInfo::CurrentObjectInfo(rmgObjGen->type, rmgObjGen->subtype);
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
        editor.SetMapControlSpellLevels(false);
    }
}
void RMGObjectsEditor::SetMapControlSpellLevels(const BOOL state, const BOOL blockWaterSpells) noexcept
{
    if (state)
    {
        spellLvls[eSpell::SUMMON_BOAT] = P_Spell[eSpell::SUMMON_BOAT].level;
        spellLvls[eSpell::SCUTTLE_BOAT] = P_Spell[eSpell::SCUTTLE_BOAT].level;

        spellLvls[eSpell::FLY] = P_Spell[eSpell::FLY].level;
        spellLvls[eSpell::WATER_WALK] = P_Spell[eSpell::WATER_WALK].level;
        spellLvls[eSpell::DIMENSION_DOOR] = P_Spell[eSpell::DIMENSION_DOOR].level;
        spellLvls[eSpell::TOWN_PORTAL] = P_Spell[eSpell::TOWN_PORTAL].level;

        P_Spell[eSpell::FLY].level = _RMGObjGenScroll_::MAP_CONTROL_SPELL_LEVEL;
        P_Spell[eSpell::WATER_WALK].level = _RMGObjGenScroll_::MAP_CONTROL_SPELL_LEVEL;
        P_Spell[eSpell::DIMENSION_DOOR].level = _RMGObjGenScroll_::MAP_CONTROL_SPELL_LEVEL;
        P_Spell[eSpell::TOWN_PORTAL].level = _RMGObjGenScroll_::MAP_CONTROL_SPELL_LEVEL;
        if (blockWaterSpells)
        {
            P_Spell[eSpell::SUMMON_BOAT].level = _RMGObjGenScroll_::BANNED_SPELL_LEVEL;
            P_Spell[eSpell::SCUTTLE_BOAT].level = _RMGObjGenScroll_::BANNED_SPELL_LEVEL;
            P_Spell[eSpell::WATER_WALK].level = _RMGObjGenScroll_::BANNED_SPELL_LEVEL;
        }
    }
    else
    {
        P_Spell[eSpell::SUMMON_BOAT].level = spellLvls[eSpell::SUMMON_BOAT];
        P_Spell[eSpell::SCUTTLE_BOAT].level = spellLvls[eSpell::SCUTTLE_BOAT];

        P_Spell[eSpell::FLY].level = spellLvls[eSpell::FLY];
        P_Spell[eSpell::WATER_WALK].level = spellLvls[eSpell::WATER_WALK];
        P_Spell[eSpell::DIMENSION_DOOR].level = spellLvls[eSpell::DIMENSION_DOOR];
        P_Spell[eSpell::TOWN_PORTAL].level = spellLvls[eSpell::TOWN_PORTAL];
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
            if (objGen->type == eObject::SPELL_SCROLL)
            {
                objGen->subtype = reinterpret_cast<_RMGObjGenScroll_ *>(objGen)->spellLevel;
                if (objGen->subtype > 6)
                {
                    return EXEC_DEFAULT;
                }
            }
            if (generatedInfo.ObjectCantBeGenerated(objGen, zoneId))
            {

                // set flag that limit is exceeded
                c->flags.SF = c->flags.OF;
            }
            if (objGen->type == eObject::SPELL_SCROLL)
            {
                // reinterpret_cast<_RMGObjGenScroll_ *>(objGen)->spellLevel = objGen->subtype;
                objGen->subtype = 0;
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
        const H3RmgObject *rmgObject = *reinterpret_cast<H3RmgObject **>(c->ebp + 0x8);
        // increase counter
        auto &prototype = rmgObject->properties->prototype;
        switch (prototype->type)
        {
        case eObject::SPELL_SCROLL:

            if (generatedInfo.lastGeneratedSpellScroll)
            {
                const int storedObjectSubtype = prototype->subtype;
                if (storedObjectSubtype < 7)
                {
                    prototype->subtype = generatedInfo.lastGeneratedSpellScroll->spellLevel;
                    generatedInfo.IncreaseObjectsCounters(prototype, c->ecx);
                    prototype->subtype = storedObjectSubtype;
                }

                generatedInfo.lastGeneratedSpellScroll = nullptr;
            }

            break;

        default:
            generatedInfo.IncreaseObjectsCounters(prototype, c->ecx);

            break;
        }
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
            const int creatureType = DwordAt(dwellings4Ptr + (i << 2) + 16 * objGen->subtype);

            if (creatureType != eCreature::UNDEFINED)
            {
                auto &info = P_CreatureInformation[creatureType];
                const int creatureTown = info.town;
                if (creatureTown != zoneGen->townType2 && i == 0)
                {
                    if (i == 0)
                    {
                        return resultValue;
                    }
                    else
                    {
                        continue;
                    }
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
                    resultValue += dwellingSlotValue;

                    if (creatureTown != eTown::NEUTRAL)
                    {
                        resultValue += totalTownsCount * aiValue >> 1;
                    }
                }
            }
        }

        return resultValue;
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
    // srand(GetTime());
    // const unsigned char objectGenAttemps = rand() % 100 + 3;
    // ByteAt(0x0546A6F + 2) = objectGenAttemps;
    // H3Messagebox(Era::IntToStr(objectGenAttemps).c_str());

    const_cast<H3RmgRandomMapGenerator *>(rmg)->randomSeed = TEST_SEED;
    H3Random::SetRandomSeed(rmg->randomSeed);
    CDECL_1(void, 0x61841F, TEST_SEED);
#endif // _DEBUG

    THISCALL_2(void, h->GetDefaultFunc(), rmg, RmgTemplate);

    Get().BeforeMapGeneration(rmg);
}

void RMGObjectsEditor::BeforeMapGeneration(const H3RmgRandomMapGenerator *rmgStruct)
{
    // create limits counters
    generatedInfo.Assign(rmgStruct, RMGObjectInfo::CurrentObjectInfo());

    // _PI->WriteDword(0x0541013,)
    // static BOOL firstRun = true;
    // if (firstRun)
    //{
    //    constexpr int ADDED_MONSTERS_NUM = 29;
    //    DwordAt(0x0541013 + 2) += ADDED_MONSTERS_NUM * 4;
    //    DwordAt(0x0541063 + 1) += ADDED_MONSTERS_NUM;
    //    DwordAt(0x05410B3 + 1) += ADDED_MONSTERS_NUM;
    //    DwordAt(0x0541159 + 1) += ADDED_MONSTERS_NUM;

    //    firstRun = false;
    //}
    SetMapControlSpellLevels(true, !(rmgStruct->waterAmount));

    // store spell levels
    // change guard ai values
    auto &monstersInfo = RMGObjectInfo::currentRMGObjectsInfoByType[eObject::MONSTER];
    for (auto &info : monstersInfo)
    {
        if (info.value != RMGObjectInfo::UNDEFINED)
        {
            std::swap(info.value, P_CreatureInformation[info.subtype].aiValue);
        }
    }
}
void __stdcall RMGObjectsEditor::RMG__AfterMapGenerated(HiHook *h, H3RmgRandomMapGenerator *rmgStruct)
{

    Get().AfterMapGeneration(rmgStruct);
    //  clear filled data;
    THISCALL_1(void, h->GetDefaultFunc(), rmgStruct);
}
void RMGObjectsEditor::AfterMapGeneration(H3RmgRandomMapGenerator *rmgStruct) noexcept
{

    // swap generators list back
    if (generatedInfo.Inited())
    {
        generatedInfo.Clear(rmgStruct);
    }

    // swap rmgGenerators list back
    std::swap(rmgStruct->objectGenerators, editedRMGObjectGenerators);
    editedRMGObjectGenerators.RemoveAll();

    // restore changed spell levels
    SetMapControlSpellLevels(false);

    auto &monstersInfo = RMGObjectInfo::currentRMGObjectsInfoByType[eObject::MONSTER];
    for (auto &info : monstersInfo)
    {
        if (info.value != RMGObjectInfo::UNDEFINED)
        {
            std::swap(info.value, P_CreatureInformation[info.subtype].aiValue);
        }
    }
}

// set virtual object subtype to spell level to increase counters
H3RmgObject *__stdcall RMGObjectsEditor::RMG__RMGObjGenScroll__CreateObject(HiHook *h, _RMGObjGenScroll_ *scrollGen,
                                                                            H3RmgObjectPropsRef *ref,
                                                                            H3RmgRandomMapGenerator *rmg,
                                                                            H3RmgZoneGenerator *zoneGen) noexcept

{

    H3RmgObject *rmgObject = THISCALL_4(H3RmgObject *, h->GetDefaultFunc(), scrollGen, ref, rmg, zoneGen);
    if (rmgObject)
    {
        generatedInfo.lastGeneratedSpellScroll = scrollGen;

        // auto &prototype = rmgObject->properties->prototype;
        // const int storedObjectSubtype = prototype->subtype;
        // prototype->subtype = scrollGen->spellLevel;
        //// generatedInfo.IncreaseObjectsCounters(rmgObject->properties->prototype, zoneGen->zoneInfo->id);
        // prototype->subtype = storedObjectSubtype;
    }
    return rmgObject;
}
// fixes for different RMG bugs/stuff
namespace fixes
{
//
_LHF_(RMG__AtSubterranianGatesPrototypeGet)
{

    if (const auto genZone = *reinterpret_cast<H3RmgZoneGenerator **>(c->ebp + 0x8))
    {
        // _RMGObjectPrototypeRef_ *__thiscall RMG_FindObjectPrototype(_RMGStruct_ *this, int ground, eObject type,
        // int subtype)
        const DWORD correctObjectPrototypeRef =
            THISCALL_4(DWORD, 0x546530, c->edi, genZone->ground, eObject::SUBTERRANEAN_GATE, 0);
        if (correctObjectPrototypeRef)
        {
            c->eax = correctObjectPrototypeRef;
            // skip random gate property selection
            c->return_address = 0x005426BE;
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

DWORD correctObjectPrototypeRef = 0;
_LHF_(RMG__AtSecondSubterranianGatesPositioning)

{
    correctObjectPrototypeRef = 0;
    if (const auto genZone = *reinterpret_cast<H3RmgZoneGenerator **>(c->ebp - 0x10))
    {
        // _RMGObjectPrototypeRef_ *__thiscall RMG_FindObjectPrototype(_RMGStruct_ *this, int ground, eObject type,
        // int subtype)
        correctObjectPrototypeRef =
            THISCALL_4(DWORD, 0x546530, IntAt(c->ebp - 0x14), genZone->ground, eObject::SUBTERRANEAN_GATE, 0);

        if (correctObjectPrototypeRef)
        {
            // remove original property
            c->Pop();
            c->Push(correctObjectPrototypeRef);
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(RMG__AtSecondSubterranianGatesPlacement)
{
    if (correctObjectPrototypeRef)
    {
        c->ecx = correctObjectPrototypeRef;
        correctObjectPrototypeRef = 0;
    }
    return EXEC_DEFAULT;
}

} // namespace fixes

void RMGObjectsEditor::CreatePatches()
{

    // m_isInited = true;
    // Restrain Armour damage reduction at 96% before level 868
    if (!m_isInited)
    {
        // hook used to edit generate objects list data
        _pi->WriteHiHook(0x539000, THISCALL_, RMG__CreateObjectGenerators);
        _pi->WriteHiHook(0x549FCE, THISCALL_, RMG__InitGenZones);

        // hook is used to block object generation
        _pi->WriteLoHook(0x54676B, RMG__ZoneGeneration__AfterObjectTypeZoneLimitCheck);
        _pi->WriteHiHook(0x0534CE0, THISCALL_, RMG__RMGDwellingObject_AtGettingValue);

        _pi->WriteHiHook(0x05353C0, THISCALL_, RMG__RMGObjGenScroll__CreateObject);
        _pi->WriteLoHook(0x540881, RMG__RMGObject_AtPlacement);

        _pi->WriteHiHook(0x5382E0, THISCALL_, RMG__AfterMapGenerated);

        // fixes for subterranian gates
        using namespace fixes;
        // skips random selsection of the gates subtype
        _pi->WriteLoHook(0x05426B5, RMG__AtSubterranianGatesPrototypeGet);

        // creates correct gates prototype corresponding to the ground of the 2nd subterranian gate
        _pi->WriteLoHook(0x05427DA, RMG__AtSecondSubterranianGatesPositioning);
        _pi->WriteLoHook(0x0542937, RMG__AtSecondSubterranianGatesPlacement);

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

LPCSTR RMGObjectInfo::GetRmgTypeDescription() const noexcept
{
    return EraJS::read(H3String::Format("RMG.objectGeneration.%d.text.rmg", type).String());
}
LPCSTR RMGObjectInfo::GetRmgSubtypeDescription() const noexcept
{
    return EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.rmg", type, subtype).String());
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

void RMGObjectInfo::InitDefaultProperties(const ObjectLimitsInfo &limitsInfo, const INT16 *maxSubtypes)
{
    RMGObjectInfo *objInfo = nullptr;
    bool readSucces = false;

    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    {
        const int maxSubtype = maxSubtypes[objType];
        defaultRMGObjectsInfoByType[objType].resize(maxSubtype);

        int typeData[5] = {true, limitsInfo.mapTypesLimit[objType], limitsInfo.zoneTypeLimits[objType], UNDEFINED,
                           UNDEFINED};

        for (size_t keyIndex = 0; keyIndex < SIZE; keyIndex++)
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

            for (size_t keyIndex = 0; keyIndex < SIZE; keyIndex++)
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

    // custom data for scrolls
    auto &ref = defaultRMGObjectsInfoByType[eObject::SPELL_SCROLL];

    const int isize = ref.size();
    isize;
    // dwellings value calculation
    const DWORD dwellings1Ptr = DwordAt(0x534CE7 + 3);
    const int MAX_MON_ID = IntAt(0x4A1657);

    for (auto &dwellingObjInfo : defaultRMGObjectsInfoByType[eObject::CREATURE_GENERATOR1])
    {
        const int dwellingCreatureType = DwordAt(dwellings1Ptr + (dwellingObjInfo.subtype << 2));
        if (dwellingCreatureType < MAX_MON_ID)
        {
            const int creatureAIValue = P_CreatureInformation[dwellingCreatureType].aiValue;
            dwellingObjInfo.value = creatureAIValue;
        }
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

    for (auto &objInfoVec : currentRMGObjectsInfoByType)
    {
        for (auto &objectInfo : objInfoVec)
        {

            objectInfo.ReadFromINI();
            objectInfo.Clamp();
        }
    }
}
LPCSTR RMGObjectInfo::GetObjectName(const H3MapItem *mapItem)
{
    if (mapItem)
    {
        return GetObjectName(mapItem->objectType, mapItem->objectSubtype);
    }
    return h3_NullString;
}
LPCSTR RMGObjectInfo::GetObjectName(const INT32 type, const INT32 subtype)
{
    LPCSTR result = h3_NullString;

    const int creatureBankId = cbanks::CreatureBanksExtender::GetCreatureBankType(type, subtype);
    if (creatureBankId >= 0)
    {
        return H3CreatureBankSetup::Get()[creatureBankId].name.String();
    }
    bool readSucces = false;

    libc::sprintf(h3_TextBuffer, OBJECT_SUBTYPE_NAME_JSON_KEY_FORMAT, type, subtype);
    LPCSTR jsonString = EraJS::read(h3_TextBuffer, readSucces);

    if (readSucces)
    {

        result = jsonString;
    }
    else
    {
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
            result = jsonString;
            break;
        case eObject::RESOURCE:
            result = P_ResourceName[subtype];
            break;
        case eObject::SHRINE_OF_MAGIC_INCANTATION:
            if (subtype)
            {
                libc::sprintf(h3_TextBuffer, OBJECT_SUBTYPE_NAME_JSON_KEY_FORMAT, type, subtype);
                result = EraJS::read(h3_TextBuffer);
            }
            else
            {
                result = P_ObjectName[type];
            }
            break;

        case eObject::TOWN:
            result = P_TownNames[subtype];
            break;
        default:
            result = P_ObjectName[type];
            break;
        }
    }

    return result;
}

LPCSTR RMGObjectInfo::GetObjectDescription(const H3MapItem *mapItem)
{
    if (mapItem)
    {
        return GetObjectDescription(mapItem->objectType, mapItem->objectSubtype);
    }
    return h3_NullString;
}
LPCSTR RMGObjectInfo::GetObjectDescription(const INT32 type, const INT32 subtype)
{
    LPCSTR result = h3_NullString;
    bool readSucces = false;

    libc::sprintf(h3_TextBuffer, OBJECT_SUBTYPE_DESC_JSON_KEY_FORMAT, type, subtype);
    LPCSTR jsonString = EraJS::read(h3_TextBuffer, readSucces);

    if (readSucces)
    {
        result = jsonString;
    }
    return h3_NullString;
}
inline int *create3DArray(int X, int Y, int Z)
{
    return new int[X * Y * Z];
}

inline int *create2DArray(int X, int Y)
{
    return new int[X * Y];
}

void GeneratedInfo::Assign(const H3RmgRandomMapGenerator *rmg,
                           const std::vector<RMGObjectInfo> (&userRmgInfoSet)[h3::limits::OBJECTS])
{

    const UINT templateBaseZoneAmount = rmg->zoneGenerators.Size();
    if (templateBaseZoneAmount)
    {
        UINT totalZoneConnectionsAmount = 0;
        const BOOL hasWater = rmg->waterAmount;
        if (hasWater)
        {
            // adding zone + totalZoneConnectionsAmount cause of water connections for the land zones creation
            std::unordered_set<UINT> connectionZonesIdPairs;

            for (const auto &zoneGen : rmg->zoneGenerators)
            {
                for (const auto &zoneConnection : zoneGen->zoneInfo->connections)
                {
                    const H3Position position(zoneConnection.zone->id, zoneGen->zoneInfo->id, 0);
                    if (connectionZonesIdPairs.insert(position.Mixed()).second)
                    {
                        totalZoneConnectionsAmount++;
                    }
                }
            }
        }

        const UINT zonesAmount = templateBaseZoneAmount + totalZoneConnectionsAmount;

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

        // sprintf(h3_TextBuffer, "maxObjectSubtype: %d", maxSubtypes[eObject::SPELL_SCROLL]);
        // H3Mes sagebox(h3_TextBuffer);

        // create counters and limits
        eachZoneGeneratedBySubtype = create3DArray(zonesAmount, H3_MAX_OBJECTS, maxObjectSubtype);
        mapGeneratedBySubtype = create2DArray(H3_MAX_OBJECTS, maxObjectSubtype);
        zoneLimitsBySubtype = create2DArray(H3_MAX_OBJECTS, maxObjectSubtype);
        mapLimitsBySubtype = create2DArray(H3_MAX_OBJECTS, maxObjectSubtype);

        const int arraylength = H3_MAX_OBJECTS * maxObjectSubtype * sizeof(int);

        memset(eachZoneGeneratedBySubtype, 0, zonesAmount * arraylength);
        memset(mapGeneratedBySubtype, 0, arraylength);
        memset(zoneLimitsBySubtype, 0, arraylength);
        memset(mapLimitsBySubtype, 0, arraylength);

        const int maxSubtype = maxObjectSubtype;
        LPCSTR iniFile = "Runtime/Rmg/Debug.ini";
        LPCSTR section = "MAIN";

        if (1)
        {

            Era::ClearIniCache(iniFile);

            Era::WriteStrToIni("seed", std::to_string(rmg->randomSeed).c_str(), section, iniFile);
            Era::WriteStrToIni("kek?", rand() & 1 ? "kek" : "no kek", section, iniFile);

            section = "Assign";
            // assign info from userData
            Era::WriteStrToIni("initLimits", std::to_string(false).c_str(), section, iniFile);
            Era::WriteStrToIni("maxObjectSubtype", std::to_string(maxSubtype).c_str(), section, iniFile);
            Era::WriteStrToIni("arraylength", std::to_string(arraylength).c_str(), section, iniFile);
            Era::WriteStrToIni("objectGenerators::Size", std::to_string(rmg->objectGenerators.Size()).c_str(), section,
                               iniFile);
            Era::SaveIni(iniFile);
        }

        for (const auto &vec : userRmgInfoSet)
        {
            for (const auto &info : vec)
            {

                try
                {
                    const int index2 = index2D(info.type, info.subtype, maxSubtype);
                    zoneLimitsBySubtype[index2] = info.zoneLimit;
                    mapLimitsBySubtype[index2] = info.mapLimit;
                }
                catch (const std::exception &)
                {
                    Era::WriteStrToIni("lastItemTypeToInit", std::to_string(info.type).c_str(), section, iniFile);
                    Era::WriteStrToIni("lastItemTypeToInit", std::to_string(info.type).c_str(), section, iniFile);

                    section = "GENERATORS";
                    int counter = 0;
                    for (auto *p_ObjGen : rmg->objectGenerators)
                    {
                        // each object gen of that type has hihgher subptype
                        Era::WriteStrToIni("type", std::to_string(p_ObjGen->type).c_str(), section, iniFile);
                        Era::WriteStrToIni("subtype", std::to_string(p_ObjGen->subtype).c_str(), section, iniFile);
                    }

                    Era::SaveIni(iniFile);
                }
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
