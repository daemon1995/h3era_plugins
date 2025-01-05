#include "pch.h"
namespace extender
{

std::vector<ObjectsExtender *> ObjectsExtender::extenders;
std::vector<std::string> ObjectsExtender::additionalProperties;
std::vector<RMGObjectInfo> ObjectsExtender::additionalRmgObjects;
LoopSoundManager ObjectsExtender::soundManager;

void EditableH3TextFile::AddLine(LPCSTR txt)
{
    this->text.Add(txt);
}
// int ObjectsExtender::AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player)
//{
//	return 0;
// }
// BOOL ObjectsExtender::HeroMapItemVisitFunction(HookContext* c, const H3Hero* currentHero, const H3MapItem* mapItem,
// const BOOL isPlayer, const BOOL skipMapMessage)
//{
//	return 0;
// }
ObjectsExtender::ObjectsExtender(PatcherInstance *pi) : IGamePatch(pi)
{

    CreatePatches();
    extenders.emplace_back(this);
}

// call atoi for hte first txt file
// int __stdcall ObjectsExtender::LoadObjectsTxt(HiHook* h, const DWORD data)

_LHF_(ObjectsExtender::LoadObjectsTxt)
{
    // check if there are any object properties extenders
    if (additionalProperties.size())
    {
        // get objects added list
        EditableH3TextFile *objectTxt = *reinterpret_cast<EditableH3TextFile **>(c->ebp + 0x8);

        // copy original objects added list into set
        std::set<LPCSTR> objectsSet(objectTxt->begin(), objectTxt->end());

        UINT32 newProperties = 0;
        // iterate each added property
        for (auto &prop : additionalProperties)
        {
            // if possible to insert
            if (objectsSet.insert(prop.c_str()).second)
            {
                // add that property into main objects list
                objectTxt->AddLine(prop.c_str()); // add new txt entry
                // increase added objects number
                newProperties++;
            }
        }

        c->eax += newProperties;
    }

    return EXEC_DEFAULT;
}

void __stdcall OnWogObjectHint(Era::TEvent *e)
{
}

ObjectsExtender::~ObjectsExtender()
{
    // extenders.erase(this);
    additionalProperties.clear();
}

void ObjectsExtender::LoadMapObjectPropertiesByTypeSubtypes() noexcept
{
    bool readSuccess = false;

    // load properties for direct type/subtype objects
    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    { // iterate all the objects types entries

        const int maxSubtype =
            objType == eObject::ARTIFACT || objType == eObject::CREATURE_GENERATOR1 ? limits::EXTENDED : limits::COMMON;

        for (size_t objSubtype = 0; objSubtype < maxSubtype; objSubtype++)
        {
            int propertyIdCounter = 0;

            do
            {
                LPCSTR str = EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.properties.%d", objType,
                                                          objSubtype, propertyIdCounter++)
                                             .String(),
                                         readSuccess);
                if (readSuccess)
                {
                    additionalProperties.emplace_back(str);
                }
            } while (readSuccess);
        }
    }
}

void ObjectsExtender::LoadMapObjectPropertiesFromLoadedMods() noexcept
{

    std::vector<std::string> modList;
    modList::GetEraModList(modList);

    for (const auto &modName : modList)
    {

        bool readSuccess = false;

        int propertyIdCounter = 0;

        do
        {
            LPCSTR str = EraJS::read(
                H3String::Format("RMG.%s.properties.%d", modName.c_str(), propertyIdCounter++).String(), readSuccess);
            if (readSuccess)
            {
                additionalProperties.emplace_back(str);
            }
        } while (readSuccess);
    }
    /* LPCSTR loooSoundName =
         EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.sound.loop", creatureBankId).String(), trSuccess);
     soundManager.loopSoundNames.emplace_back(trSuccess ? loooSoundName : h3_NullString);*/
}

void __stdcall LoopSoundManager::OnGameLeave(Era::TEvent *event)
{

    // if (instance->soundManager.loopSoundChanged)
    {
        // P_AdventureManager->loopSounds[7] = soundManager.defaultWav;
        // soundManager.defaultWav = nullptr;
        // soundManager.loopSoundChanged = false;
    }
}

int __stdcall LoopSoundManager::AdvMgr_MapItem_Select_Sound(HiHook *h, H3AdventureManager *adv, const int x,
                                                            const int y, const int z)
{

    H3MapItem *mapItem = adv->GetMapItem(x, y, z);

    int result = THISCALL_4(int, h->GetDefaultFunc(), adv, x, y, z);

    if (result == -1 && mapItem->objectType == eObject::CREATURE_BANK)
    {
    }

    const int crBankId = cbanks::CreatureBanksExtender::GetCreatureBankId(mapItem->objectType, mapItem->objectSubtype);
    // if (crBankId >= cbanks::CreatureBanksExtender::Get().defaultBanksNumber &&
    // Get().soundManager.loopSoundNames[crBankId] != h3_NullString)
    //{

    //    if (!Get().soundManager.loopSounds[crBankId])
    //        Get().soundManager.loopSounds[crBankId] = H3WavFile::Load(Get().soundManager.loopSoundNames[crBankId]);

    //    P_AdventureManager->loopSounds[7] = Get().soundManager.loopSounds[crBankId];
    //    result = 7;
    //    Get().soundManager.loopSoundChanged = true;

    //    return result;
    //}
    // else if (Get().soundManager.loopSoundChanged)
    //{
    //    if (!Get().soundManager.defaultWav)
    //        Get().soundManager.defaultWav = H3WavFile::Load("LoopCave.wav");
    //    P_AdventureManager->loopSounds[7] = Get().soundManager.defaultWav; // H3WavFile::Load("LoopCave.wav");
    //    Get().soundManager.loopSoundChanged = false;
    //}
    // return result;

    return EXEC_DEFAULT;
}

void __stdcall ObjectsExtender::H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup)
{

    // get Additional Propertise  <-  R E W R I T E   L A T E R   W I T H   N L O H M A N

    LoadMapObjectPropertiesByTypeSubtypes();

    LoadMapObjectPropertiesFromLoadedMods();

    // load additional unique objects properties from each loaded mod json key

    for (auto &obj : additionalRmgObjects)
    {
        //	setup->objectLists[obj.type][obj.subtype] = obj;
    }

    // Get All The Extenders we have

    for (auto &extender : extenders)
    {
        // call additional data loading from json
        // we call it once to later add into general objects.txt list
        // extender->GetObjectPreperties();
    }

    H3WavFile *ptr = nullptr;
    soundManager.loopSounds.emplace_back(ptr);

    // call native fucntion to load objects.txt (0x515038)
    THISCALL_1(void, h->GetDefaultFunc(), setup);

    // create max subtype value for all object gens
    INT16 maxSubtypes[h3::limits::OBJECTS] = {};

    auto objList = setup->objectLists;

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
        }
    }

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

bool RMGObjectSetable::operator<(const RMGObjectSetable &other) const
{
    return type != other.type ? type < other.type : subtype < other.subtype;
}
//
void ObjectsExtender::AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjecsList)
{
    // check if there are any object properties extenders
    if (extenders.size())
    {
        std::set<RMGObjectSetable> objectsSet;
        // create set of the objects to add only unique objects
        for (auto rmgObj : *rmgObjecsList)
        {
            objectsSet.insert({rmgObj->type, rmgObj->subtype});
        }
        // iterate all extenders container
        for (auto &info : additionalRmgObjects)
        {
            if (objectsSet.insert({info.type, info.subtype}).second)

            // iterate each added RMG INFO
            {
                for (auto &extender : extenders)

                // check if it is possible to add object into the list
                {

                    // if yes then create obj gen
                    H3RmgObjectGenerator *objGen = extender->CreateRMGObjectGen(info);
                    // and return to add into the list
                    if (objGen)
                    {
                        rmgObjecsList->Push(objGen);
                    }
                }
            }
        }
    }
}

_LHF_(ObjectsExtender::H3AdventureManager__ObjectVisit_SoundPlay) // (HiHook* h, const int objType, const int objSetup)
{
    // before player visits object
    // get object
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->ebx))
    {
        bool readSuccess = false;

        // try to read sound file name from ERA js
        H3String soundFileName = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.sound.enter", mapItem->objectType, mapItem->objectSubtype)
                .String(),
            readSuccess);
        // if there is entry
        if (readSuccess)
        {
            //  and filename isn't empty
            if (!soundFileName.Empty())
                // play sound
                P_SoundManager->PlaySoundAsync(soundFileName.String());
            // remove pusheed argumnets before hook
            c->esp += 8;
            // set new return address
            c->return_address = 0x4AA75C;
            // jump after native function
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
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

        const H3Hero *currentHero = reinterpret_cast<H3Hero *>(c->ebx);
        const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);

        INT aiResWeight = 0;

        for (const auto &extender : extenders)
        {
            if (extender->SetAiMapItemWeight(mapItem, currentHero, player, aiResWeight))
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
        _PI->WriteLoHook(0x4AA757, H3AdventureManager__ObjectVisit_SoundPlay);

        // _PI->WriteLoHook(0x40C5A1, H3AdventureManager__GetPyramidObjectHoverHint);
        //_PI->WriteLoHook(0x414F66, H3AdventureManager__GetPyramidObjectClickHint);
        _pi->WriteLoHook(0x04C0A5F, Game__NewGameObjectIteration);
        _pi->WriteLoHook(0x04C8847, Game__NewWeekObjectIteration);

        _PI->WriteLoHook(0x4A819C, H3AdventureManager__ObjectVisit);

        _PI->WriteLoHook(0x40D052, H3AdventureManager__GetDefaultObjectHoverHint); // mouse over hint
        _PI->WriteLoHook(0x415999, H3AdventureManager__GetDefaultObjectClickHint); // rmc hint

        _PI->WriteLoHook(0x528559, AIHero_GetObjectPosWeight); // AI object visit stuff

        _PI->WriteHiHook(0x4EE01C, THISCALL_, H3GameMainSetup__LoadObjects);

        //   _PI->WriteHiHook(0x418580, THISCALL_, LoopSoundManager::AdvMgr_MapItem_Select_Sound);
        // Era::RegisterHandler(LoopSoundManager::OnGameLeave, "OnGameLeave");
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
BOOL ObjectsExtender::SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                         int &aiResWeight) const noexcept
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
} // namespace extender
