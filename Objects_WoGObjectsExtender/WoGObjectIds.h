#pragma once
#include "framework.h"

namespace wog { namespace objects {

enum Subtype : int
{
    MYSTERIOUS_DWELLING = 3,
    JUNK_MERCHANT = 8,
    ARCANE_TOWER = 9,
    MAGIC_AIR_MUSHROOM = 14,
    DEATH_CHAMBER = 15,
    MAGIC_WATER_MUSHROOM = 18,
    ALTAR_OF_TRANSFORMATION = 20,
    FORGOTTEN_SHRINE = 22,
    MAGIC_EARTH_MUSHROOM = 23,
    ALMS_HOUSE = 24,
    MAGIC_FIRE_MUSHROOM = 29,
    BATTLE_ACADEMY = 32,
    PALACE_OF_DREAMS = 33,
    LIVING_SKULL = 50,
    MARKET_OF_TIME = 51,
    ARTIFICER = 52,
    EMERALD_TOWER = 65,
    MIRROR_OF_HOME_WAY = 66
};

// WoG option IDs used only to gate the legacy 63/subtype identity.
// Native ERA 141/subtype objects are controlled by RMG.objectGeneration.*.enabled.
enum Option : int
{
    OPT_MYSTERIOUS_DWELLING = 15,
    OPT_JUNK_MERCHANT = 108,
    OPT_ARCANE_TOWER = 104,
    OPT_MAGIC_MUSHROOMS = 110,
    OPT_DEATH_CHAMBER = 70,
    OPT_ALTAR = 14,
    OPT_FORGOTTEN_SHRINE = 60,
    OPT_ALMS_HOUSE = 18,
    OPT_BATTLE_ACADEMY = 16,
    OPT_PALACE_OF_DREAMS = 11,
    OPT_LIVING_SKULL = 12,
    OPT_EMERALD_TOWER = 44,
    OPT_MIRROR = 52
};

// Standard WoG 3.58+ creature IDs used by the original scripts.
// Always validate them against H3CreatureCount before dereferencing.
namespace creature {
constexpr int SUPREME_ARCHANGEL = 150;
constexpr int HELL_BARON = 153;
constexpr int BLOOD_DRAGON = 154;
constexpr int DARKNESS_DRAGON = 155;
constexpr int GHOST = 159;
constexpr int FIRST_EMISSARY = 160;
constexpr int LAST_EMISSARY = 163;
constexpr int SORCERESS = 193;
constexpr int GORYNYCH = 168;
constexpr int ARCTIC_SHARPSHOOTER = 170;
constexpr int LAVA_SHARPSHOOTER = 171;
}

struct Descriptor
{
    int subtype;
    int optionId;
    int defaultAiWeight;
    bool cloneLegacyProperty;
    bool rmgEnabled;
};

const Descriptor *FindDescriptor(int subtype) noexcept;
const Descriptor *DescriptorsBegin() noexcept;
const Descriptor *DescriptorsEnd() noexcept;
bool IsPortedSubtype(int subtype) noexcept;

// Intentionally not present: Commander Sanctuary is type 80 and remains owned by its original implementation.
static_assert(extender::ERA_OBJECT_TYPE != 80, "ERA WoG object identity must never alias Commander Sanctuary");

}} // namespace wog::objects
