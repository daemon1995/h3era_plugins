#include "WoGObjectsExtender.h"

namespace wog
{
std::array<int, extendersManager::limits::COMMON> WoGObjectsExtender::WoGObjectOptionsIds;
WoGObjectsExtender *WoGObjectsExtender::instance = nullptr;

WoGObjectsExtender::WoGObjectsExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.WoGObjectsExternder.daemon_n"))
{
    CreatePatches();
}
void WoGObjectsExtender::AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) noexcept
{
    const int maxSubtype = maxSubtypes[WOG_OBJECT_TYPE];
    objectSubtypesInfo.Reserve(maxSubtype);
    for (size_t i = 1; i <= maxSubtype; i++)
    {
        WoGObjectOptionsIds[i] = EraJS::readInt(H3String::Format(jsonKeyFormat, WOG_OBJECT_TYPE, i).String());
        AddUniqueObjectInfo(WOG_OBJECT_TYPE, i);
    }
}

H3RmgObjectGenerator *WoGObjectsExtender::CreateRMGObjectGen(const RMGObjectProperties &info,
                                                             const BOOL isPseudoGeneration) const noexcept
{
    if (!isPseudoGeneration && !WoGObjectHasOptionEnabled(info))
    {
        return nullptr;
    }
    return CreateDefaultH3RmgObjectGenerator(info);
}

BOOL WoGObjectsExtender::IsWoGObject(const H3MapItem *mapItem) noexcept
{
    return mapItem && mapItem->objectType == WOG_OBJECT_TYPE && mapItem->objectSubtype != 0;
}

BOOL WoGObjectsExtender::IsWoGObject(const H3RmgObjectGenerator *p_ObjGen) noexcept
{
    return p_ObjGen && p_ObjGen->type == WOG_OBJECT_TYPE && p_ObjGen->subtype != 0;
}

BOOL WoGObjectsExtender::WoGObjectHasOptionEnabled(const RMGObjectProperties &info) noexcept
{
    return DwordAt(WOG_OPTIONS_ARRAY + WoGObjectOptionsIds[info.subtype] * 4);
}

WoGObjectsExtender &WoGObjectsExtender::Get()
{
    if (!instance)
        instance = new WoGObjectsExtender();
    return *instance;
}

} // namespace wog
