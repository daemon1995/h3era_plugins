#include "HandlersList.h"
#include <unordered_set>
void MapObjectHandler::Init()
{

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = H3DwellingNames1::Get();

    std::unordered_set<int> dwelling1Types, dwelling4Types;

    for (auto &i : P_Game->mainSetup.objectLists[eObject::CREATURE_GENERATOR1])
    {
        dwelling1Types.insert(i.subtype);
    }
    for (auto &i : P_Game->mainSetup.objectLists[eObject::CREATURE_GENERATOR4])
    {
        dwelling4Types.insert(i.subtype);
    }

    const int dwellings1Num = dwelling1Types.size(); // IntAt(0x0405CCE + 2) / 4;
    const int dwellings4Num = dwelling4Types.size(); // Updated to use dwelling4Types.size()

#ifdef CREATE_JSON
    std::vector<std::string> objects, dwellings1, dwellings4;
    objects.resize(limits::OBJECTS);
    dwellings1.resize(dwellings1Num);
    dwellings4.resize(dwellings4Num);
#endif // CREATE_JSON
    // load dwelling names
    for (size_t i = 0; i < dwellings1Num; i++)
    {
#ifdef CREATE_JSON
        dwellings1[i] = ExportManager::LPCSTR_to_wstring(table[i]);
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.dwellings1.%d", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }
    table = H3DwellingNames4::Get();
    // load dwelling 4 names
    for (size_t i = 0; i < dwellings4Num; i++)
    {
#ifdef CREATE_JSON
        dwellings4[i] = ExportManager::LPCSTR_to_wstring(H3DwellingNames4::Get()[i]);
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.dwellings4.%d", i);
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
#ifdef CREATE_JSON
        objects[i] = ExportManager::LPCSTR_to_wstring(table[i]);
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.objects.%d", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult; // + 1 because first object is empty
    }
#ifdef CREATE_JSON
    ExportManager::CreateObjectsJson(objects, dwellings1, dwellings4);
    ExportManager::CreateCreatureBanksJson();
#endif // CREATE_JSON
}
