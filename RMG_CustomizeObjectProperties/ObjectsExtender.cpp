#include "pch.h"

namespace extender
{

std::set<ObjectsExtender *> ObjectsExtender::extenders;
std::vector<std::string> ObjectsExtender::additionalProperties;
std::vector<RMGObjectInfo> ObjectsExtender::additionalRmgObjects;

void EditableH3TextFile::AddLine(LPCSTR txt)
{
    this->text.Add(txt);
}
// int ObjectsExtender::AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player)
//{
//	return 0;
// }
// BOOL ObjectsExtender::HeroMapItemVisitFunction(HookContext* c, const H3Hero* hero, const H3MapItem* mapItem, const
// BOOL isPlayer, const BOOL skipMapMessage)
//{
//	return 0;
// }
ObjectsExtender::ObjectsExtender(PatcherInstance *pi) : IGamePatch(pi)
{

    CreatePatches();
    extenders.insert(this);
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
    extenders.erase(this);
    additionalProperties.clear();
}

void __stdcall ObjectsExtender::H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup)
{

    // get Additional Propertise  <-  R E W R I T E   L A T E R   W I T H   N L O H M A N

    for (size_t objType = 0; objType < H3_MAX_OBJECTS; objType++)
    { // iterate all the objects types entries

        const int maxSubtype = objType == eObject::ARTIFACT || objType == eObject::CREATURE_GENERATOR1 ? 1024 : 255;

        for (size_t objSubtype = 0; objSubtype < maxSubtype; objSubtype++)
        {
            bool readSuccess = false;
            int propertyIdCounter = 0;
            ;

            do
            {
                //				std::string str =
                //EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.properties.%d", objType, objSubtype,
                //propertyIdCounter++).String(), readSuccess);
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

    // Get All The Extenders we have

    for (auto &extender : extenders)
    {
        // call additional data loading from json
        // we call it once to later add into general objects.txt list
        // extender->GetObjectPreperties();
    }

    // call native fucntion to load objects.txt (0x515038)
    THISCALL_1(void, h->GetDefaultFunc(), setup);

    // create max subtype value for all object gens
    INT16 maxSubtypes[H3_MAX_OBJECTS] = {};

    auto objList = setup->objectLists;

    for (size_t objType = 0; objType < H3_MAX_OBJECTS; objType++)
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

            // if (objectSubtypeValue || objectSubtypeDensity)
            //{
            //	RMGObjectInfo rmgObjectInfo(objType, objSubtype);
            //	rmgObjectInfo.value = objectSubtypeValue;
            //	rmgObjectInfo.density = objectSubtypeDensity;
            //	additionalRmgObjects.emplace_back(rmgObjectInfo);
            // }
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

H3RmgObjectGenerator *CreateRmgObjectGen(const RMGObjectInfo &info)
{

    // iterate vector with objects Info and generate
    H3RmgObjectGenerator *objGen = nullptr;
    // if (info.Clamp())
    {
        switch (info.type)
        {
        case eObject::SHRINE_OF_MAGIC_INCANTATION:
        case eObject::SHRINE_OF_MAGIC_GESTURE:
        case eObject::SHRINE_OF_MAGIC_THOUGHT:

            objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1);

            objGen = THISCALL_3(H3RmgObjectGenerator *, 0x534EC0, objGen, info.type, info.value);
            objGen->subtype = info.subtype;

            break;
        case eObject::CREATURE_BANK:
        case eObject::PYRAMID:
            // hota's warehouses
        case warehouses::WAREHOUSE_OBJECT_TYPE:

            objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1);
            THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, info.type, info.subtype, info.value, info.density);
            break;
        case eObject::CREATURE_GENERATOR1:

            // objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1);
            // THISCALL_5(H3RmgObjectGenerator*, 0x534640, objGen, info.type, info.subtype, info.value, info.density);
            //// set dwellings vTable
            //*reinterpret_cast<uintptr_t*>(objGen) = 0x640BC8;
            break;

        default:
            break;
        }
    }
    return objGen;
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
        for (auto &extender : extenders)
        {
            // iterate each added RMG INFO
            for (auto &info : extender->additionalRmgObjects)
            {
                // check if it is possible to add object into the list
                if (objectsSet.insert({info.type, info.subtype}).second)
                {
                    // if yes then create obj gen
                    H3RmgObjectGenerator *objGen = CreateRmgObjectGen(info);
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

_LHF_(H3AdventureManager__GetPyramidObjectHoverHint)
{
    bool readSucces = false;

    const H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);
    LPCSTR objName = EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.name", mapItem->objectType, mapItem->objectSubtype).String(),
        readSucces);

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

void ObjectsExtender::CreatePatches()
{
    // before any of extenders is inited
    if (extenders.empty())
    {
        // Era::RegisterHandler(OnWogObjectHint);
        _PI->WriteLoHook(0x515038, LoadObjectsTxt);
        _PI->WriteLoHook(0x4AA757, H3AdventureManager__ObjectVisit_SoundPlay);

        //_PI->WriteLoHook(0x40C5A1, H3AdventureManager__GetPyramidObjectHoverHint);
        //_PI->WriteLoHook(0x414F66, H3AdventureManager__GetPyramidObjectClickHint);

        _PI->WriteHiHook(0x4EE01C, THISCALL_, H3GameMainSetup__LoadObjects);

        //	Era::RegisterHandler(OnWogObjectHint, "OnWogObjectHint");
    }
}
void ObjectsExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
}
} // namespace extender
