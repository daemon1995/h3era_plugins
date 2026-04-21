// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#define ERA_MODLIST
#include "framework.h"
#pragma comment(linker, "/EXPORT:GameModIsLoaded=_GameModIsLoaded@4")

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
LPCSTR pluginVersion = "0.7.0";
} // namespace dllText
class CombatEmulator
{
  public:
    static void Init();
};

DllExport BOOL __stdcall GameModIsLoaded(LPCSTR modName)
{
    if (!modName || libc::strcmp(modName, h3_NullString) == 0)
    {
        return false;
    }

    const auto &modList = modList::GetEraModList();
    for (auto &i : modList)
    {
        if (libc::strcmpi(modName, i.c_str()) == 0)
        {
            return true;
        }
    }
    return false;
}

_LHF_(HooksInit)
{
    CombatEmulator::Init();

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
            initialized = true;
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
