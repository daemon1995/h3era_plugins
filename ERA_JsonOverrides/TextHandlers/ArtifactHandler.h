#pragma once
#include "HandlersList.h"

class ArtifactHandler
{
    static H3TextTable *__stdcall LoadArtTraitsFile(HiHook *h, LPCSTR fileName)
    {
        auto *txt = THISCALL_1(H3TextTable *, h->GetDefaultFunc(), fileName);
        if (txt)
        {
            const DWORD rows = txt->CountRows();
            if (rows < 3) return txt;
            const DWORD rowCount4 = rows * sizeof(LPCSTR);
            if (DwordAt(0x44CCA8) > rowCount4) _PI->WriteDword(0x44CCA8, rowCount4);
            if (DwordAt(0x44CACA) > rowCount4) _PI->WriteDword(0x44CACA, rowCount4);
        }
        return txt;
    }
    static int __stdcall AfterReadAllTxtFiles(HiHook *h)
    {
        // 0x50C380 returns a directory/status integer, not a TXT-load boolean.
        const int result = CDECL_0(int, h->GetDefaultFunc());
        h->Undo();
        auto *artifacts = H3ArtifactSetup::Get();
        const int count = GetArtifactsNumber();
        auto *events = GetEventTable();
        const int eventCount = EraJS::GameTables::ArtifactEventCapacity();
        for (int i = 0; artifacts && i < count; ++i)
        {
            EraJS::ReadArtifact(artifacts[i], i, count);
            if (events && i < eventCount)
                EraJS::ReadField(events[i], EraJS::ArtifactSchema::event, i);
        }
        // Missing JSON keeps the native ARTEVENT entry; ADVEVENT is unrelated.
        return result;
    }
  public:
    static void Init()
    {
        _PI->WriteHiHook(0x4EE036, CDECL_, AfterReadAllTxtFiles);
        _PI->WriteHiHook(0x44CA43, THISCALL_, LoadArtTraitsFile);
    }
    static LPCSTR *GetEventTable() noexcept { return EraJS::GameTables::ArtifactEvents(); }
    static int GetArtifactsNumber() noexcept { return IntAt(0x717020); }
};
