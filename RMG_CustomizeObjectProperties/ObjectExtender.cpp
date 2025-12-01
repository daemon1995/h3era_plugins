#include "pch.h"

namespace extender
{

ObjectExtender::ObjectExtender()
{
    ObjectExtenderManager::Get()->AddExtender(this);
}

// call atoi for hte first txt file
// int __stdcall ObjectExtender::LoadObjectsTxt(HiHook* h, const DWORD data)

ObjectExtender::~ObjectExtender()
{
}

// H3RmgObjectGenerator *ObjectExtender::CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &objectInfo) noexcept
//{
//     H3RmgObjectGenerator *objGen = nullptr;
//     if (objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1))
//     {
//         THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, objectInfo.type, objectInfo.subtype, objectInfo.value,
//                    objectInfo.density);
//     }
//     return objGen;
// }

H3RmgObjectGenerator *ObjectExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    return ObjectExtenderManager::CreateDefaultH3RmgObjectGenerator(objectInfo);
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

void ObjectExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
}
BOOL ObjectExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                           const BOOL isRightClick) const noexcept
{
    return false;
}
BOOL ObjectExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *activePlayer,
                                        int &aiMapItemWeight, int *moveDistance, const H3Position pos) const noexcept
{
    return false;
}
BOOL ObjectExtender::RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                                 const H3String &defaltText) noexcept
{
    return false;
}
BOOL ObjectExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{
    return false;
}
BOOL ObjectExtender::InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept
{
    return false;
}
BOOL ObjectExtender::VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                                  const BOOL isHuman) const noexcept
{
    return false;
}

} // namespace extender
