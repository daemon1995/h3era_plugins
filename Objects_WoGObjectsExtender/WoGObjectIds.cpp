#include "WoGObjectIds.h"

namespace wog
{
namespace objects
{
namespace
{
const Descriptor kDescriptors[] = {{VENUS_FOUNTAIN, OPT_POTIONS, 600, true, true},
                                   {MYSTERIOUS_DWELLING, OPT_MYSTERIOUS_DWELLING, 2500, true, true},
                                   {JUNK_MERCHANT, OPT_JUNK_MERCHANT, 1200, true, true},
                                   {ARCANE_TOWER, OPT_ARCANE_TOWER, 900, true, true},
                                   {MAGIC_AIR_MUSHROOM, OPT_MAGIC_MUSHROOMS, 650, true, true},
                                   {DEATH_CHAMBER, OPT_DEATH_CHAMBER, 3000, true, true},
                                   {MAGIC_WATER_MUSHROOM, OPT_MAGIC_MUSHROOMS, 650, true, true},
                                   {ALTAR_OF_TRANSFORMATION, OPT_ALTAR, 1100, true, true},
                                   {SUMMONING_STONES, OPT_SUMMONING_STONES, 1800, true, true},
                                   {FORGOTTEN_SHRINE, OPT_FORGOTTEN_SHRINE, 750, true, true},
                                   {MAGIC_EARTH_MUSHROOM, OPT_MAGIC_MUSHROOMS, 650, true, true},
                                   {ALMS_HOUSE, OPT_ALMS_HOUSE, 2500, true, true},
                                   {MAGIC_FIRE_MUSHROOM, OPT_MAGIC_MUSHROOMS, 650, true, true},
                                   {BATTLE_ACADEMY, OPT_BATTLE_ACADEMY, 1800, true, true},
                                   {PALACE_OF_DREAMS, OPT_PALACE_OF_DREAMS, 450, true, true},
                                   {BERSERKER_FOUNTAIN, OPT_POTIONS, 600, true, true},
                                   {LIVING_SKULL, OPT_LIVING_SKULL, 0, true, true},
                                   {ADVENTURE_CAVE, OPT_ADVENTURE_CAVE, 2000, true, true},
                                   {SCHOOL_OF_WIZARDRY, OPT_SCHOOL_OF_WIZARDRY, 1500, true, true},
                                   {EMERALD_TOWER, OPT_EMERALD_TOWER, 1600, true, true},
                                   {MIRROR_OF_HOME_WAY, OPT_MIRROR, 0, true, true}};
}

const Descriptor *DescriptorsBegin() noexcept
{
    return kDescriptors;
}
const Descriptor *DescriptorsEnd() noexcept
{
    return kDescriptors + sizeof(kDescriptors) / sizeof(kDescriptors[0]);
}
const Descriptor *FindDescriptor(const int subtype) noexcept
{
    for (auto p = DescriptorsBegin(); p != DescriptorsEnd(); ++p)
        if (p->subtype == subtype)
            return p;
    return nullptr;
}
bool IsPortedSubtype(const int subtype) noexcept
{
    return FindDescriptor(subtype) != nullptr;
}

} // namespace objects
} // namespace wog
