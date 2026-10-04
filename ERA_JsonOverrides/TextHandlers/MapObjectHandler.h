#pragma once
#include "HandlersList.h"

class MapObjectHandler
{
  public:
    static void Init()
    {
        auto *table = H3DwellingNames1::Get();
        for (int i = 0; table && i < EraJS::GameTables::Dwelling1Capacity(); ++i)
            EraJS::ReadField(table[i], "era.dwellings1.%d", i);
        table = H3DwellingNames4::Get();
        for (int i = 0; table && i < EraJS::GameTables::Dwelling4Capacity(); ++i)
            EraJS::ReadField(table[i], "era.dwellings4.%d", i);
        table = H3ObjectName::Get();
        for (int i = 0; table && i < limits::OBJECTS; ++i)
            EraJS::ReadField(table[i], "era.objects.%d", i);
    }
};
