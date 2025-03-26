#include "ExportManager.h"
#include "pch.h"

#ifdef CREATE_JSON

#include <fstream>
#include <thread>
// Структура для хранения данных о монстре

std::string ExportManager::LPCSTR_to_wstring(LPCSTR ansi_str)
{
    // 1. Получаем длину без нуль-терминатора
    int src_len = strlen(ansi_str);

    // 2. ANSI → UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi_str, src_len, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi_str, src_len, &wstr[0], wlen);

    // 3. UTF-16 → UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    std::string utf8_str(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8_str[0], ulen, NULL, NULL);

    return utf8_str;
}

// Функция для создания JSON-документа
void ExportManager::CreateMonstersJson(const std::vector<MonsterInfo> &monsters)
{
    const std::string filePath = "monsterNames.json";
    // std::thread th([&]() {
    nlohmann::json j;

    // Создаем структуру JSON
    for (size_t i = 0; i < monsters.size(); ++i)
    {
        if (!monsters[i].singularName.empty())
            j["era"]["monsters"][std::to_string(i)]["name"]["singular"] = monsters[i].singularName;

        if (!monsters[i].pluralName.empty())
            j["era"]["monsters"][std::to_string(i)]["name"]["pluralName"] = monsters[i].pluralName;

        if (!monsters[i].description.empty())
            j["era"]["monsters"][std::to_string(i)]["name"]["description"] = monsters[i].description;
    }

    // Сохраняем JSON в файл
    std::ofstream outFile(filePath);
    if (outFile.is_open())
    {
        outFile << j.dump(4);
        outFile.close();
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    // });
    // th.detach();
}

// Функция для создания JSON-документа
void ExportManager::CreateArtifactsJson(const std::vector<ArtifactInfo> &artifacts)
{
    const std::string filePath = "artifactNames.json";

    // std::thread th([&]() {
    nlohmann::json j;

    // Создаем структуру JSON
    for (size_t i = 0; i < artifacts.size(); ++i)
    {
        if (!artifacts[i].name.empty())
            j["era"]["artifacts"][std::to_string(i)]["name"] = artifacts[i].name;
        if (!artifacts[i].description.empty())
            j["era"]["artifacts"][std::to_string(i)]["description"] = artifacts[i].description;

        if (!artifacts[i].event.empty())
            j["era"]["artifacts"][std::to_string(i)]["event"] = artifacts[i].event;
    }

    // Сохраняем JSON в файл
    std::ofstream outFile(filePath);
    if (outFile.is_open())
    {

        outFile << j.dump(4); // , 32, false, nlohmann::json::error_handler_t::replace);
        outFile.close();
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    //  });
    //  CreateArtifactsJson(artifacts, "artifactNames.json");
    //  th.detach();
}

void ExportManager::CreateObjectsJson(const std::vector<std::string> &objectNames,
                                      const std::vector<std::string> &dwellingNames)
{
    const std::string filePath = "objectNames.json";
    // std::thread th([&]() {
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
    //});
    //  th.detach();
}
#endif // CREATE_JSON
