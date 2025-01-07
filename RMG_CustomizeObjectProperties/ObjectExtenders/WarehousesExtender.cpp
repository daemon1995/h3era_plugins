#include "../pch.h"
namespace warehouses
{

WarehousesExtender::WarehousesExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.WarehousesExtender.daemon_n"))
{

    CreatePatches();
}

WarehousesExtender::~WarehousesExtender()
{
}

void ShowMessage(const H3MapItem *mapItem, const int resType, const int resNum)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    if (skipMapMessage)
    {
        if (resNum < 1)
        {
            objName.Append(
                EraJS::read(H3String::Format("RMG.objectGeneration.%d.text.visited", mapItem->objectType).String()));
        }

        THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), resType, resNum);
    }
    else
    {
        objName.Append(EraJS::read(
            resNum > 0 ? H3String::Format("RMG.objectGeneration.%d.text.visit", mapItem->objectType).String()
                       : H3String::Format("RMG.objectGeneration.%d.text.visited", mapItem->objectType).String()));

        H3PictureCategories pics(ePictureCategories(resType), resNum);
        h3::libc::sprintf(h3_TextBuffer, "%s", objName.String());
        H3Messagebox::Show(pics);
    }
}

BOOL WarehousesExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero,
                                               const H3Player *activePlayer, const BOOL isRightClick) const noexcept
{
    if (const auto warehouse = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        const bool isVistedByPlayer = THISCALL_2(bool, 0x529B70, mapItem, H3HumanID::Get());
        if (isVistedByPlayer)
        {
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ", P_GeneralText->GetText(354));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

BOOL WarehousesExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{
    if (const auto warehouse = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        *warehouse = {};
        const UINT resType = mapItem->objectSubtype;
        warehouse->resourceType = resType;
        // reset objSetup of the item
        warehouse->Reset();
        return true;
    }
    return false;
}
BOOL WarehousesExtender::InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept
{
    if (const auto warehouse = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        
        warehouse->Reset();
        return true;
    }
    return false;
}

BOOL WarehousesExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                      const BOOL isHuman) const noexcept
{

    if (auto objSetup = H3MapItemWarehouse::GetWarehouse(mapItem))
    {

        // set object as visited
        THISCALL_2(void, 0x4FC620, mapItem, hero->owner);

        const INT16 resType = objSetup->resourceType;
        const INT16 resNum = objSetup->resourceAmount ? resType == eResource::GOLD ? objSetup->resourceAmount * 500
                                                                                   : objSetup->resourceAmount
                                                      : -1;

        // if object has resources
        if (resNum > 0)
        {
            // remove resources from the object
            objSetup->resourceAmount = 0;

            if (resType < h3::limits::RESOURCES + 1) // mithrill just in case
            {
                // add resourcse
                THISCALL_3(void, 0x04E3870, hero, resType, resNum);
            }
        }
        if (isHuman)
        {
            ShowMessage(mapItem, resNum > 0 ? resType : -1, resNum > 0 ? resNum : -1);
        }
        return true;
    }

    return false;
}

BOOL WarehousesExtender::SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *player,
                                            int &aiMapItemWeight) const noexcept
{

    // for hota warehouse
    if (auto objSetup = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        const bool isVistedByPlayer = THISCALL_2(bool, 0x529B70, mapItem, H3CurrentPlayerID::Get());

        // if AI player didnt' visit or there is resources
        if (!isVistedByPlayer || objSetup->resourceAmount)
        {

            UINT resType = objSetup->resourceType;
            if (resType >= h3::limits::RESOURCES)
            {
                resType = eResource::GOLD;
            }
            const INT16 resNum = resType == eResource::GOLD ? objSetup->resourceAmount * 500 : objSetup->resourceAmount;

            aiMapItemWeight = static_cast<int>(player->resourceImportance[resType] * resNum); // static_cast<double*>()
        }
        return true;
    }

    return false;
}

H3RmgObjectGenerator *WarehousesExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == WAREHOUSE_OBJECT_TYPE)
    {
        return ObjectsExtender::CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

void WarehousesExtender::CreatePatches()
{
    if (!m_isInited)
    {
        m_isInited = true;
    }
}

inline H3MapItemWarehouse *H3MapItemWarehouse::GetWarehouse(H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == WAREHOUSE_OBJECT_TYPE)
    {
        return reinterpret_cast<H3MapItemWarehouse *>(&mapItem->setup);
    }

    return nullptr;
}

void H3MapItemWarehouse::Reset()
{
    resourceAmount = 0;
    visited = 0;
    switch (resourceType)
    {
    case eResource::WOOD:
    case eResource::ORE:
        resourceAmount = 10;
        break;
    case eResource::MERCURY:
    case eResource::SULFUR:
    case eResource::CRYSTAL:
    case eResource::GEMS:
        resourceAmount = 6;

        break;
    case eResource::GOLD:
        resourceAmount = 4; // *500 when used
        break;
        ////mithril
    case 7:
        resourceAmount = 2; //
        break;
    default:
        break;
    }
}

WarehousesExtender &WarehousesExtender::Get()
{
    static WarehousesExtender _instance;
    return _instance;
}

} // namespace warehouses
