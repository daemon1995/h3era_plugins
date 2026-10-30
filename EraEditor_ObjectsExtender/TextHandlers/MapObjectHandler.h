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
static_assert(sizeof(MapObjectData) == 16, "Editor object type records must remain 16 bytes");
class MapObjectHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR OBJECTS = "era.objects.%d";
    };

    static void Init();
};
