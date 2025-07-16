#include "pch.h"

#ifdef CREATE_JSON

#include <fstream>
#include <string>
#include <thread>
// Структура для хранения данных о монстре

std::string ExportManager::LPCSTR_to_wstring(LPCSTR ansi_str)
{
    // 1. Получаем длину без нуль-терминатора
    const int src_len = strlen(ansi_str);

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

void ExportManager::WriteJsonFile(const std::string &filePath, nlohmann::json &j)
{

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
}
void ExportManager::WriteJsonFile(const std::string &filePath, nlohmann::ordered_json &j)
{

    // Сохраняем JSON в файл
    if (!j.empty())
    {
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
    }
}

// Функция для создания JSON-документа
void ExportManager::CreateArtifactsJson(const std::vector<ArtifactInfo> &artifacts)
{
    const std::string filePath = "artifactNames.json";

    // std::thread th([&]() {
    nlohmann::ordered_json j;

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
    WriteJsonFile(filePath, j);

    //  });
    //  CreateArtifactsJson(artifacts, "artifactNames.json");
    //  th.detach();
}

// Функция для создания JSON-документа
void ExportManager::CreateMonstersJson(const std::vector<MonsterInfo> &monsters)
{
    const std::string filePath = "monsterNames.json";
    // std::thread th([&]() {
    nlohmann::ordered_json j;

    // Создаем структуру JSON
    for (size_t i = 0; i < monsters.size(); ++i)
    {
        if (!monsters[i].singularName.empty())
            j["era"]["monsters"][std::to_string(i)]["name"]["singular"] = monsters[i].singularName;

        if (!monsters[i].pluralName.empty())
            j["era"]["monsters"][std::to_string(i)]["name"]["plural"] = monsters[i].pluralName;

        if (!monsters[i].description.empty())
            j["era"]["monsters"][std::to_string(i)]["name"]["description"] = monsters[i].description;
    }

    WriteJsonFile(filePath, j);

    // });
    // th.detach();
}

static int GetCreatureBankId(const int objType, const int objSubtype) noexcept
{
    int cbId = -1;
    switch (objType)
    {
    case eObject::CREATURE_BANK:
        cbId = objSubtype;
        break;
    case eObject::DERELICT_SHIP:
        cbId = eCrBank::DERELICT_SHIP;
        break;
    case eObject::DRAGON_UTOPIA:
        cbId = eCrBank::DRAGON_UTOPIA;
        break;
    case eObject::CRYPT:
        cbId = eCrBank::CRYPT;
        break;
    case eObject::SHIPWRECK:
        cbId = eCrBank::SHIPWRECK;
        break;
    default:
        break;
    }

    return cbId;
}

void ExportManager::CreateTownBuildingsJson(const std::vector<std::string> &standardBuildingNames,
                                            const std::vector<DwellingInfo> &dwellingInfos,
                                            const std::vector<DwellingInfo> &dwelling4Infos)
{

    const std::string filePath = "townBuildings.json";
    const UINT dwellinsPerTown = ByteAt(0x05B995F + 2);
    const UINT townsNum = DwordAt(0x05B9962 + 2) / dwellinsPerTown;

    const UINT neutralTownId = townsNum - 1;

    nlohmann::ordered_json j;

    for (size_t townType = 0; townType < townsNum; townType++)
    {
        const int jsonTownType =
            townType == neutralTownId ? -1 : townType; // Neutral town is 0 in JSON, others are 1-indexed
        for (size_t dwellingId = 0; dwellingId < dwellinsPerTown; dwellingId++)
        {
            const UINT stringId = townType * dwellinsPerTown + dwellingId;
            if (!dwellingInfos[stringId].name.empty())
            {
                j["era"]["towns"][std::to_string(jsonTownType)]["dwellings"][std::to_string(dwellingId)]["name"] =
                    dwellingInfos[stringId].name;
            }
            if (!dwellingInfos[stringId].description.empty())
            {
                j["era"]["towns"][std::to_string(jsonTownType)]["dwellings"][std::to_string(dwellingId)]
                 ["description"] = dwellingInfos[stringId].description;
            }
        }
    }

    WriteJsonFile(filePath, j);
}

void ExportManager::CreateCreatureBanksJson()
{
    const std::string filePath = "creatureBankNames.json";

    nlohmann::ordered_json j;

    // Создаем структуру JSON
    const auto &banks = P_CreatureBankSetup->Get();

    const auto bankObjectTypes = {eObject::CREATURE_BANK, eObject::DERELICT_SHIP, eObject::DRAGON_UTOPIA,
                                  eObject::CRYPT, eObject::SHIPWRECK};
    for (const int objectType : bankObjectTypes)
    {
        const int size = P_Game->mainSetup.objectLists[objectType].Size();

        bool readSuccess = false;
        std::vector<std::pair<int, int>> bankTypes; // (size);
        bankTypes.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            const auto &objectPrototype = P_Game->mainSetup.objectLists[objectType][i];

            const int cbId = GetCreatureBankId(objectPrototype.type, objectPrototype.subtype);
            if (!banks[cbId].name.Empty())
                bankTypes.push_back({cbId, objectPrototype.subtype});
        }

        std::sort(bankTypes.begin(), bankTypes.end(),
                  [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.first < b.first; });

        for (size_t i = 0; i < bankTypes.size(); i++)
        {
            const auto &objInfo = bankTypes[i];

            const auto &bank = banks[objInfo.first];

            j["RMG"]["objectGeneration"][std::to_string(objectType)][std::to_string(objInfo.second)]["name"] =
                LPCSTR_to_wstring(bank.name.String());

            LPCSTR visitText = EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visit", objectType, objInfo.second).String(),
                readSuccess);
            if (readSuccess)
            {
                j["RMG"]["objectGeneration"][std::to_string(objectType)][std::to_string(objInfo.second)]["text"]
                 ["visit"] = visitText;
            }
        }
    }

    WriteJsonFile(filePath, j);
}
void ExportManager::CreateObjectsJson(const std::vector<std::string> &objectNames,
                                      const std::vector<std::string> &dwelling1Names,
                                      const std::vector<std::string> &dwelling4Names)
{
    const std::string filePath = "objectNames.json";
    // std::thread th([&]() {
    nlohmann::ordered_json j;
    // Создаем структуру JSON
    for (size_t i = 0; i < objectNames.size(); ++i)
    {
        if (!objectNames[i].empty())
            j["era"]["objects"][std::to_string(i)] = objectNames[i];
    }
    for (size_t i = 0; i < dwelling1Names.size(); ++i)
    {
        if (!dwelling1Names[i].empty())
            j["era"]["dwellings1"][std::to_string(i)] = dwelling1Names[i];
    }
    for (size_t i = 0; i < dwelling4Names.size(); ++i)
    {
        if (!dwelling4Names[i].empty())
            j["era"]["dwellings4"][std::to_string(i)] = dwelling4Names[i];
    }

    // Сохраняем JSON в файл
    WriteJsonFile(filePath, j);

    //});
    //  th.detach();
}
#endif // CREATE_JSON
