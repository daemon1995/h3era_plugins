#include "ArtifactHandler.h"
#include "pch.h"

#ifdef CREATE_JSON

#include <fstream>
#include <thread>
// Структура для хранения данных о монстре
struct ArtifactInfo
{
    std::string name;
    std::string description;
    std::string event;
};

// Функция для создания JSON-документа
void CreateArtifactsJson(const std::vector<ArtifactInfo> &artifacts, const std::string &filePath)
{
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
        outFile << j.dump(4); // 4 — это отступ для красивого форматирования
        outFile.close();
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
}
#endif // CREATE_JSON
bool __stdcall LoadArtTraitsTxt(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());

    if (result)
    {
        h->Undo();

        const int artsNum = IntAt(0x49DD8E + 2) / 4;
        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        LPCSTR *eventTable = *reinterpret_cast<LPCSTR **>(0x49F51B + 3);
#ifdef CREATE_JSON
        std::vector<ArtifactInfo> artifacts;
        artifacts.resize(artsNum);
#endif // CREATE_JSON
        for (size_t i = 0; i < artsNum; i++)
        {
            auto &artInfo = P_ArtifactSetup[i];
#ifdef CREATE_JSON
            artifacts[i] = {artInfo.name, artInfo.description, eventTable[i]};
            ;
#endif // CREATE_JSON
            sprintf(h3_TextBuffer, "era.artifacts.%d.name", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                artInfo.name = readResult;

            sprintf(h3_TextBuffer, "era.artifacts.%d.description", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                artInfo.description = readResult;

            sprintf(h3_TextBuffer, "era.artifacts.%d.event", i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                eventTable[i] = readResult;
        }
#ifdef CREATE_JSON
        std::thread th(CreateArtifactsJson, artifacts, "artifactNames.json");
        //  CreateArtifactsJson(artifacts, "artifactNames.json");
        th.detach();
#endif // CREATE_JSON
    }

    return result;
}
void ArtifactHandler::Init()
{

    // Warning: this hook is after all txt read
    _PI->WriteHiHook(0x04EDEA2, CDECL_, LoadArtTraitsTxt);
}
