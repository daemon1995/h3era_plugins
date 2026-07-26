#include "WoGObjectsExtender.h"

namespace wog
{
std::array<int, extendersManager::limits::COMMON> WoGObjectsExtender::WoGObjectOptionsIds;
WoGObjectsExtender *WoGObjectsExtender::instance = nullptr;

WoGObjectsExtender::WoGObjectsExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.WoGObjectsExternder.daemon_n"))
{
    CreatePatches();
    objectType = extender::WOG_OBJECT_TYPE;
}

void WoGObjectsExtender::CreatePatches()
{
    if (!m_isInited)
    {
        m_isInited = true;
    }
}

void WoGObjectsExtender::AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) noexcept
{
    const int maxSubtype = maxSubtypes[WOG_OBJECT_TYPE];
    objectSubtypes.Resize(maxSubtype);
    for (size_t i = 1; i <= maxSubtype; i++)
    {
        WoGObjectOptionsIds[i] = EraJS::readInt(H3String::Format(jsonKeyFormat, WOG_OBJECT_TYPE, i).String());
        objectSubtypes[i - 1] = i;
    }
}

BOOL WoGObjectsExtender::IsWoGObject(const H3MapItem *mapItem) noexcept
{
    return mapItem && mapItem->objectType == WOG_OBJECT_TYPE && mapItem->objectSubtype != 0;
}

BOOL WoGObjectsExtender::IsWoGObject(const H3RmgObjectGenerator *p_ObjGen) noexcept
{
    return p_ObjGen && p_ObjGen->type == WOG_OBJECT_TYPE && p_ObjGen->subtype != 0;
}

BOOL WoGObjectsExtender::WoGObjectHasOptionEnabled(const H3RmgObjectGenerator *p_ObjGen) noexcept
{
    return DwordAt(WOG_OPTIONS_ARRAY + WoGObjectOptionsIds[p_ObjGen->subtype] * 4);
}

WoGObjectsExtender &WoGObjectsExtender::Get()
{
    if (!instance)
        instance = new WoGObjectsExtender();
    return *instance;
}

} // namespace wog
