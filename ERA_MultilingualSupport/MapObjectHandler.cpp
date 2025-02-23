#include "MapObjectHandler.h"
#include "pch.h"

void MapObjectHandler::Init()
{
    static BOOL firstCall = true;

    if (!firstCall)
        return;
    firstCall = false;

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = H3DwellingNames1::Get();
    const int dwellingsNum = IntAt(0x49DD8E + 2) / 4;

    // load dwelling names
    for (size_t i = 0; i < dwellingsNum; i++)
    {
        sprintf(h3_TextBuffer, "era.objects.17.%d", i);
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
        sprintf(h3_TextBuffer, "era.objects.%d", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }
}
