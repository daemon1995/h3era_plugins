#include "pch.h"

#ifdef CREATE_JSON

#include <fstream>
#include <string>

// Структура для хранения данных о монстре
constexpr int ITEMS_PER_PANEL = 3;
std::string ExportManager::LPCSTR_to_wstring(LPCSTR ansi_str)
{
    // 1. Получаем длину без нуль-терминатора
    const int src_len = strlen(ansi_str);

    // 2. ANSI → UTF-16
    const int wlen = MultiByteToWideChar(CP_ACP, 0, ansi_str, src_len, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi_str, src_len, &wstr[0], wlen);

    // 3. UTF-16 → UTF-8
    const int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    std::string utf8_str(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8_str[0], ulen, NULL, NULL);

    return utf8_str;
}

BOOL ExportManager::WriteJsonFile(const std::string &filePath, nlohmann::json &j)
{
    // Сохраняем JSON в файл
    std::ofstream outFile(filePath);
    if (outFile.is_open())
    {
        outFile << j.dump(4);
        outFile.close();
        return true;
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    return false; // Возвращаем false, если JSON пустой
}
BOOL ExportManager::WriteJsonFile(const std::string &filePath, nlohmann::ordered_json &j)
{

    // Сохраняем JSON в файл
    if (!j.empty())
    {
        std::ofstream outFile(filePath);
        if (outFile.is_open())
        {
            outFile << j.dump(4);
            outFile.close();
            return true;
        }
        else
        {
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        }
    }
    return false; // Возвращаем false, если JSON пустой
}

INT ExportManager::GetMaxOriginalId(LPCSTR keySubstring, const int defaultValue)
{
    // Получаем максимальный ID оригинальных данных из конфигурации
    bool readSuccess = false;
    const int maxId = EraJS::readInt(MAX_ID_JSON + std::string(keySubstring), readSuccess);
    if (!readSuccess || maxId < 0)
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
        auto &creatureInfo = P_CreatureInformation[i];
        auto &ptr = creatureInfo.nameSingular;
        if (ptr && stricmp(ptr, h3_NullString))
            j["era"]["monsters"][std::to_string(i)]["name"]["singular"] = LPCSTR_to_wstring(ptr);

        ptr = creatureInfo.namePlural;
        if (ptr && stricmp(ptr, h3_NullString))
            j["era"]["monsters"][std::to_string(i)]["name"]["plural"] = LPCSTR_to_wstring(ptr);

        // Если описание не пустое, добавляем его в JSON
        ptr = creatureInfo.description;
        if (ptr && stricmp(ptr, h3_NullString))
            j["era"]["monsters"][std::to_string(i)]["name"]["description"] = LPCSTR_to_wstring(ptr);
    }

    return WriteJsonFile(filePath, j);
}

BOOL ExportManager::CreateArtifactsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)

{
    nlohmann::ordered_json j;
    constexpr int maxOriginalId = 170;
    const int outOfBound = GetMaxOriginalId("artifacts", maxOriginalId) + 1;
    const int minId = originalData ? 0 : outOfBound;
    const int maxArtId = IntAt(0x49DD8E + 2) >> 2;
    const int maxId = Clamp(0, additionalData ? maxArtId : outOfBound, maxArtId);

    const auto &event = ArtifactHandler::GetEventTable();
    // Создаем структуру JSON
    for (size_t i = minId; i < maxId; ++i)
    {
        auto &artInfo = P_ArtifactSetup->Get()[i];
        if (artInfo.name)
        {
            j["era"]["artifacts"][std::to_string(i)]["name"] = LPCSTR_to_wstring(artInfo.name);
        }

        if (artInfo.description)
        {
            j["era"]["artifacts"][std::to_string(i)]["description"] = LPCSTR_to_wstring(artInfo.description);
        }

        if (event[i] && libc::strlen(event[i])) // Added argument to strlen()
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
        if (stricmp(table[i], h3_NullString))
            j["era"]["objects"][std::to_string(i)] = LPCSTR_to_wstring(table[i]);
    }
    table = H3DwellingNames1::Get();

    constexpr int maxOriginalId1 = 100;
    const int outOfBound1 = GetMaxOriginalId("dwellings1", maxOriginalId1) + 1;
    const int minId1 = originalData ? 0 : outOfBound1;
    const int maxAmount1 = MapObjectHandler::GetSubtypesAmount(eObject::CREATURE_GENERATOR1);
    const int maxId1 = Clamp(0, additionalData ? maxAmount1 : outOfBound1, maxAmount1);

    for (size_t i = minId1; i < maxId1; ++i)
    {
        if (stricmp(table[i], h3_NullString))
            j["era"]["dwellings1"][std::to_string(i)] = LPCSTR_to_wstring(table[i]);
    }

    const int dwellings4Num = MapObjectHandler::GetSubtypesAmount(eObject::CREATURE_GENERATOR4);

    constexpr int maxOriginalId4 = 1;
    const int outOfBound4 = GetMaxOriginalId("dwellings4", maxOriginalId4) + 1;
    const int minId4 = originalData ? 0 : outOfBound4;
    const int maxAmount4 = MapObjectHandler::GetSubtypesAmount(eObject::CREATURE_GENERATOR4);
    const int maxId4 = Clamp(0, additionalData ? maxAmount4 : outOfBound4, maxAmount4);

    table = H3DwellingNames4::Get();

    for (size_t i = minId4; i < maxId4; ++i)
    {
        if (stricmp(table[i], h3_NullString))
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

    int creatureBanksNumber = 0;
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
                 ["visit"] = LPCSTR_to_wstring(visitText);
            }
        }
    }

    return WriteJsonFile(filePath, j);
}
BOOL ExportManager::CreateTownBuildingsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData)

{
    const UINT dwellinsPerTown = ByteAt(0x05B995F + 2);
    const UINT townsNum = DwordAt(0x05B9962 + 2) / dwellinsPerTown;

    const UINT neutralTownId = townsNum - 1;
    const auto townDwellingNames = TownHandler::GetTownDwellingNames();
    const auto townDwellingDescriptions = TownHandler::GetTownDwellingDescriptions();
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
                j["era"]["towns"][std::to_string(jsonTownType)]["dwellings"][std::to_string(dwellingId)]["name"] =
                    LPCSTR_to_wstring(townDwellingNames[stringId]);
            }
            // if (!dwellingInfos[stringId].description.empty())
            {
                j["era"]["towns"][std::to_string(jsonTownType)]["dwellings"][std::to_string(dwellingId)]
                 ["description"] = LPCSTR_to_wstring(townDwellingDescriptions[stringId]);
            }
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
#endif // CREATE_JSON

void ExportDlg::CreateDlgItems()
{
    CreateOKButton();
    CreateCancelButton();

    BOOL (*selectionPanelExportFunctions[])(LPCSTR, const BOOL, const BOOL) = {
        ExportManager::CreateMonstersJson, ExportManager::CreateArtifactsJson, ExportManager::CreateObjectsJson,
        ExportManager::CreateCreatureBanksJson, ExportManager::CreateTownBuildingsJson};
    LPCSTR panelPaths[] = {ExportManager::MonsterInfo::DEFAULT_PATH, ExportManager::ArtifactInfo::DEFAULT_PATH,
                           ExportManager::ObjectInfo::DEFAULT_PATH, ExportManager::CreatureBankInfo::DEFAULT_PATH,
                           ExportManager::TownBuildingInfo::DEFAULT_PATH};
    constexpr size_t panelsNum = std::size(selectionPanelExportFunctions);
    constexpr int startId = 1;
    selectionPanels.reserve(panelsNum);

    constexpr int borderPadding = 20;

    for (size_t i = 0; i < panelsNum; i++)
    {

        const int y = borderPadding + 30 + i * PANELS_PADDING;
        libc::sprintf(h3_TextBuffer, PANEL_NAME_FORMAT, i);
        if (auto panel =
                CreateSelectionPanel(borderPadding, y, EraJS::read(h3_TextBuffer), startId + i * ITEMS_PER_PANEL,
                                     panelPaths[i], selectionPanelExportFunctions[i]))
        {
            selectionPanels.emplace_back(panel);
        }
    }
    if (selectionPanels.size())
    {
        LPCSTR text = EraJS::read("era.locale.dlg.export.item");
        CreateText(borderPadding, borderPadding, PANEL_TEXT_WIDTH, 20, text, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                   -1);
        text = EraJS::read("era.locale.dlg.export.original");
        CreateText(borderPadding + PANEL_TEXT_WIDTH, borderPadding, PANEL_TEXT_WIDTH, 20, text, NH3Dlg::Text::MEDIUM,
                   eTextColor::REGULAR, -1);
        text = EraJS::read("era.locale.dlg.export.additional");
        CreateText(borderPadding + PANEL_TEXT_WIDTH + CHECKBOX_PADDING, borderPadding, PANEL_TEXT_WIDTH, 20, text,
                   NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    }
}
ExportDlg::SelectionPanel *ExportDlg::CreateSelectionPanel(const int x, const int y, LPCSTR text, const int startId,
                                                           LPCSTR path,
                                                           BOOL (*exportFunc)(LPCSTR, const BOOL, const BOOL))

{
    SelectionPanel *panel = nullptr;
    if (panel = new SelectionPanel())
    {
        constexpr int panelTextW = PANEL_TEXT_WIDTH;

        panel->panelText = CreateText(x, y + 3, panelTextW, 20, text, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                                      startId, eTextAlignment::MIDDLE_LEFT);
        const int panelTextX = panel->panelText->GetX();

        const int originalCheckBoxX = panelTextW + CHECKBOX_PADDING / 2;
        panel->originalDataCheckBox = CreateDef(originalCheckBoxX, y, startId + 1, NH3Dlg::Assets::ON_OFF_CHECKBOX, 1);
        panel->additionalDataCheckBox =
            CreateDef(originalCheckBoxX + CHECKBOX_PADDING, y, startId + 2, NH3Dlg::Assets::ON_OFF_CHECKBOX, 1);
        panel->exportPath = path;
        panel->exportFunction = exportFunc;
    }
    return panel;
}

ExportDlg::ExportDlg(const int width, const int height, const int x, const int y, const DlgStyle *style)
    : H3Dlg(width, height, x, y, false, false)
{
    this->AddBackground(true, false, true);
    this->flags ^= 0x10; // disable dlg shadow
    CreateDlgItems();
}

ExportDlg::~ExportDlg()
{
    for (auto &i : selectionPanels)
    {
        delete i;
    }
}

BOOL ExportDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftDown())
    {
        if (auto checkBox = GetDef(msg.itemId))
        {
            P_SoundManager->ClickSound();
            checkBox->SetFrame(checkBox->GetFrame() ^ 1);
            checkBox->Draw();
            checkBox->Refresh();
            return 1;
        }
    }

    return 0;
}
VOID ExportDlg::OnOK()
{
    bool exportSuccess = false;
    std::string message, list;
    for (auto &i : selectionPanels)
    {
        if (i->exportFunction)
        {
            const BOOL originalData = i->originalDataCheckBox->GetFrame() == 1;
            const BOOL additionalData = i->additionalDataCheckBox->GetFrame() == 1;
            if (originalData || additionalData)
            {
                LPCSTR key = i->exportFunction(i->exportPath, originalData, additionalData)
                                 ? EraJS::read("era.locale.dlg.export.success")
                                 : EraJS::read("era.locale.dlg.export.error");
                libc::sprintf(h3_TextBuffer, key, i->exportPath);
                list += "\n" + std::string(h3_TextBuffer);
            }
        }
    }
    if (!list.empty())
    {
        message = EraJS::read("era.locale.dlg.export.completed");
        libc::sprintf(h3_TextBuffer, message.c_str(), list.c_str());
        if (H3Messagebox::Choice(h3_TextBuffer))
        {
        }
    }
    else
    {
        message = EraJS::read("era.locale.dlg.export.empty");
        H3Messagebox::Show(message.c_str());
    }
}
