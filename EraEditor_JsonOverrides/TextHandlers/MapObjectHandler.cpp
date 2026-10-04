#include "MapObjectHandler.h"

static void __stdcall Load_Objnames_TXT(HiHook *h)
{
    CDECL_0(void, h->GetDefaultFunc());
    auto *table = MapObjectData::Get();
    for (int i = 0; table && i < limits::OBJECTS; ++i)
        EraJS::ReadField(table[i].objectName, MapObjectHandler::formats::OBJECTS, i);
}
static void __stdcall Load_CrBanks_TXT(HiHook *h)
{
    CDECL_0(void, h->GetDefaultFunc());
    auto *names = *reinterpret_cast<LPCSTR **>(0x48D496 + 1);
    const DWORD end = DwordAt(0x48D4D0 + 2);
    const DWORD base = reinterpret_cast<DWORD>(names);
    if (!names || end < base || (end - base) % sizeof(LPCSTR)) return;
    const size_t count = (end - base) / sizeof(LPCSTR);
    EraJS::ReadIndexedText(names, count, MapObjectHandler::formats::CREATURE_BANKS,
                           static_cast<int>(eObject::CREATURE_BANK));
}
void MapObjectHandler::Init()
{
    _PI->WriteHiHook(0x45C713, CDECL_, Load_Objnames_TXT);
    _PI->WriteHiHook(0x48D3A2, CDECL_, Load_CrBanks_TXT);
}
