#include "MapObjectHandler.h"
#include "pch.h"

#ifdef CREATE_JSON

#include <fstream>
#include <thread>

// Функция для создания JSON-документа
void CreateObjectsJson(const std::vector<std::string> &objectNames, const std::vector<std::string> &dwellingNames,
                       const std::string &filePath)
{
    nlohmann::json j;
    // Создаем структуру JSON
    for (size_t i = 0; i < objectNames.size(); ++i)
    {
        if (!objectNames[i].empty())
            j["era"]["objects"][std::to_string(i)] = objectNames[i];
    }
    for (size_t i = 0; i < dwellingNames.size(); ++i)
    {
        if (!dwellingNames[i].empty())
            j["era"]["dwellings1"][std::to_string(i)] = dwellingNames[i];
    }

    // Сохраняем JSON в файл
    std::ofstream outFile(filePath);
    if (outFile.is_open())
    {
        outFile << j.dump(4); // 4 — это отступ для красивого форматирования
        outFile.close();
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
}
#endif // CREATE_JSON
void MapObjectHandler::Init()
{

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = H3DwellingNames1::Get();
    const int dwellingsNum = IntAt(0x49DD8E + 2) / 4;

#ifdef CREATE_JSON
    std::vector<std::string> objects, dwellings;
    objects.resize(limits::OBJECTS);
    dwellings.resize(dwellingsNum);
#endif // CREATE_JSON
    // load dwelling names
    for (size_t i = 0; i < dwellingsNum; i++)
    {
#ifdef CREATE_JSON
        dwellings[i] = table[i];
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.dwellings1.%d", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }

    //  H3TextFile *txt = reinterpret_cast<H3TextFile* >(0x069168C);
    //  if (txt)
    //  {
    //     // const size_t txtSize = int( txt->begin() )- int( txt->end());
    //    //  for (size_t i = 0; i < txtSize; i++)
    //{
    //         // MessageBoxA(nullptr, txt->GetText(0), "", MB_OK);
    //}
    //      //MessageBoxA(nullptr, *txt->begin(), "", MB_OK);
    //  }
    // load native object names
    readSuccess = false;
    readResult = nullptr;
    table = H3ObjectName::Get();
    for (size_t i = 0; i < limits::OBJECTS; i++)
    {
#ifdef CREATE_JSON
        objects[i] = table[i];
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.objects.%d", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }
#ifdef CREATE_JSON
    std::thread th(CreateObjectsJson, objects, dwellings, "objectNames.json");
    //    CreateObjectsJson(objects,dwellings,"objectNames.json");

    th.detach();
#endif // CREATE_JSON
}
