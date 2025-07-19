#include "HandlersList.h"

bool __stdcall LoadArtTraitsTxt(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    if (result)
    {
        h->Undo();

        const int artsNum = IntAt(0x49DD8E + 2) / 4;
        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        LPCSTR *eventTable = ArtifactHandler::GetEventTable();
        const int lastEventIndex = H3AdveventText::Get()->end() - H3AdveventText::Get()->begin() >> 2;
        for (size_t i = 0; i < artsNum; i++)
        {
            auto &artInfo = P_ArtifactSetup->Get()[i];

            sprintf(h3_TextBuffer, ArtifactHandler::formats::NAME, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                artInfo.name = readResult;
            }

            sprintf(h3_TextBuffer, ArtifactHandler::formats::DESCRIPTION, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                artInfo.description = readResult;
            }

            sprintf(h3_TextBuffer, ArtifactHandler::formats::EVENT, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                eventTable[i] = readResult;
            }
            else if (i > lastEventIndex)
            {
                eventTable[i] = h3_NullString; // if not read, set to nullptr
            }
        }
    }

    return result;
}

inline LPCSTR *ArtifactHandler::GetEventTable() noexcept
{
    return *reinterpret_cast<LPCSTR **>(0x49F51B + 3);
}

void ArtifactHandler::Init()
{
    // Warning: this hook is after all txt read
    _PI->WriteHiHook(0x04EDEA2, CDECL_, LoadArtTraitsTxt);
}
