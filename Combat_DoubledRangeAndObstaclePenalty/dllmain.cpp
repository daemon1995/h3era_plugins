#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

double shootingObstaclePenalty = 0.4; // 40%
double shootingDistancePenalty = 0.4; // 40%

namespace valueKey
{
LPCSTR distance = "suft.combat.ranged.penalties.distance";
LPCSTR obstacle = "suft.combat.ranged.penalties.obstacle";
} // namespace valueKey

_LHF_(HooksInit)
{
    if (double obstaclePenalty = EraJS::readFloat(valueKey::obstacle))
    {
        shootingObstaclePenalty = obstaclePenalty;
    }

    _PI->WriteDword(0x04439A5 + 2, (DWORD)&shootingObstaclePenalty);

    if (double distancePenalty = EraJS::readFloat(valueKey::distance))
    {
        shootingDistancePenalty = distancePenalty;
    }
    _PI->WriteDword(0x04439C2 + 2, (DWORD)&shootingDistancePenalty);

    return EXEC_DEFAULT;
}
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

            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
