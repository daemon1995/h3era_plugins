#include "MonsterHandler.h"
#include "pch.h"
#ifdef CREATE_JSON

#include <fstream>
#include <thread>

// Структура для хранения данных о монстре
struct MonsterInfo
{
    std::string singularName;
    std::string pluralName;
    std::string description;
};

// Функция для создания JSON-документа
void CreateMonstersJson(const std::vector<MonsterInfo> &monsters, const std::string &filePath)
{
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
        outFile << j.dump(4); // 4 — это отступ для красивого форматирования
        outFile.close();
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
}
#endif // CREATE_JSON

bool __stdcall LoadCranimTxt(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    if (result)
    {
        const int MAX_MON_ID = IntAt(0x4A1657);
        H3CreatureAnimation *animTable = *reinterpret_cast<H3CreatureAnimation **>(0x67FF74);
        for (size_t i = 0; i < MAX_MON_ID; i++)
        {
            //   animTable[i].walkAnimationTime =1;
            // P_CreatureA::Get() = 12;
        }
    }

    return result;
}
bool __stdcall WoG_OnMapReset(HiHook *h, int a1)
{
    bool result = CDECL_1(bool, h->GetDefaultFunc(), a1);

    if (result)
    {
        static BOOL firstCall = true;

        if (!firstCall)
            return result;
        firstCall = false;

        const int MAX_MON_ID = IntAt(0x4A1657);

        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        //  auto table = H3CreatureInformation::Get();
        const auto singleNames = *reinterpret_cast<LPCSTR **>(0x047B12C + 1);
        const auto pluralNames = *reinterpret_cast<LPCSTR **>(0x047B10C + 1);
        const auto descriptions = *reinterpret_cast<LPCSTR **>(0x047B0EC + 1);

#ifdef CREATE_JSON
        std::vector<MonsterInfo> monsters;
        monsters.resize(MAX_MON_ID);
#endif // CREATE_JSON

        for (size_t i = 0; i < MAX_MON_ID; i++)
        {
#ifdef CREATE_JSON
            monsters[i] = {singleNames[i], pluralNames[i], descriptions[i]};
            ;
#endif // CREATE_JSON
            sprintf(h3_TextBuffer, "era.monsters.%d.name.singular", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                singleNames[i] = readResult;

            sprintf(h3_TextBuffer, "era.monsters.%d.name.plural", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                pluralNames[i] = readResult;

            sprintf(h3_TextBuffer, "era.monsters.%d.name.description", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                descriptions[i] = readResult;
        }
#ifdef CREATE_JSON
        std::thread th(CreateMonstersJson, monsters, "mosterNames.json");
        //        CreateMonstersJson(monsters, "mosterNames.json");
        th.detach();
#endif // CREATE_JSON
    }

    return result;
}

void MonsterHandler::Init()
{
    _PI->WriteHiHook(0x07117CA, CDECL_, WoG_OnMapReset); //
                                                         //  _PI->WriteHiHook(0x04EDF4B, CDECL_, LoadCranimTxt);   //
}
