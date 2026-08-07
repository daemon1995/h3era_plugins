#include "WoGObjectsExtender.h"

namespace wog
{
std::array<int, 255> WoGObjectsExtender::WoGObjectOptionsIds;
WoGObjectsExtender *WoGObjectsExtender::instance = nullptr;

WoGObjectsExtender::WoGObjectsExtender() : ObjectExtender(_PI)
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

H3RmgObjectGenerator *WoGObjectsExtender::CreateRMGObjectGen(const extender::RMGObjectProperties &info,
                                                             const BOOL isPseudoGeneration) const noexcept
{
    if (!isPseudoGeneration && !WoGObjectHasOptionEnabled(info))
    {
        return nullptr;
    }
    return CreateDefaultH3RmgObjectGenerator(info);
}

BOOL WoGObjectsExtender::WoGObjectHasOptionEnabled(const extender::RMGObjectProperties &info) noexcept
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
