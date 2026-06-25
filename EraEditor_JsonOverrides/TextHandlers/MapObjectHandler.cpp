#include "HandlersList.h"
#include <unordered_set>
void MapObjectHandler::Init()
{

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = H3DwellingNames1::Get();

    const int dwellings1Num = GetSubtypesAmount(eObject::CREATURE_GENERATOR1);
    const int dwellings4Num = GetSubtypesAmount(eObject::CREATURE_GENERATOR4);

    // load dwelling names
    for (size_t i = 0; i < dwellings1Num; i++)
    {

        sprintf(h3_TextBuffer, formats::DWELLING1, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }
    table = H3DwellingNames4::Get();
    // load dwelling 4 names
    for (size_t i = 0; i < dwellings4Num; i++)
    {
        sprintf(h3_TextBuffer, formats::DWELLING4, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }

    // load native object names
    readSuccess = false;
    readResult = nullptr;
    table = H3ObjectName::Get();
    for (size_t i = 0; i < limits::OBJECTS; i++)
    {
        sprintf(h3_TextBuffer, formats::OBJECTS, i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult; // + 1 because first object is empty
    }
}

int MapObjectHandler::GetSubtypesAmount(const eObject type) noexcept
{
    std::unordered_set<int> objectSubtypes;
    for (auto &i : P_Game->mainSetup.objectLists[type])
    {
        objectSubtypes.insert(i.subtype);
    }
    return objectSubtypes.size();
}
