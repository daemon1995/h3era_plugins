#pragma once
#include "GameJsonTables.hpp"
#include "JsonObjectExport.hpp"
#include "../EraPluginsAPI/ObjectDataAPI.h"
#include "../Objects/DwellingInfoView.h"
#include <algorithm>
#include <cstddef>

namespace EraJS
{
namespace GameDwellings
{
constexpr const char *PLUGIN = "ERA_DwellingsExtender.era";
inline bool Readable(const void *data, size_t size) noexcept
{
    if (!data || !size) return false;
    const auto begin = reinterpret_cast<uintptr_t>(data);
    if (begin + size < begin) return false;
    auto address = begin;
    while (address < begin + size)
    {
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(reinterpret_cast<const void *>(address), &region, sizeof(region)) ||
            region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
        const auto end = reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize;
        if (end <= address) return false;
        address = end;
    }
    return true;
}
inline bool ReadNative(int type, int subtype, DwellingExportInfo &info) noexcept
{
    using namespace h3;
    using namespace dwellings;
    if ((type != SINGLE_TYPE && type != MULTIPLE_TYPE) || subtype < 0 || subtype >= MAX_CAPACITY) return false;
    DwellingInfoView view;
    view.type = type;
    view.defaultName = H3ObjectName::Get()[type];
    if (type == SINGLE_TYPE)
    {
        view.nameCount = GameTables::Dwelling1Capacity();
        view.creatureCount = (std::max)(view.nameCount, (std::max)(IntAt(0x5032BC) + 1, IntAt(0x539C77)));
        view.creatures = reinterpret_cast<const int *>(DwordAt(0x534CEA));
        view.names = H3DwellingNames1::Get();
    }
    else
    {
        view.nameCount = view.creatureCount = GameTables::Dwelling4Capacity();
        view.creatures = reinterpret_cast<const int *>(DwordAt(0x4B85B7));
        view.names = H3DwellingNames4::Get();
    }
    const int slots = type == SINGLE_TYPE ? 1 : 4;
    if (view.creatureCount <= 0 || view.creatureCount > MAX_CAPACITY || subtype >= view.creatureCount ||
        !Readable(view.creatures, size_t(view.creatureCount) * slots * sizeof(int))) return false;
    if (view.nameCount < 0 || view.nameCount > MAX_CAPACITY ||
        (view.nameCount && !Readable(view.names, size_t(view.nameCount) * sizeof(const char *)))) return false;
    DwellingInfoData data;
    if (!ReadDwellingInfo(view, subtype, data)) return false;
    info = DwellingExportInfo{};
    std::copy(data.creatures.begin(), data.creatures.end(), info.creatures);
    info.slotCount = data.slotCount;
    info.name = data.name;
    const int singleFrames = ByteAt(0x521A7F) == 0x83 && ByteAt(0x521A80) == 0xF8 ? ByteAt(0x521A81) : 0;
    info.frameCount = singleFrames ? (type == SINGLE_TYPE ? singleFrames : (std::max)(0, IntAt(0x520D1D) - singleFrames)) : 0;
    info.frame = subtype < info.frameCount ? subtype : 0;
    return true;
}
inline bool Read(int type, int subtype, DwellingExportInfo &info) noexcept
{
    if (type != dwellings::SINGLE_TYPE && type != dwellings::MULTIPLE_TYPE) return false;
    if (subtype < 0 || subtype >= dwellings::MAX_CAPACITY) return false;
    // Do not load a DLL as a side effect of displaying a hint. Missing/inactive
    // extenders fall back to the currently installed native table operands.
    const auto *api = era_services::Find<era_objects::DwellingApi>(era_objects::DWELLINGS);
    const auto read = api ? api->read : nullptr;
    if (read)
    {
        DwellingExportInfo value;
        if (read(type, subtype, &value) && value.slotCount == (type == dwellings::SINGLE_TYPE ? 1 : 4))
        {
            info = value;
            return true;
        }
    }
    return ReadNative(type, subtype, info);
}
inline void Initialize(const INT16 *limits, unsigned count) noexcept
{
    const auto *api = era_services::Find<era_objects::DwellingApi>(era_objects::DWELLINGS);
    const auto initialize = api ? api->initialize : nullptr;
    if (initialize) initialize(limits, count);
}
inline BOOL Export(int type, int subtype, DwellingExportInfo *info) noexcept
{
    if (!info || info->structSize < offsetof(DwellingExportInfo, image)) return FALSE;
    DwellingExportInfo result;
    if (!Read(type, subtype, result)) return FALSE;
    result.structSize = info->structSize;
    h3::libc::memcpy(info, &result, (std::min)(size_t(info->structSize), sizeof(result)));
    return TRUE;
}
} // namespace GameDwellings
} // namespace EraJS
