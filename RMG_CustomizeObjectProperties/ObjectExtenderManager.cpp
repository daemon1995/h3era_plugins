#include <thread>
#include <unordered_set>

#include "pch.h"

#include "SoundManager.h"

// Реальная dispatch-table
DispatchCell dispatchTable[MAX_TYPES][MAX_SUBTYPES]{};

// helper: создаём массив HookFn из дескриптора
static void BuildMethodsArray(const ObjectExtenderDesc *desc, HookFn outMethods[METHODS_COUNT])
{
    outMethods[AFTER_LOADING_OBJECTS_TXT] = nullptr; // пока нет реализации
    outMethods[CREATE_RMG_OBJECT_GEN] = nullptr;
    outMethods[NEW_GAME_OBJECT_ITERATION] = nullptr;
    outMethods[NEW_WEEK_OBJECT_ITERATION] = nullptr;
    outMethods[HERO_MAP_ITEM_VISIT] = static_cast<HookFn>(desc->visit);
    outMethods[SET_HINT_IN_H3_TEXT_BUFFER] = static_cast<HookFn>(desc->set_hint);
    outMethods[AI_MAP_ITEM_WEIGHT] = static_cast<HookFn>(desc->set_ai_weight);
    outMethods[RMG_DLG_SHOW_CUSTOM_OBJECT_HINT] = nullptr; // можно добавить позже
}

// Реализация регистрации
DllExport bool __stdcall RegisterObjectExtender(const ObjectExtenderDesc *desc)
{
    if (!desc || !desc->rules || desc->ruleCount == 0)
        return false;

    HookFn methods[METHODS_COUNT];
    BuildMethodsArray(desc, methods);

    for (int r = 0; r < desc->ruleCount; ++r)
    {
        const ObjectMatchRule &rule = desc->rules[r];
        int type = rule.objectType;

        if (!rule.subtypes) // все подтипы
        {
            for (int st = 0; st < MAX_SUBTYPES; ++st)
            {
                DispatchCell &cell = dispatchTable[type][st];
                for (int m = 0; m < METHODS_COUNT; ++m)
                {
                    if (desc->methods_mask & (1 << m))
                        cell.methods[m] = methods[m];
                }
            }
        }
        else // только указанные подтипы
        {
            for (int st = 0; st < rule.subtypeCount; ++st)
            {
                int subtype = rule.subtypes[st];
                DispatchCell &cell = dispatchTable[type][subtype];
                for (int m = 0; m < METHODS_COUNT; ++m)
                {
                    if (desc->methods_mask & (1 << m))
                        cell.methods[m] = methods[m];
                }
            }
        }
    }

    return true;
}
DllExport LPCSTR __stdcall GetResolvedObjectName(const int objectType, const int subtype)
{
    return RMGObjectInfo::GetObjectName(objectType, subtype);
}

namespace extender
{
#define READ_RMG_JSON_FIELD_1(field, objType)                                                                          \
    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d." #field, objType).String())
#define READ_RMG_JSON_FIELD_2(field, objType, objSubtype)                                                              \
    EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d." #field, objType, objSubtype).String())
bool RMGObjectSetable::operator==(const RMGObjectSetable &other) const noexcept
{
    return type == other.type && subtype == other.subtype; // ? type < other.type : subtype < other.subtype;
}

size_t RMGObjectSetable::HashFunction::operator()(const RMGObjectSetable &obj) const noexcept
{
    size_t typeHash = std::hash<int>()(obj.type);
    size_t subtypeHash = std::hash<int>()(obj.subtype) << 1;
    return typeHash ^ subtypeHash;
}

/// <summary>
/// function to load additional properties into objects.txt
/// it is called from H3GameMainSetup::LoadObjects at 0x515038
/// it also may replace original properties with new ones if they have same
/// type/subtype/mask but different other params
/// </summary>
/// <param name=""></param>
_LHF_(ObjectExtenderManager::LoadObjectsTxt)
{
    // check if there are any object properties extenders
    if (H3TextFile *objectTxt = *reinterpret_cast<H3TextFile **>(c->ebp + 0x8))
    {
        if (const size_t newProperties = instance->additionalProperties.InsertPropertiesIntoObjectsList(objectTxt))
        {
            c->eax += newProperties;
        }
    }

    return EXEC_DEFAULT;
}
ObjectExtenderManager *ObjectExtenderManager::instance = nullptr;

ObjectExtenderManager::ObjectExtenderManager()
    : IGamePatch("EraPlugin.RMG_CustomizeObjectProperties.ObjectExtenderManager.daemon_n")
{
    CreatePatches();
    // Initialize your extenders here
}
void ObjectExtenderManager::CreatePatches()
{
    if (!m_isInited)
    {

        m_isInited = true;

        // Era::RegisterHandler(OnWogObjectHint);
        _PI->WriteLoHook(0x515038, LoadObjectsTxt);
        _PI->WriteHiHook(0x4EE01C, THISCALL_, H3GameMainSetup__LoadObjects);

        // _PI->WriteLoHook(0x40C5A1, H3AdventureManager__GetPyramidObjectHoverHint);
        //_PI->WriteLoHook(0x414F66, H3AdventureManager__GetPyramidObjectClickHint);
        _pi->WriteLoHook(0x04C0A5F, Game__NewGameObjectIteration);
        _pi->WriteLoHook(0x04C8847, Game__NewWeekObjectIteration);

        _PI->WriteLoHook(0x4A819C, H3AdventureManager__ObjectVisit);

        _PI->WriteLoHook(0x40D052, H3AdventureManager__GetDefaultObjectHoverHint); // mouse over hint
        _PI->WriteLoHook(0x415999, H3AdventureManager__GetDefaultObjectClickHint); // rmc hint

        _PI->WriteLoHook(0x528559, AIHero_GetObjectPosWeight); // AI object visit stuff

        skipMapMessageByHdMod = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

        // patch hota object types unable to be entered
        auto *settingsTable = H3GlobalObjectSettings::Get();
        for (size_t i = HOTA_PICKUPABLE_OBJECT_TYPE; i <= HOTA_UNREACHABLE_OBJECT_TYPE; i++)
        {
            settingsTable[i].cannotEnter = true;
            settingsTable[i].exitTop = true;
            settingsTable[i].canBeRemoved = true;
        }

        //	Era::RegisterHandler(OnWogObjectHint, "OnWogObjectHint");
    }
}

int ObjectExtenderManager::ShowObjectHint(LoHook *h, HookContext *c, const BOOL isRighClick)
{
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);
    const H3Hero *currentHero = *reinterpret_cast<H3Hero **>(c->ebp - 0x10);

    const int interactPlayerId = IntAt(0x6977DC); // P_MePlayerID *(INT32 *)0x6977DC
    BOOL hintIsSet = false;
    for (auto &extender : objectExtenders)
    {
        hintIsSet = extender->SetHintInH3TextBuffer(mapItem, currentHero, interactPlayerId, isRighClick);
        if (hintIsSet)
        {
            break;
        }
    }

    if (0)
    {
        if (auto extender = FindExtender(mapItem))
        {
            hintIsSet = extender->SetHintInH3TextBuffer(mapItem, currentHero, interactPlayerId, isRighClick);
        }
    }
    if (!hintIsSet)
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);
        sprintf(h3_TextBuffer, "%s", objName.String());
    }

    c->edi = (int)h3_TextBuffer;
    c->return_address = h->GetAddress() + 7;

    return NO_EXEC_DEFAULT;
}

// ObjectExtender *ObjectExtenderManager::GetExtender(const INT16 mapItemType, const INT16 mapItemSubtype)
//{
//
//     // if (true)
//     {
//         return extendersMap[mapItemType << 16 | mapItemSubtype];
//     }
//
//     return nullptr;
// }
H3RmgObjectGenerator *ObjectExtenderManager::CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &objectInfo) noexcept
{
    H3RmgObjectGenerator *objGen = nullptr;
    if (objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1))
    {
        THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, objectInfo.type, objectInfo.subtype, objectInfo.value,
                   objectInfo.density);
    }
    return objGen;
}

_LHF_(ObjectExtenderManager::H3AdventureManager__GetDefaultObjectClickHint)
{
    return instance->ShowObjectHint(h, c, true);
}
_LHF_(ObjectExtenderManager::H3AdventureManager__GetDefaultObjectHoverHint)
{
    return instance->ShowObjectHint(h, c, false);
}

_LHF_(ObjectExtenderManager::AIHero_GetObjectPosWeight)
{

    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->esi))
    {

        for (auto &objectExtender : instance->objectExtenders)
        {
            H3Hero *currentHero = reinterpret_cast<H3Hero *>(c->ebx);
            int *moveDistance = reinterpret_cast<int *>(c->edi);
            const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);
            const H3Position pos = *reinterpret_cast<H3Position *>(c->ebp + 0x8);

            INT aiResWeight = 0;
            if (objectExtender->SetAiMapItemWeight(mapItem, currentHero, player, aiResWeight, moveDistance, pos))
            {
                c->eax = aiResWeight;
                c->return_address = 0x05285A1;
                return NO_EXEC_DEFAULT;
            }
        }

        return EXEC_DEFAULT;

        if (auto *extenders = FindExtender(mapItem))
        {
            H3Hero *currentHero = reinterpret_cast<H3Hero *>(c->ebx);
            int *moveDistance = reinterpret_cast<int *>(c->edi);
            const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);
            const H3Position pos = *reinterpret_cast<H3Position *>(c->ebp + 0x8);

            INT aiResWeight = 0;
            if (extenders->SetAiMapItemWeight(mapItem, currentHero, player, aiResWeight, moveDistance, pos))
            {
                c->eax = aiResWeight;
                c->return_address = 0x05285A1;
                return NO_EXEC_DEFAULT;
            }
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(ObjectExtenderManager::Game__NewGameObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);

    for (auto &objectExtender : instance->objectExtenders)
    {
        if (objectExtender->InitNewGameMapItemSetup(mapItem))
        {
            return EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;

    if (auto *objectExtender = FindExtender(mapItem))
    {
        objectExtender->InitNewGameMapItemSetup(mapItem);
    }

    return EXEC_DEFAULT;
}
_LHF_(ObjectExtenderManager::Game__NewWeekObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);

    for (auto &objectExtender : instance->objectExtenders)
    {
        if (objectExtender->InitNewWeekMapItemSetup(mapItem))
        {
            return EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
    if (auto *objectExtender = FindExtender(mapItem))
    {
        objectExtender->InitNewWeekMapItemSetup(mapItem);
    }

    return EXEC_DEFAULT;
}
_LHF_(ObjectExtenderManager::H3AdventureManager__ObjectVisit)
{
    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {

        H3Hero *currentHero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);

        const H3Position position = DwordAt(c->ebp + 0x10);
        const bool isHuman = DwordAt(c->ebp + 0x14);

        for (auto &objectExtender : instance->objectExtenders)
        {
            if (objectExtender->VisitMapItem(currentHero, mapItem, position, isHuman))
            {
                return EXEC_DEFAULT;
            }
        }
        return EXEC_DEFAULT;

        if (auto *objectExtender = FindExtender(mapItem))
        {
            H3Hero *currentHero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);

            const H3Position position = DwordAt(c->ebp + 0x10);
            const bool isHuman = DwordAt(c->ebp + 0x14);
            objectExtender->VisitMapItem(currentHero, mapItem, position, isHuman);
        }
    }

    return EXEC_DEFAULT;
}

void __stdcall ObjectExtenderManager::H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup)
{

    // get Additional Propertise  <-  R E W R I T E   L A T E R   W I T H   N L O H M A N
    auto &additionalProperties = instance->additionalProperties;
    additionalProperties.LoadCommonProperties();
    // load additional unique objects properties from each loaded mod json key
    additionalProperties.LoadAdditionalPropertiesFromMods();

    // call native fucntion to load objects.txt (0x515038)
    // and increase eax by number of added objects
    THISCALL_1(void, h->GetDefaultFunc(), setup);

    // create max subtype value for all object gens
    auto &maximumObjectSubtypes = instance->maximumObjectSubtypes;

    std::vector<sound::SoundManager::ObjectSound> addedWavNames;
    maximumObjectSubtypes[eObject::SPELL_SCROLL] = 7;

    auto *objList = setup->objectLists;
    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    { // iterate all the objects types entries

        for (const auto &obj : objList[objType])
        {

            // each object gen of that type has hihgher subptype
            if (obj.subtype >= maximumObjectSubtypes[obj.type])
            {
                // assume that object t/s must be max limit array m_size
                maximumObjectSubtypes[obj.type] = obj.subtype;
            }
        }

        // first check only object type value/density

        const int objectTypeValue = READ_RMG_JSON_FIELD_1(value, objType);
        const int objectTypeDensity = READ_RMG_JSON_FIELD_1(density, objType);

        bool objectTypeHasLoopSound = false;
        LPCSTR objectTypeWavName = EraJS::read(H3String::Format("RMG.objectGeneration.%d.sound.loop", objType).String(),
                                               objectTypeHasLoopSound);

        auto &additionalRmgObjects = instance->additionalRmgObjects;
        // next check each object subtype value/density
        for (size_t objSubtype = 0; objSubtype <= maximumObjectSubtypes[objType]; objSubtype++)
        {

            const int objectSubtypeValue = READ_RMG_JSON_FIELD_2(value, objType, objSubtype);
            // EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.value", objType, objSubtype).String());
            const int objectSubtypeDensity = READ_RMG_JSON_FIELD_2(density, objType, objSubtype);
            // EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.density", objType, objSubtype).String());

            // if we have any value/density
            // create rmgObjectInfo object
            if (objectSubtypeValue || objectSubtypeDensity || objectTypeValue || objectTypeDensity)
            {
                RMGObjectInfo rmgObjectInfo(objType, objSubtype);
                rmgObjectInfo.value = objectSubtypeValue;
                rmgObjectInfo.density = objectSubtypeDensity;
                additionalRmgObjects.emplace_back(rmgObjectInfo);
            }

            bool objectSubtypeHasLoopSound = false;
            LPCSTR objectSubtypeWavName =
                EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.sound.loop", objType, objSubtype).String(),
                            objectSubtypeHasLoopSound);

            if (objectSubtypeHasLoopSound)
            {
                addedWavNames.emplace_back(
                    sound::SoundManager::ObjectSound{objectSubtypeWavName, (objType << 16) | objSubtype});
            }
            else if (objectTypeHasLoopSound)
            {
                addedWavNames.emplace_back(
                    sound::SoundManager::ObjectSound{objectTypeWavName, (objType << 16) | 0xFFFF});
            }
        }
    }
    if (addedWavNames.size())
    {
        std::thread th(&sound::SoundManager::Init, addedWavNames);
        th.detach();
        //  sound::SoundManager::Init(addedWavNames);
    }
    instance->allowRegistration = false;
    // clear registered extenders from previous loads
    instance->registeredExtenders.clear();
    // instance->InitializeExtendersTypes();

    // block objec entry tile passability for HOTA_PICKUPABLE_OBJECT_TYPE
    auto &extenders = instance->objectExtenders;

    // Get All The Extenders we have
    for (auto &extender : extenders)
    {
        // call additional data loading from json
        extender->AfterLoadingObjectsTxtProc(maximumObjectSubtypes);
    }
    editor::RMGObjectsEditor::Init(maximumObjectSubtypes);
}

void ObjectExtenderManager::AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjectsList)
{
    // check if there are any object properties extenders
    if (objectExtenders.size())
    {
        std::unordered_set<RMGObjectSetable, RMGObjectSetable::HashFunction> objectsSet;
        // create set of the objects to add only unique objects
        for (auto rmgObj : *rmgObjectsList)
        {
            objectsSet.insert(RMGObjectSetable{rmgObj->type, rmgObj->subtype});
        }

        ObjectExtender *typeRelatedObjectExtender = nullptr;
        // iterate each added RMG INFO
        for (auto &info : additionalRmgObjects)
        {
            H3RmgObjectGenerator *objGen = nullptr;

            // check if it is possible to add object into the list
            if (objectsSet.insert(RMGObjectSetable{info.type, info.subtype}).second)
            {

                switch (info.type)
                {
                case eObject::CREATURE_GENERATOR4:
                    objGen = CreateDefaultH3RmgObjectGenerator(info);
                    objGen->vTable = reinterpret_cast<H3RmgObjectGenerator::VTable *>(0x0640BC8);

                    break;
                case eObject::FREELANCERS_GUILD:
                    objGen = CreateDefaultH3RmgObjectGenerator(info);

                    break;
                default:
                    if (dispatchTable)
                    {
                    }
                    // if (typeRelatedObjectExtender = FindExtender(info.type, info.subtype))
                    //{
                    //     typeRelatedObjectExtender->CreateRMGObjectGen(info);
                    // }
                    //  iterate all extenders container
                    for (auto &extender : objectExtenders)
                    {
                        // if yes then create obj gen

                        if (objGen = extender->CreateRMGObjectGen(info))
                            break;
                        // and return to add into the list
                    }
                    break;
                }
                if (objGen)
                {
                    rmgObjectsList->Push(objGen);
                }
            }
        }
    }
}
BOOL ObjectExtenderManager::ShowObjectExtendedInfo(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                                   H3String &stringResult) noexcept
{
    // stringResult.Erase();
    LPCSTR defName = attributes->defName.String();
    H3DefLoader def(defName);

    stringResult = H3String::Format("{~>%s:0:%d block}", defName, rand() % def->groups[0]->count); // .Append(defPic);
    stringResult += info.GetName();
    stringResult.Append(H3String::Format(" (%d/%d)", info.type, info.subtype));

    // display terrain types that object can be placed on

    H3String terrainStr;
    bool hasTerrain = false;
    for (size_t i = 0; i < 10; i++)
    {
        if (attributes->maskTerrain.bitfield.GetState(i))
        {
            LPCSTR terrainDefName = (*reinterpret_cast<LPCSTR **>(0x04071E7 + 1))[i];
            auto defTerrain = H3LoadedDef::Load(terrainDefName); // [i] ;
            if (defTerrain)
            {
                const int frameIndex = defTerrain->groups[0]->count < 60 ? 15 : 60;
                terrainStr += H3String::Format("{~>%s:0:%d valign=top}", terrainDefName, frameIndex);
                hasTerrain = true;
                defTerrain->Dereference();
            }
        }
    }
    if (hasTerrain)
    {
        stringResult.Append("\n");
        stringResult += terrainStr;
    }

    auto &extenders = instance->objectExtenders;

    for (auto &i : extenders)
    {
        if (i->RMGDlg_ShowCustomObjectHint(info, attributes, stringResult))
        {
            return true;
        }
    }
    return 0;
}

BOOL ObjectExtenderManager::AddExtender(ObjectExtender *ext)
{

    if (!allowRegistration)
    {
        MessageBoxA(NULL, ErrorText::ERR_REGISTRATION_CLOSED, ErrorText::TITLE_ERROR, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!ext)
    {
        MessageBoxA(NULL, ErrorText::ERR_EMPTY_EXTENDER, ErrorText::TITLE_ERROR, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!instance->registeredExtenders.insert(ext).second)
    {
        MessageBoxA(NULL, ErrorText::ERR_EXTENDER_ALREADY_REGISTERED, ErrorText::TITLE_ERROR, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    // add extender to the list
    instance->objectExtenders.push_back(ext);
    return TRUE;
}

ObjectExtenderManager *ObjectExtenderManager::Get()
{

    if (!instance)
        instance = new ObjectExtenderManager();
    return instance;
}

DllExport BOOL __stdcall RegisterObjectExtenderOld(ObjectExtender *extender)
{
    return ObjectExtenderManager::Get()->AddExtender(extender);
}

DllExport ObjectExtender *__stdcall CreateObjectExtender(ObjectExtender *_this) noexcept
{
    //*_this[0]

    return nullptr;
}
DllExport ObjectExtender *__stdcall CreateObjectExtenderByType(const int objectType, const int objectSubtype) noexcept
{
    //*_this[0]

    return nullptr;
}

} // namespace extender
