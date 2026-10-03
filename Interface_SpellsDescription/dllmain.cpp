#include "SpellDescriptions.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace
{
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";

_LHF_(HooksInit)
{
    SpellDescriptions::Install(_PI);
    return EXEC_DEFAULT;
}
} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        static bool initialized = false;
        if (!initialized)
        {
            initialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
            Era::ConnectEra(module, INSTANCE_NAME);
            // Consumers may resolve the API during their DLL initialization.
            SpellDescriptions::PublishApi();
            // Same game initialization point as before extraction.
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    }
    return TRUE;
}
