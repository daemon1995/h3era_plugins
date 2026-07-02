#pragma once
#include "../framework.h"

struct MapObjectData
{
    const char *objectName;
    int objectIndex;
    char _0_f[4];
    H3Bitfield bits;

    static MapObjectData *Get()
    {
        return *reinterpret_cast<MapObjectData **>(0x0401EB8 + 1);
    }
};
class MapObjectHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR DWELLING1 = "era.dwellings1.%d";
        static constexpr LPCSTR DWELLING4 = "era.dwellings4.%d";
        static constexpr LPCSTR OBJECTS = "era.objects.%d";
        static constexpr LPCSTR CREATURE_BANKS = "RMG.objectGeneration.%d.%d.name";
        static constexpr LPCSTR CUSTOM_OBJECT = CREATURE_BANKS;
    };

    static void Init();
    static int GetSubtypesAmount(const eObject type) noexcept;
};
