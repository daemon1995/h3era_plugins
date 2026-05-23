#define _H3API_PLUGINS_
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

struct TxtHandler
{
    DWORD expectedRowCount;
    DWORD expectedObjectsCount;
    DWORD rowCount;
    DWORD objectsCount;

} artifactsHandler;
H3TextTable *__stdcall LoadArtTraitsFile(HiHook *h, LPCSTR fileName)
{
    H3TextTable *artraitsTxt = CDECL_1(H3TextTable *, h->GetDefaultFunc(), fileName);
    if (artraitsTxt)
    {

        const DWORD rowCount = artraitsTxt->CountRows();
        const DWORD expectedRowCount = IntAt(0x040365E + 1) >> 2;

        artifactsHandler.rowCount = rowCount;
        artifactsHandler.objectsCount = rowCount - 2;
        artifactsHandler.expectedRowCount = expectedRowCount;
        artifactsHandler.expectedObjectsCount = expectedRowCount - 2;

        const DWORD newSizeToIterate = rowCount << 2;

        // if txt file has less lines than patched lines to read we fix it
        if (IntAt(0x040365E + 1) > newSizeToIterate)
        {
            _PI->WriteDword(0x040365E + 1, newSizeToIterate);
        }
        if (IntAt(0x040369D + 2) > newSizeToIterate)
        {

            _PI->WriteDword(0x040369D + 2, newSizeToIterate);
        }

        if (IntAt(0x040382E + 3) > newSizeToIterate)
        {
            _PI->WriteDword(0x040382E + 3, newSizeToIterate);
        }
    }
    return artraitsTxt;
}
#include <vector>

std::vector<std::string> artNames;

bool __stdcall LoadAllArtifactFiles(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    // if (result)
    {
        h->Undo();

        const DWORD artsCount = artifactsHandler.objectsCount;
        const DWORD expectedObjectsCount = artifactsHandler.expectedObjectsCount;

        if (artsCount >= expectedObjectsCount)
            return result;

        H3ArtifactSetup *artSetupTable = *reinterpret_cast<H3ArtifactSetup **>(0x04036EC + 1);
        char buffer[0x512]{};

        artNames.resize(expectedObjectsCount);
        for (size_t i = artsCount; i < expectedObjectsCount; i++)
        {
            auto &artInfo = artSetupTable[i];
            //           artInfo = {};
            sprintf(buffer, "tum art %d name", i);
            artNames[i] = buffer;
            artInfo.name = artNames[i].c_str();
            artInfo.description = artNames[i].c_str();
            artInfo.position = eArtifactPositions(rand() % 9 + 1);
            artInfo.cost = 0;
            artInfo.type = eArtifactType::MAJOR;
            artInfo.disabled = false;
            artInfo.comboArtifactId = eCombinationArtifacts::NONE;
            artInfo.partOfComboArtifactId = eCombinationArtifacts::NONE;
            //		artInfo.description = "";
            // memcpy(&artInfo, &artSetupTable[eArtifact::BLACKSHARD_OF_THE_DEAD_KNIGHT], sizeof(artInfo));
        }
    }

    return result;
}

_LHF_(HooksInit)
{
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            // credits: RoseKavalier
            const IMAGE_DOS_HEADER *pDOSHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(0x400000);
            const IMAGE_NT_HEADERS *pNTHeaders =
                reinterpret_cast<IMAGE_NT_HEADERS *>((PBYTE(pDOSHeader) + pDOSHeader->e_lfanew));
            const DWORD entry = pNTHeaders->OptionalHeader.AddressOfEntryPoint;
            if (entry != 0xE84D4 || IntAt(0x4E84D4 + 1) != 0x596ED4)
            {
                return TRUE;
            }
            initialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::instanceName);
            _PI->WriteHiHook(0x045C72C, CDECL_, LoadAllArtifactFiles);
            _PI->WriteHiHook(0x040362E, CDECL_, LoadArtTraitsFile);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
