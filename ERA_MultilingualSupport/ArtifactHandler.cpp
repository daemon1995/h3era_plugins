#include "ArtifactHandler.h"
#include "pch.h"

void ArtifactHandler::Init()
{
    static BOOL firstCall = true;

    if (!firstCall)
        return;
    firstCall = false;

    const int artsNum = IntAt(0x49DD8E + 2) / 4;
    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *eventTable = *reinterpret_cast<LPCSTR **>(0x49F51B + 3);

    for (size_t i = 0; i < artsNum; i++)
    {
        auto &artInfo = P_ArtifactSetup[i];
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
}
