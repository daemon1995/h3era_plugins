#include "ExportManager.h"
#ifdef CREATE_TEXT_JSON_EXPORTS

#include <algorithm>
#include <limits>

namespace
{
std::string ToWindowsLineEndings(const std::string &text)
{
    std::string result;
    result.reserve(text.size());

    for (const char character : text)
    {
        if (character == '\n' && (result.empty() || result.back() != '\r'))
            result.push_back('\r');
        result.push_back(character);
    }
    return result;
}

BOOL WriteTextFile(const std::string &filePath, const std::string &text)
{
    const std::string fileContent = ToWindowsLineEndings(text);
    const HANDLE fileHandle = CreateFileA(filePath.c_str(), GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
                                          FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return FALSE;

    size_t bytesToWrite = fileContent.size();
    size_t bytesWrittenTotal = 0;
    while (bytesToWrite != 0)
    {
        const DWORD writeSize = static_cast<DWORD>(
            std::min<size_t>(bytesToWrite, (std::numeric_limits<DWORD>::max)()));
        DWORD bytesWritten = 0;
        if (!WriteFile(fileHandle, &fileContent[bytesWrittenTotal], writeSize, &bytesWritten, nullptr) || bytesWritten == 0)
        {
            CloseHandle(fileHandle);
            return FALSE;
        }
        bytesWrittenTotal += bytesWritten;
        bytesToWrite -= bytesWritten;
    }

    return CloseHandle(fileHandle) != FALSE;
}
} // namespace

// Структура для хранения данных о монстре
std::string ExportManager::LPCSTR_to_wstring(LPCSTR ansi_str)
{
    // 1. Получаем длину без нуль-терминатора
    if (!ansi_str || !*ansi_str) return {};
    const int src_len = static_cast<int>(std::strlen(ansi_str));

    // 2. ANSI → UTF-16
    const DWORD acp = Era::GetCodePage();
    const int wlen = ::MultiByteToWideChar(acp, 0, ansi_str, src_len, NULL, 0);
    if (wlen <= 0) throw std::runtime_error("Could not decode exported text");
    std::wstring wstr(wlen, 0);
    if (::MultiByteToWideChar(acp, 0, ansi_str, src_len, &wstr[0], wlen) != wlen)
        throw std::runtime_error("Could not decode exported text");

    // 3. UTF-16 → UTF-8
    const int ulen = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    if (ulen <= 0) throw std::runtime_error("Could not encode exported text");
    std::string utf8_str(ulen, 0);
    if (::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8_str[0], ulen, NULL, NULL) != ulen)
        throw std::runtime_error("Could not encode exported text");

    return utf8_str;
}

BOOL ExportManager::WriteJsonFile(const std::string &filePath, nlohmann::json &j)
{
    // Сохраняем JSON в файл
    if (WriteTextFile(filePath, j.dump(4)))
        return true;

    throw std::runtime_error("Failed to open file for writing: " + filePath);
}

void EnsureDirectoryExists(const std::string &filePath)
{
    size_t pos = 0;
    while ((pos = filePath.find_first_of("/\\", pos + 1)) != std::string::npos)
    {
        std::string dir = filePath.substr(0, pos);
        if (!dir.empty())
        {
            CreateDirectoryA(dir.c_str(), NULL);
        }
    }
}

BOOL ExportManager::WriteJsonFile(const std::string &filePath, nlohmann::ordered_json &j)
{

    // Сохраняем JSON в файл
    if (!j.empty())
    {
        EnsureDirectoryExists(filePath);
        if (WriteTextFile(filePath, j.dump(4)))
            return true;

        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    return false; // Возвращаем false, если JSON пустой
}

INT ExportManager::GetMaxOriginalId(LPCSTR keySubstring, const int defaultValue)
{
    // Получаем максимальный ID оригинальных данных из конфигурации
    bool readSuccess = false;
    const int maxId = EraJS::readInt(MAX_ID_JSON + std::string(keySubstring), readSuccess);
    if (!readSuccess || maxId < 0 || maxId == (std::numeric_limits<int>::max)())
    {
        return defaultValue;
    }
    return maxId;
}
BOOL ExportManager::CreateMonstersJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)
{
    nlohmann::ordered_json j;

    constexpr int maxOriginalId = 196;
    const int outOfBound = GetMaxOriginalId("creatures", maxOriginalId) + 1;

    const int minId = originalData ? 0 : outOfBound;
    const int maxMonId = IntAt(0x4A1657);
    const int maxId = Clamp(0, additionalData ? maxMonId : outOfBound, maxMonId);

    // Создаем структуру JSON
    for (size_t i = minId; i < maxId; ++i)
    {
        EraJS::VisitCreatureText(P_CreatureInformation[i], [&](LPCSTR field, LPCSTR value) {
            if (!value || !*value) return;
            auto &creatureJson = j["era"]["monsters"][std::to_string(i)];
            if (std::strcmp(field, "description") == 0) creatureJson[field] = LPCSTR_to_wstring(value);
            else creatureJson["name"][field] = LPCSTR_to_wstring(value);
        });
    }

    return WriteJsonFile(filePath, j);
}

BOOL ExportManager::CreateArtifactsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)
{
    nlohmann::ordered_json j;

    constexpr int maxOriginalId = 170;
    const int outOfBound = GetMaxOriginalId("artifacts", maxOriginalId) + 1;
    const int minId = originalData ? 0 : outOfBound;
    const int maxArtId = ArtifactInfo::GetArtifactsNumber();
    const int maxId = Clamp(0, additionalData ? maxArtId : outOfBound, maxArtId);

    const auto &event = ArtifactInfo::GetEventTable();
    // Создаем структуру JSON
    for (size_t i = minId; i < maxId; ++i)
    {
        const auto &artInfo = P_ArtifactSetup->Get()[i];
        auto &artifactJson = j["era"]["artifacts"][std::to_string(i)];
        EraJS::VisitArtifact(artInfo, [&](LPCSTR field, const auto &value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, LPCSTR>)
            {
                if (value) artifactJson[field] = LPCSTR_to_wstring(value);
            }
            else artifactJson[field] = static_cast<int>(value);
        });

        if (event && i < EraJS::GameTables::ArtifactEventCapacity() && event[i] && libc::strlen(event[i])) // Added argument to strlen()
            j["era"]["artifacts"][std::to_string(i)]["event"] = LPCSTR_to_wstring(event[i]);
    }

    // Сохраняем JSON в файл
    return WriteJsonFile(filePath, j);
}
BOOL ExportManager::CreateObjectsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)

{

    nlohmann::ordered_json j;
    // Создаем структуру JSON

    const int objectsNum = limits::OBJECTS;
    LPCSTR *table = H3ObjectName::Get();
    for (size_t i = 0; i < objectsNum; ++i)
    {
        if (table[i] && libc::strcmpi(table[i], h3_NullString))
            j["era"]["objects"][std::to_string(i)] = LPCSTR_to_wstring(table[i]);
    }
    table = H3DwellingNames1::Get();

    constexpr int maxOriginalId1 = 100;
    const int outOfBound1 = GetMaxOriginalId("dwellings1", maxOriginalId1) + 1;
    const int minId1 = originalData ? 0 : outOfBound1;
    const int maxAmount1 = EraJS::GameTables::Dwelling1Capacity();
    const int maxId1 = Clamp(0, additionalData ? maxAmount1 : outOfBound1, maxAmount1);

    for (size_t i = minId1; i < maxId1; ++i)
    {
        if (table[i] && libc::strcmpi(table[i], h3_NullString))
            j["era"]["dwellings1"][std::to_string(i)] = LPCSTR_to_wstring(table[i]);
    }

    const int dwellings4Num = EraJS::GameTables::Dwelling4Capacity();

    constexpr int maxOriginalId4 = 1;
    const int outOfBound4 = GetMaxOriginalId("dwellings4", maxOriginalId4) + 1;
    const int minId4 = originalData ? 0 : outOfBound4;
    const int maxAmount4 = EraJS::GameTables::Dwelling4Capacity();
    const int maxId4 = Clamp(0, additionalData ? maxAmount4 : outOfBound4, maxAmount4);

    table = H3DwellingNames4::Get();

    for (size_t i = minId4; i < maxId4; ++i)
    {
        if (table[i] && libc::strcmpi(table[i], h3_NullString))
            j["era"]["dwellings4"][std::to_string(i)] = LPCSTR_to_wstring(table[i]);
    }

    // Сохраняем JSON в файл
    return WriteJsonFile(filePath, j);
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
BOOL ExportManager::CreateCreatureBanksJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)

{
    nlohmann::ordered_json j;

    // Создаем структуру JSON
    const auto &banks = P_CreatureBankSetup->Get();

    const auto bankObjectTypes = {eObject::CREATURE_BANK, eObject::DERELICT_SHIP, eObject::DRAGON_UTOPIA,
                                  eObject::CRYPT, eObject::SHIPWRECK};

    int creatureBanksNumber = EraJS::GameTables::CreatureBankCapacity();
    for (auto &i : P_Game->mainSetup.objectLists[eObject::CREATURE_BANK])
    {
        if (i.subtype >= creatureBanksNumber)
        {
            creatureBanksNumber = i.subtype + 1;
        }
    }

    constexpr int maxOriginalId = 20;
    const int outOfBound = GetMaxOriginalId("creatureBanks", maxOriginalId) + 1;
    const int minId = originalData ? 0 : outOfBound;
    const int maxId = Clamp(0, additionalData ? creatureBanksNumber : outOfBound, creatureBanksNumber);

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
            if (cbId < minId || cbId >= maxId) continue;
            // Do not infer allocation size from sparse subtype IDs.
            bankTypes.push_back({cbId, objectPrototype.subtype});
        }

        std::sort(bankTypes.begin(), bankTypes.end(),
                  [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.first < b.first; });

        for (size_t i = 0; i < bankTypes.size(); i++)
        {
            const auto &objInfo = bankTypes[i];

            LPCSTR name = nullptr;
            if (objInfo.first < EraJS::GameTables::CreatureBankCapacity())
                name = banks[objInfo.first].name.String();
            else
            {
                // Some extenders relocate setups without publishing capacity.
                // Export their existing JSON instead of probing past the known array.
                name = EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.name", objectType, objInfo.second).String(), readSuccess);
                if (!readSuccess) name = nullptr;
            }
            if (name && *name)
                j["RMG"]["objectGeneration"][std::to_string(objectType)][std::to_string(objInfo.second)]["name"] = LPCSTR_to_wstring(name);

            LPCSTR visitText = EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visit", objectType, objInfo.second).String(),
                readSuccess);
            if (readSuccess)
            {
                j["RMG"]["objectGeneration"][std::to_string(objectType)][std::to_string(objInfo.second)]["text"]
                 ["visit"] = LPCSTR_to_wstring(visitText);
            }
        }
    }

    return WriteJsonFile(filePath, j);
}
BOOL ExportManager::CreateTownBuildingsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)

{
    const UINT dwellinsPerTown = ByteAt(0x05B995F + 2);
    if (!dwellinsPerTown) return FALSE;
    const UINT townsNum = DwordAt(0x05B9962 + 2) / dwellinsPerTown;
    if (!townsNum) return FALSE;

    const UINT neutralTownId = townsNum - 1;
    const auto townDwellingNames = TownBuildingInfo::GetTownDwellingNames();
    const auto townDwellingDescriptions = TownBuildingInfo::GetTownDwellingDescriptions();
    nlohmann::ordered_json j;

    constexpr int maxOriginalId = 10;
    const int outOfBound = GetMaxOriginalId("towns", maxOriginalId) + 1;
    const int minId = originalData ? 0 : outOfBound;
    const int maxId = Clamp(0, additionalData ? townsNum : outOfBound, townsNum);

    for (size_t townType = minId; townType < maxId; townType++)
    {

        const bool isNeutralTown = townType == neutralTownId;
        if (isNeutralTown && originalData == false)
        {
            break; // Neutral town is not exported w/o original data
        }
        const int jsonTownType = isNeutralTown ? -1 : townType; // Neutral town is 0 in JSON, others are 1-indexed

        for (size_t dwellingId = 0; dwellingId < dwellinsPerTown; dwellingId++)
        {
            const UINT stringId = townType * dwellinsPerTown + dwellingId;
            // if (!dwellingInfos[stringId].name.empty())
            {
                j["era"]["towns"][std::to_string(jsonTownType)]["buildings"][std::to_string(30 + dwellingId)]["name"] =
                    LPCSTR_to_wstring(townDwellingNames[stringId]);
            }
            // if (!dwellingInfos[stringId].description.empty())
            {
                j["era"]["towns"][std::to_string(jsonTownType)]["buildings"][std::to_string(30 + dwellingId)]
                 ["description"] = LPCSTR_to_wstring(townDwellingDescriptions[stringId]);
            }
        }
    }

    return WriteJsonFile(filePath, j);
}
BOOL ExportManager::CreateHeroesJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)

{
    nlohmann::ordered_json j;

    constexpr int maxOriginalId = 155;
    const int outOfBound = GetMaxOriginalId("heroes", maxOriginalId) + 1;

    const int heroCount = H3HeroCount::Get();
    const int minId = originalData ? 0 : outOfBound;
    const int maxId = Clamp(0, additionalData ? heroCount : outOfBound, heroCount);

    LPCSTR str = nullptr;
    for (size_t heroType = minId; heroType < maxId; heroType++)
    {

        str = P_HeroInfo[heroType].name;
        if (str && (libc::strcmpi(str, h3_NullString)))
            j["era"]["heroes"][std::to_string(heroType)]["name"] = LPCSTR_to_wstring(str);

        // Export hero data
        str = P_HeroSpecialty[heroType].spShort;
        if (str && (libc::strcmpi(str, h3_NullString)))
            j["era"]["heroes"][std::to_string(heroType)]["specialty"]["short"] = LPCSTR_to_wstring(str);

        str = P_HeroSpecialty[heroType].spFull;
        if (str && (libc::strcmpi(str, h3_NullString)))
            j["era"]["heroes"][std::to_string(heroType)]["specialty"]["full"] = LPCSTR_to_wstring(str);

        str = P_HeroSpecialty[heroType].spDescr;
        if (str && (libc::strcmpi(str, h3_NullString)))
            j["era"]["heroes"][std::to_string(heroType)]["specialty"]["description"] = LPCSTR_to_wstring(str);

        str = EraJS::GameTables::HeroBiographies()[heroType];
        if (str && (libc::strcmpi(str, h3_NullString)))
        {
            j["era"]["heroes"][std::to_string(heroType)]["biography"] = LPCSTR_to_wstring(str);
        }
    }

    return WriteJsonFile(filePath, j);
}

// void ExportManager::ExportAllToJson(const BOOL originalDatas, const BOOL additionalData)
//{
//     ExportManager::CreateMonstersJson(originalDatas);
//     ExportManager::CreateArtifactsJson(originalDatas);
//     ExportManager::CreateObjectsJson(originalDatas);
//     ExportManager::CreateCreatureBanksJson(originalDatas);
//     ExportManager::CreateTownBuildingsJson(originalDatas, true);
// }

#endif // CREATE_TEXT_JSON_EXPORTS
