#include "ArtifactHandler.h"
#include "pch.h"

#ifdef CREATE_JSON
#include "ExportManager.h"
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
        std::vector<ExportManager::ArtifactInfo> artifacts;
        artifacts.resize(artsNum);
#endif // CREATE_JSON
        for (size_t i = 0; i < artsNum; i++)
        {
            auto &artInfo = P_ArtifactSetup[i];
#ifdef CREATE_JSON
            artifacts[i] = {ExportManager::LPCSTR_to_wstring(artInfo.name),
                            ExportManager::LPCSTR_to_wstring(artInfo.description),
                            ExportManager::LPCSTR_to_wstring(eventTable[i])};
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
        ExportManager::CreateArtifactsJson(artifacts);
#endif // CREATE_JSON
    }

    return result;
}
void ArtifactHandler::Init()
{

    // Warning: this hook is after all txt read
    _PI->WriteHiHook(0x04EDEA2, CDECL_, LoadArtTraitsTxt);
}
