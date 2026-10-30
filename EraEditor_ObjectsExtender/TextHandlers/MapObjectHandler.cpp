#include "MapObjectHandler.h"
#include "TextOverrides.h"
#include "../../headers/Era/ObjectText.hpp"

namespace
{
MapObjectData *boundTypes = nullptr;
LPCSTR *boundBanks = nullptr;
size_t boundBankCount = 0;
}

static void __stdcall Load_Objnames_TXT(HiHook *h)
{
    editorObjects::TextOverrides::Forget(boundTypes, size_t(limits::OBJECTS) * sizeof(MapObjectData));
    CDECL_0(void, h->GetDefaultFunc());
    auto *table = MapObjectData::Get();
    boundTypes = table;
    for (int i = 0; table && i < limits::OBJECTS; ++i)
    {
        char key[80];
        std::snprintf(key, sizeof(key), MapObjectHandler::formats::OBJECTS, i);
        editorObjects::TextOverrides::Apply(table[i].objectName, key);
    }
}
static void __stdcall Load_CrBanks_TXT(HiHook *h)
{
    editorObjects::TextOverrides::Forget(boundBanks, boundBankCount * sizeof(LPCSTR));
    boundBanks = nullptr;
    boundBankCount = 0;
    CDECL_0(void, h->GetDefaultFunc());
    auto *names = *reinterpret_cast<LPCSTR **>(0x48D496 + 1);
    const DWORD end = DwordAt(0x48D4D0 + 2);
    const DWORD base = reinterpret_cast<DWORD>(names);
    if (!names || end < base || (end - base) % sizeof(LPCSTR)) return;
    const size_t count = (end - base) / sizeof(LPCSTR);
    if (count > size_t(object_config::SUBTYPE_COUNT_LIMIT)) return;
    boundBanks = names;
    boundBankCount = count;
    for (size_t i = 0; i < count; ++i)
    {
        char key[80];
        std::snprintf(key, sizeof(key), "RMG.objectGeneration.16.%d.name", static_cast<int>(i));
        editorObjects::TextOverrides::Apply(names[i], key);
    }
}
void MapObjectHandler::Init()
{
    _PI->WriteHiHook(0x45C713, CDECL_, Load_Objnames_TXT);
    _PI->WriteHiHook(0x48D3A2, CDECL_, Load_CrBanks_TXT);
}
