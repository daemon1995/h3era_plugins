#include "ArtifactHandler.h"

static H3TextTable *__stdcall LoadArtTraitsFile(HiHook *h, LPCSTR fileName)
{
    auto &state = ArtifactHandler::Handler();
    state = {};
    auto *txt = CDECL_1(H3TextTable *, h->GetDefaultFunc(), fileName);
    if (!txt) return nullptr;
    const DWORD rows = txt->CountRows();
    DWORD expectedRows = DwordAt(0x40365E + 1) / sizeof(LPCSTR);
    // The native do/while reads row 2 even when its end operand equals 8.
    // Leave short resources to the native minimum-row check; keep its ownership.
    if (rows < 3 || expectedRows < 3) return txt;
    const auto *table = *reinterpret_cast<H3ArtifactSetup **>(0x4036EC + 1);
    // Raising only the TXT loop limit does not grow the original 144-slot array.
    if (reinterpret_cast<DWORD>(table) == 0x59A2E0) expectedRows = (std::min)(expectedRows, DWORD(146));
    state.rowCount = rows;
    state.objectsCount = (std::min)(rows, expectedRows) - 2;
    state.expectedObjectsCount = expectedRows - 2;
    const DWORD bytes = (std::min)(rows, expectedRows) * sizeof(LPCSTR);
    if (DwordAt(0x40365E + 1) > bytes) _PI->WriteDword(0x40365E + 1, bytes);
    if (DwordAt(0x40369D + 2) > bytes) _PI->WriteDword(0x40369D + 2, bytes);
    if (DwordAt(0x40382E + 3) > bytes) _PI->WriteDword(0x40382E + 3, bytes);
    return txt;
}

static bool __stdcall LoadAllArtifactTxtFiles(HiHook *h)
{
    const bool loaded = CDECL_0(bool, h->GetDefaultFunc());
    if (loaded)
    {
        auto *table = *reinterpret_cast<H3ArtifactSetup **>(0x4036EC + 1);
        const auto count = ArtifactHandler::Handler().objectsCount;
        for (DWORD i = 0; table && i < count; ++i) EraJS::ReadArtifact(table[i], static_cast<int>(i), static_cast<int>(count));
    }
    return loaded;
}
int ArtifactHandler::GetArtifactsNumber() noexcept { return artifactsHandler.objectsCount; }
void ArtifactHandler::Init()
{
    _PI->WriteHiHook(0x45C72C, CDECL_, LoadAllArtifactTxtFiles);
    _PI->WriteHiHook(0x40362E, CDECL_, LoadArtTraitsFile);
}
