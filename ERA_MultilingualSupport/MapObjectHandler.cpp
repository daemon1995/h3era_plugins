#include "MapObjectHandler.h"
#include "pch.h"

#ifdef CREATE_JSON
#include "ExportManager.h"
#endif // CREATE_JSON
void MapObjectHandler::Init()
{

    bool readSuccess = false;
    LPCSTR readResult = nullptr;
    LPCSTR *table = H3DwellingNames1::Get();
    const int dwellings1Num = IntAt(0x0405CCE + 2) / 4;

#ifdef CREATE_JSON
    std::vector<std::string> objects, dwellings1; // , dwellings4;
    objects.resize(limits::OBJECTS);
    dwellings1.resize(dwellings1Num);
#endif // CREATE_JSON
    // load dwelling names
    for (size_t i = 0; i < dwellings1Num; i++)
    {
#ifdef CREATE_JSON
        dwellings1[i] = ExportManager::LPCSTR_to_wstring(table[i]);
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.dwellings1.%d", i);
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
#ifdef CREATE_JSON
        objects[i] = ExportManager::LPCSTR_to_wstring(table[i]);
#endif // CREATE_JSON
        sprintf(h3_TextBuffer, "era.objects.%d", i);
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess)
            table[i] = readResult;
    }
#ifdef CREATE_JSON
    ExportManager::CreateObjectsJson(objects, dwellings1);
#endif // CREATE_JSON
}
