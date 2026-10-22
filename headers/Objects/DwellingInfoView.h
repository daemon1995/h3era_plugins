#pragma once
#include "DwellingConfig.h"

namespace dwellings
{
// A bounded, read-only view. Its counts describe the published tables, never
// the largest subtype found in JSON or in the map's graphics definitions.
struct DwellingInfoView
{
    int type = -1;
    int creatureCount = 0;
    int nameCount = 0;
    const int *creatures = nullptr;
    const char *const *names = nullptr;
    const char *defaultName = nullptr;
};
struct DwellingInfoData
{
    CreatureSlots creatures = EMPTY_CREATURES;
    int slotCount = 0;
    const char *name = nullptr;
};
inline bool ReadDwellingInfo(const DwellingInfoView &view, int subtype, DwellingInfoData &result) noexcept
{
    if ((view.type != SINGLE_TYPE && view.type != MULTIPLE_TYPE) || !view.creatures ||
        view.creatureCount <= 0 || view.creatureCount > MAX_CAPACITY || subtype < 0 || subtype >= view.creatureCount ||
        view.nameCount < 0 || view.nameCount > MAX_CAPACITY)
        return false;
    DwellingInfoData value;
    value.slotCount = view.type == SINGLE_TYPE ? 1 : 4;
    for (int slot = 0; slot < value.slotCount; ++slot)
        value.creatures[slot] = view.creatures[subtype * value.slotCount + slot];
    value.name = view.names && subtype < view.nameCount && view.names[subtype] ? view.names[subtype] : view.defaultName;
    result = value;
    return true;
}
} // namespace dwellings
