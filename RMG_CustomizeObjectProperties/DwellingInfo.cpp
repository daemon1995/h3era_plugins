#include "DwellingInfo.h"
#include "../headers/Era/GameDwellingInfo.hpp"

#pragma comment(linker, "/EXPORT:GetDwellingExportInfo=_GetDwellingExportInfo@12")
#pragma comment(linker, "/EXPORT:AppendDwellingRmgHint=_AppendDwellingRmgHint@12")
#pragma comment(linker, "/EXPORT:CreateDwellingRmgGenerator=_CreateDwellingRmgGenerator@16")
DllExport BOOL __stdcall GetDwellingExportInfo(int type, int subtype, EraJS::DwellingExportInfo *info)
{
    return EraJS::GameDwellings::Export(type, subtype, info);
}

DllExport BOOL __stdcall AppendDwellingRmgHint(int type, int subtype, H3String *text)
{
    return text && rmgDwellings::AppendHint(type, subtype, *text);
}

DllExport H3RmgObjectGenerator *__stdcall CreateDwellingRmgGenerator(int type, int subtype, int value, int density)
{
    if ((type != dwellings::SINGLE_TYPE && type != dwellings::MULTIPLE_TYPE) || !object_config::IsObjectSubtype(subtype))
        return nullptr;
    const RMGObjectProperties record(type, subtype, TRUE, -1, -1, value, density);
    return rmgDwellings::CreateGenerator(record);
}

namespace rmgDwellings
{
LPCSTR GetName(int type, int subtype) noexcept
{
    EraJS::DwellingExportInfo info;
    return EraJS::GameDwellings::Read(type, subtype, info) && info.name ? info.name : H3ObjectName::Get()[type];
}

BOOL AppendHint(int type, int subtype, H3String &text) noexcept
{
    EraJS::DwellingExportInfo dwelling;
    if (!EraJS::GameDwellings::Read(type, subtype, dwelling)) return FALSE;
    H3String hint;
    int town = eTown::NEUTRAL;
    const int creatureCount = IntAt(0x4A1657);
    for (int slot = 0; slot < dwelling.slotCount; ++slot)
        if (dwellings::IsCreature(dwelling.creatures[slot], creatureCount))
        {
            const auto &info = P_CreatureInformation[dwelling.creatures[slot]];
            if (hint.Empty()) town = info.town;
            hint += H3String::Format("%d {~>%s:0:%d valign=bottom}", info.grow,
                                     NH3Dlg::Assets::CREATURE_SMALL, dwelling.creatures[slot] + 2);
        }
    if (!hint.Empty())
    {
        if (town != eTown::NEUTRAL)
            hint = H3String::Format("{~>%s:0:%d valign=bottom} ", NH3Dlg::Assets::TOWN_SMALL, town * 2 + 2) + hint;
        text += extendersManager::ObjectExtenderManager::DLG_HORIZONTAL_GAP + hint;
    }
    return TRUE;
}

void InitDefaultValues() noexcept
{
    for (int type : {dwellings::SINGLE_TYPE, dwellings::MULTIPLE_TYPE})
        for (auto &record : RMGObjectInfo::defaultRMGObjectsInfoByType[type])
        {
            if (record.value != RMGObjectProperties::UNDEFINED) continue;
            EraJS::DwellingExportInfo info;
            if (!EraJS::GameDwellings::Read(type, record.subtype, info)) continue;
            int value = 0;
            bool valid = false;
            for (int slot = 0; slot < info.slotCount; ++slot)
                if (dwellings::IsCreature(info.creatures[slot], IntAt(0x4A1657)))
                {
                    value += P_CreatureInformation[info.creatures[slot]].aiValue;
                    valid = true;
                }
            if (valid || type == dwellings::MULTIPLE_TYPE) record.value = value;
        }
}

H3RmgObjectGenerator *CreateGenerator(const RMGObjectProperties &record) noexcept
{
    EraJS::DwellingExportInfo info;
    if (!EraJS::GameDwellings::Read(record.type, record.subtype, info)) return nullptr;
    bool valid = false;
    for (int slot = 0; slot < info.slotCount; ++slot)
        valid |= dwellings::IsCreature(info.creatures[slot], IntAt(0x4A1657));
    if (!valid) return nullptr;
    auto *generator = extender::ObjectExtender::CreateDefaultH3RmgObjectGenerator(record);
    if (generator) generator->vTable = reinterpret_cast<H3RmgObjectGenerator::VTable *>(0x640BC8);
    return generator;
}
} // namespace rmgDwellings
