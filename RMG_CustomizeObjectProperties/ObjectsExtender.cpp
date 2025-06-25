#include "pch.h"
#include <thread>
#include <sstream>
namespace extender
{

std::vector<ObjectsExtender *> ObjectsExtender::extenders;
std::vector<RMGObjectInfo> ObjectsExtender::additionalRmgObjects;
std::unordered_map<std::string, std::string> ObjectProperty::additionalPropertiesMap;
BOOL ObjectsExtender::skipMapMessageByHdMod = false;
BOOL ObjectsExtender::patchesCreated = false;

void EditableH3TextFile::AddLine(LPCSTR txt)
{
    this->text.Add(txt);
}
size_t EditableH3TextFile::GetLineCount() const noexcept
{
    return this->text.Size();
}
ObjectsExtender::ObjectsExtender(PatcherInstance *pi) : IGamePatch(pi)
{

    if (!patchesCreated)
    {
        CreatePatches();
        patchesCreated = true;
        skipMapMessageByHdMod = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");
    }
    extenders.emplace_back(this);
}

// call atoi for hte first txt file
// int __stdcall ObjectsExtender::LoadObjectsTxt(HiHook* h, const DWORD data)

_LHF_(ObjectsExtender::LoadObjectsTxt)
{
    // check if there are any object properties extenders
    if (ObjectProperty::additionalPropertiesMap.size())
    {
        // get objects added list
        EditableH3TextFile *objectTxt = *reinterpret_cast<EditableH3TextFile **>(c->ebp + 0x8);

        const UINT linesCount = objectTxt->GetLineCount();
        for (size_t i = 1; i < linesCount; i++)
        {
            // create buffer string to transform it
            std::string txtPropertyString((*objectTxt)[i]);

            // std::transform(txtPropertyString.begin(), txtPropertyString.end(), txtPropertyString.begin(), ::tolower);

            if (const std::string *propertyReplace = ObjectProperty::FindPropertyReplace(txtPropertyString.c_str()))
            {
                // Era::WriteStrToIni((*objectTxt)[i], propertyReplace->data(), "tryingToReplace",
                //                    "runtime/tum/properties.ini");
                (*objectTxt)[i] = propertyReplace->data();
            }
        }

        // copy original objects added list into set
        std::unordered_set<LPCSTR> objectsSet(objectTxt->begin(), objectTxt->end());

        UINT32 newProperties = 0;
        // iterate each added property
        for (auto &prop : ObjectProperty::additionalPropertiesMap)
        {
            // Era::WriteStrToIni(prop.first.c_str(), prop.second.c_str(), "objectsExtender",
            //                    "runtime/tum/properties.ini");

            // if possible to insert
            if (objectsSet.insert(prop.second.c_str()).second)
            {

                // add that property into main objects list
                objectTxt->AddLine(prop.second.c_str()); // add new txt entry
                // increase added objects number
                newProperties++;
            }
        }
        // Era::SaveIni("runtime/tum/properties.ini");

        c->eax += newProperties;
    }

    return EXEC_DEFAULT;
}

ObjectsExtender::~ObjectsExtender()
{
    // extenders.erase(this);
}
void ObjectsExtender::LoadMapObjectPropertiesByTypeSubtypes() noexcept
{
    bool readSuccess = false;

    // load properties for direct type/subtype objects
    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    { // iterate all the objects types entries

        const size_t maxSubtype =
            objType == eObject::ARTIFACT || objType == eObject::CREATURE_GENERATOR1 ? limits::EXTENDED : limits::COMMON;

        for (size_t objSubtype = 0; objSubtype < maxSubtype; objSubtype++)
        {
            size_t propertyIdCounter = 0;

            do
            {
                std::string str = EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.properties.%d", objType,
                                                               objSubtype, propertyIdCounter++)
                                                  .String(),
                                              readSuccess);

                if (readSuccess)
                {
                    ObjectProperty::AddProperty(str);
                }
            } while (readSuccess);
        }
    }
}

void ObjectsExtender::LoadMapObjectPropertiesFromLoadedMods() noexcept
{

    std::vector<std::string> modList;
    modList::GetEraModList(modList, true);

    for (auto &modName : modList)
    {
        //    std::string modName = "wog";
        bool readSuccess = false;

        int propertyIdCounter = 0;

        // first read raw propertiese array
        do
        {
            std::string str = EraJS::read(
                H3String::Format("RMG.%s.properties.%d", modName.c_str(), propertyIdCounter++).String(), readSuccess);
            if (readSuccess)
            {
                ObjectProperty::AddProperty(str);
            }
        } while (readSuccess);

        // later read array of arrays
        int arrayCounter = 0;
        bool breakFlag = false;
        // reset objects counter
        propertyIdCounter = 0;
        do
        {
            do
            {
                std::string str = EraJS::read(
                    H3String::Format("RMG.%s.properties.%d.%d", modName.c_str(), arrayCounter, propertyIdCounter)
                        .String(),
                    readSuccess);
                if (readSuccess)
                {
                    ObjectProperty::AddProperty(str);
                    propertyIdCounter++;
                }
                else if (propertyIdCounter)
                {
                    propertyIdCounter = 0;
                    arrayCounter++;
                    break;
                }
                else
                {
                    breakFlag = true;
                    break;
                }

            } while (true);

        } while (!breakFlag);
    }
}

void __stdcall ObjectsExtender::H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup)
{

    // get Additional Propertise  <-  R E W R I T E   L A T E R   W I T H   N L O H M A N

    LoadMapObjectPropertiesByTypeSubtypes();

    LoadMapObjectPropertiesFromLoadedMods();

    // load additional unique objects properties from each loaded mod json key

    // call native fucntion to load objects.txt (0x515038)
    THISCALL_1(void, h->GetDefaultFunc(), setup);

    // create max subtype value for all object gens
    INT16 maxSubtypes[h3::limits::OBJECTS] = {};

    auto *objList = setup->objectLists;

    std::vector<sound::SoundManager::ObjectSound> addedWavNames;
    maxSubtypes[eObject::SPELL_SCROLL] = 7;
    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    { // iterate all the objects types entries

        for (const auto &obj : objList[objType])
        {

            // each object gen of that type has hihgher subptype
            if (obj.subtype >= maxSubtypes[obj.type])
            {
                // assume that object t/s must be max limit array m_size
                maxSubtypes[obj.type] = obj.subtype + 1;
            }
        }

        // first check only object type value/density

        const int objectTypeValue = EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.value", objType).String());
        const int objectTypeDensity =
            EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.density", objType).String());

        bool objectTypeHasLoopSound = false;
        LPCSTR objectTypeWavName = EraJS::read(H3String::Format("RMG.objectGeneration.%d.sound.loop", objType).String(),
                                               objectTypeHasLoopSound);

        // next check each object subtype value/density
        for (size_t objSubtype = 0; objSubtype < maxSubtypes[objType]; objSubtype++)
        {

            const int objectSubtypeValue =
                EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.value", objType, objSubtype).String());
            const int objectSubtypeDensity =
                EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.density", objType, objSubtype).String());

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

    // block objec entry tile passability for HOTA_PICKUPABLE_OBJECT_TYPE

    struct MapItemSettings
    {
        char yellowTileNonPassability;
        char unknownTileNonPassability[2];
        char data[9];
        eObject type;
    };

    MapItemSettings *settingsTable = *reinterpret_cast<MapItemSettings **>(0x0660428);
    settingsTable[HOTA_PICKUPABLE_OBJECT_TYPE].yellowTileNonPassability = true;
    settingsTable[HOTA_PICKUPABLE_OBJECT_TYPE].unknownTileNonPassability[0] = true;
    settingsTable[HOTA_PICKUPABLE_OBJECT_TYPE].unknownTileNonPassability[1] = true;
    settingsTable[HOTA_UNREACHABLE_YT_OBJECT_TYPE].yellowTileNonPassability = true;
    settingsTable[HOTA_UNREACHABLE_YT_OBJECT_TYPE].unknownTileNonPassability[0] = true;
    settingsTable[HOTA_UNREACHABLE_YT_OBJECT_TYPE].unknownTileNonPassability[1] = true;

    // Get All The Extenders we have
    for (auto &extender : extenders)
    {
        // call additional data loading from json
        extender->AfterLoadingObjectTxtProc(maxSubtypes);
    }
    editor::RMGObjectsEditor::Init(maxSubtypes);
}

H3RmgObjectGenerator *ObjectsExtender::CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &objectInfo) noexcept
{

    H3RmgObjectGenerator *objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1);
    THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, objectInfo.type, objectInfo.subtype, objectInfo.value,
               objectInfo.density);

    return objGen;
}
H3RmgObjectGenerator *ObjectsExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    return CreateDefaultH3RmgObjectGenerator(objectInfo);
}

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
void ObjectsExtender::AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjectsList)
{
    // check if there are any object properties extenders
    if (extenders.size())
    {
        std::unordered_set<RMGObjectSetable, RMGObjectSetable::HashFunction> objectsSet;
        // create set of the objects to add only unique objects
        for (auto rmgObj : *rmgObjectsList)
        {
            objectsSet.insert({rmgObj->type, rmgObj->subtype});
        }
        // iterate each added RMG INFO

        for (auto &info : additionalRmgObjects)
        {
            // check if it is possible to add object into the list

            if (objectsSet.insert({info.type, info.subtype}).second)
            {
                // iterate all extenders container

                for (auto &extender : extenders)
                {

                    // if yes then create obj gen
                    H3RmgObjectGenerator *objGen = extender->CreateRMGObjectGen(info);
                    // and return to add into the list
                    if (objGen)
                    {
                        rmgObjectsList->Push(objGen);
                    }
                }

                if (info.type == eObject::CREATURE_GENERATOR4)
                {
                    H3RmgObjectGenerator *objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1);
                    THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, info.type, info.subtype, info.value,
                               info.density);
                    objGen->vTable = (H3RmgObjectGenerator::VTable *)0x0640BC8;
                    rmgObjectsList->Push(objGen);
                }
            }
        }
    }
}

BOOL ObjectsExtender::ShowObjectExtendedInfo(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                             H3String &stringResult) noexcept
{
    stringResult = "";
    LPCSTR defName = attributes->defName.String();
    H3DefLoader def(defName);

    stringResult += H3String::Format("{~>%s:0:%d block}", defName, rand() % def->groups[0]->count); // .Append(defPic);
    stringResult += info.GetName();
    stringResult.Append(H3String::Format(" (%d/%d)", info.type, info.subtype));

    for (auto &i : extenders)
    {
        if (i->RMGDlg_ShowCustomObjectHint(info, attributes, stringResult))
        {
            return true;
        }
    }
    return 0;
}

_LHF_(ObjectsExtender::H3AdventureManager__ObjectVisit)
{
    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {

        H3Hero *currentHero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);

        const H3Position position = DwordAt(c->ebp + 0x10);
        const bool isHuman = DwordAt(c->ebp + 0x14);

        for (const auto &extender : extenders)
        {
            if (extender->VisitMapItem(currentHero, mapItem, position, isHuman))
            {
                return EXEC_DEFAULT;
            }
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(H3AdventureManager__GetPyramidObjectHoverHint)
{
    bool readSucces = false;

    const H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);
    LPCSTR objName = EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.name", mapItem->objectType, mapItem->objectSubtype).String(),
        readSucces);
    libc::sprintf(h3_TextBuffer, objName);
    return EXEC_DEFAULT;

    int eventId = 3433444;
    Era::AllocErmFunc("OnWogObjectHint", eventId);

    auto p = mapItem->GetCoordinates();

    Era::TXVars *xVars = Era::GetArgXVars();

    (*xVars)[0] = mapItem->objectSubtype;
    (*xVars)[1] = (int)objName;
    (*xVars)[2] = p.x;
    (*xVars)[3] = p.y;
    (*xVars)[4] = p.z;

    Era::FireErmEvent(eventId);

    Era::TXVars *retXVars = Era::GetRetXVars();
    retXVars[1];
    if (readSucces)
    {
        libc::sprintf(h3_TextBuffer, *(LPCSTR *)(retXVars[1]));
    }

    return EXEC_DEFAULT;
}

_LHF_(H3AdventureManager__GetPyramidObjectClickHint)
{
    bool readSucces = false;

    const H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);
    LPCSTR objName =
        RMGObjectInfo::GetObjectName(mapItem); // EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.name",
                                               // mapItem->objectType, mapItem->objectSubtype).String(), readSucces);

    int eventId = 3433444;
    Era::AllocErmFunc("OnWogObjectHint", eventId);

    auto p = mapItem->GetCoordinates();

    Era::TXVars *xVars = Era::GetArgXVars();

    (*xVars)[0] = mapItem->objectSubtype;
    (*xVars)[1] = (int)objName;
    (*xVars)[2] = p.x;
    (*xVars)[3] = p.y;
    (*xVars)[4] = p.z;

    Era::FireErmEvent(eventId);

    Era::TXVars *retXVars = Era::GetRetXVars();
    retXVars[1];
    if (readSucces)
    {
        libc::sprintf(h3_TextBuffer, *(LPCSTR *)(retXVars[1]));
    }

    return EXEC_DEFAULT;
}

int ObjectsExtender::ShowObjectHint(LoHook *h, HookContext *c, const BOOL isRighClick)
{
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);
    const H3Hero *currentHero = *reinterpret_cast<H3Hero **>(c->ebp - 0x10);

    BOOL hintIsSet = false;
    for (auto &extender : extenders)
    {
        hintIsSet = extender->SetHintInH3TextBuffer(mapItem, currentHero, P_ActivePlayer->Get(), isRighClick);

        if (hintIsSet)
        {
            break;
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

_LHF_(ObjectsExtender::H3AdventureManager__GetDefaultObjectClickHint)
{
    return ShowObjectHint(h, c, true);
}
_LHF_(ObjectsExtender::H3AdventureManager__GetDefaultObjectHoverHint)
{
    return ShowObjectHint(h, c, false);
}

_LHF_(ObjectsExtender::AIHero_GetObjectPosWeight)
{

    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->esi))
    {

        H3Hero *currentHero = reinterpret_cast<H3Hero *>(c->ebx);
        int *moveDistance = reinterpret_cast<int *>(c->edi);
        const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);
        const H3Position pos = *reinterpret_cast<H3Position *>(c->ebp + 0x8);

        INT aiResWeight = 0;

        for (const auto &extender : extenders)
        {
            if (extender->SetAiMapItemWeight(mapItem, currentHero, player, aiResWeight, moveDistance, pos))
            {

                c->eax = aiResWeight;
                c->return_address = 0x05285A1;
                return NO_EXEC_DEFAULT;
            }
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(ObjectsExtender::Game__NewGameObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);
    for (const auto &extender : extenders)
    {
        if (extender->InitNewGameMapItemSetup(mapItem))
        {
            return EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}
_LHF_(ObjectsExtender::Game__NewWeekObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);
    for (const auto &extender : extenders)
    {
        if (extender->InitNewWeekMapItemSetup(mapItem))
        {
            return EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}

void ObjectsExtender::CreatePatches()
{
    // before any of extenders is inited
    if (extenders.empty())
    {

        // Era::RegisterHandler(OnWogObjectHint);
        _PI->WriteLoHook(0x515038, LoadObjectsTxt);

        // _PI->WriteLoHook(0x40C5A1, H3AdventureManager__GetPyramidObjectHoverHint);
        //_PI->WriteLoHook(0x414F66, H3AdventureManager__GetPyramidObjectClickHint);
        _pi->WriteLoHook(0x04C0A5F, Game__NewGameObjectIteration);
        _pi->WriteLoHook(0x04C8847, Game__NewWeekObjectIteration);

        _PI->WriteLoHook(0x4A819C, H3AdventureManager__ObjectVisit);

        _PI->WriteLoHook(0x40D052, H3AdventureManager__GetDefaultObjectHoverHint); // mouse over hint
        _PI->WriteLoHook(0x415999, H3AdventureManager__GetDefaultObjectClickHint); // rmc hint

        _PI->WriteLoHook(0x528559, AIHero_GetObjectPosWeight); // AI object visit stuff

        _PI->WriteHiHook(0x4EE01C, THISCALL_, H3GameMainSetup__LoadObjects);

        //	Era::RegisterHandler(OnWogObjectHint, "OnWogObjectHint");
    }
}
void ObjectsExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
}
BOOL ObjectsExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                            const BOOL isRightClick) const noexcept
{
    return false;
}
BOOL ObjectsExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *activePlayer,
                                         int &aiMapItemWeight, int *moveDistance, const H3Position pos) const noexcept
{
    return false;
}
BOOL ObjectsExtender::RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                                  const H3String &defaltText) noexcept
{
    return false;
}
BOOL ObjectsExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{
    return false;
}
BOOL ObjectsExtender::InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept
{
    return false;
}
BOOL ObjectsExtender::VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                                   const BOOL isHuman) const noexcept
{
    return false;
}

std::string ObjectProperty::GetMapKey(LPCSTR propertyString) noexcept
{
    std::istringstream stream(propertyString);
    std::vector<std::string> words;
    std::string word;
    words.reserve(9);
    // ��������� ������ �� ����� � ��������� �� � ������
    while (stream >> word)
    {
        words.emplace_back(word);
    }
    // get map key from def + object type + object subtype
    std::transform(words[0].begin(), words[0].end(), words[0].begin(), ::tolower);
    libc::sprintf(h3_TextBuffer, "%s_%s_%s", words[0].c_str(), words[5].c_str(), words[6].c_str());
    words.clear();
    return h3_TextBuffer;
}
// check if we have replaced property for that object
const std::string *ObjectProperty::FindPropertyReplace(LPCSTR other) noexcept
{

    auto it = additionalPropertiesMap.find(GetMapKey(other));
    return it != additionalPropertiesMap.end() ? &it->second : nullptr;
}

BOOL ObjectProperty::AddProperty(std::string &other) noexcept
{
    std::transform(other.begin(), other.end(), other.begin(), ::tolower);

    return additionalPropertiesMap.insert(std::make_pair(GetMapKey(other.data()), other)).second;
}
} // namespace extender
