#include "../pch.h"
namespace wog

{
std::array<int, extender::limits::COMMON> WoGObjectsExtender::WoGObjectOptionsIds;

WoGObjectsExtender::WoGObjectsExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.WoGObjectsExternder.daemon_n"))
{

    CreatePatches();
}

WoGObjectsExtender::~WoGObjectsExtender()
{
}

void WoGObjectsExtender::CreatePatches()
{
    if (!m_isInited)
    {

        //_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        //_pi->WriteLoHook(0x40D858, Shrine__AtGetName);
        //_pi->WriteLoHook(0x40DA24, Shrine__AtGetHint);

        m_isInited = true;
    }
}

H3RmgObjectGenerator *WoGObjectsExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{

    if (objectInfo.type == eObject::PYRAMID && objectInfo.subtype > 0)
    {
        return extender::ObjectExtenderManager::CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

void WoGObjectsExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes) noexcept
{
    const int maxSubtype = maxSubtypes[WOG_OBJECT_TYPE];
    for (size_t i = 1; i < maxSubtype; i++)
    {
        WoGObjectOptionsIds[i] = EraJS::readInt(H3String::Format(jsonKeyFormat, WOG_OBJECT_TYPE, i).String());
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

WoGObjectsExtender* WoGObjectsExtender::instance = nullptr;

WoGObjectsExtender& WoGObjectsExtender::Get()
{
    if (!instance)
        instance = new WoGObjectsExtender();
    return *instance;
}

} // namespace wog
