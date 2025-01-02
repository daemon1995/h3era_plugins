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

_LHF_(WarehousesExtender::AIHero_GetObjectPosWeight)
{
    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->esi))
    {
        // for hota warehouse
        if (auto data = H3MapItemWarehouse::GetWarehouse(mapItem))
        {

            const bool isVistedByPlayer = THISCALL_2(bool, 0x529B70, mapItem, H3CurrentPlayerID::Get());

            // if AI player didnt' visit or there is resources
            if (!isVistedByPlayer || data->resourceAmount)
            {

                UINT resType = data->resourceType;
                if (resType >= h3::limits::RESOURCES)
                {
                    resType = eResource::GOLD;
                }
                const INT16 resNum = resType == eResource::GOLD ? data->resourceAmount * 500 : data->resourceAmount;

                const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);

                const int aiResWeight =
                    static_cast<int>(player->resourceImportance[resType] * resNum); // static_cast<double*>()
                if (aiResWeight)
                {
                    c->eax = aiResWeight;
                    c->return_address = 0x05285A1;
                    return NO_EXEC_DEFAULT;
                }
            }
        }
    }

    return EXEC_DEFAULT;
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
_LHF_(WarehousesExtender::H3AdventureManager__ObjectVisit)
{
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {
        if (auto data = H3MapItemWarehouse::GetWarehouse(mapItem))
        {
            const H3Hero *hero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);

            // set object as visited
            THISCALL_2(void, 0x4FC620, mapItem, hero->owner);

            const char isHuman = CharAt(c->ebp + 0x14);
            const INT16 resType = data->resourceType;
            const INT16 resNum = data->resourceAmount
                                     ? resType == eResource::GOLD ? data->resourceAmount * 500 : data->resourceAmount
                                     : -1;

            // if object has resources
            if (resNum > 0)
            {
                // remove resources from the object
                data->resourceAmount = 0;

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
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(WarehousesExtender::Game__NewGameObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);
    if (const auto warehouse = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        *warehouse = {};
        const UINT resType = mapItem->objectSubtype;
        warehouse->resourceType = resType;
        // reset data of the item
        warehouse->Reset();
    }

    return EXEC_DEFAULT;
}
_LHF_(WarehousesExtender::Game__NewWeekObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);
    if (const auto warehouse = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        // reset data of the item
        warehouse->Reset();
    }

    return EXEC_DEFAULT;
}

BOOL ShowHint(LoHook *h, HookContext *c, LPCSTR delimiter)
{
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);
    if (const auto warehouse = H3MapItemWarehouse::GetWarehouse(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        const bool isVistedByPlayer = THISCALL_2(bool, 0x529B70, mapItem, H3HumanID::Get());
        if (isVistedByPlayer)
        {
            sprintf(h3_TextBuffer, "%s%s", delimiter, P_GeneralText->GetText(354));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        c->edi = (int)h3_TextBuffer;
        c->return_address = h->GetAddress() + 7;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

_LHF_(WarehousesExtender::H3AdventureManager__GetDefaultObjectClickHint)
{
    return ShowHint(h, c, "\n\n");
}
_LHF_(WarehousesExtender::H3AdventureManager__GetDefaultObjectHoverHint)
{
    return ShowHint(h, c, " ");
}

void WarehousesExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        _pi->WriteLoHook(0x04C0A5F, Game__NewGameObjectIteration);
        _pi->WriteLoHook(0x04C8847, Game__NewWeekObjectIteration);

        _PI->WriteLoHook(0x4A819C, H3AdventureManager__ObjectVisit);
        _PI->WriteLoHook(0x528559, AIHero_GetObjectPosWeight);

        _PI->WriteLoHook(0x40D052, H3AdventureManager__GetDefaultObjectHoverHint); // mouse over hint
        _PI->WriteLoHook(0x415999, H3AdventureManager__GetDefaultObjectClickHint); // rmc hint

        //_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        //_pi->WriteLoHook(0x40D858, Shrine__AtGetName);

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
