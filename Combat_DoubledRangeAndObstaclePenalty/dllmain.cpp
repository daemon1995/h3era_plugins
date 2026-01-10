#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

const double shootingDistancePenalty = 0.25; // 25% -- decreases damage by 4 times
const double shootingObstaclePenalty = 0.25; // 25% -- decreases damage by 4 times

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginInitialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginInitialized)
        {
            pluginInitialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::instanceName);
            Era::ConnectEra(hModule, dllText::instanceName);

            _PI->WriteDword(0x04439A5 + 2, (DWORD)&shootingObstaclePenalty);
            _PI->WriteDword(0x04439C2 + 2, (DWORD)&shootingDistancePenalty);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
