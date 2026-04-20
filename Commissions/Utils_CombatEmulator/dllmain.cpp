// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#define ERA_MODLIST
#include "framework.h"
#pragma comment(linker, "/EXPORT:GameModIsLoaded=_GameModIsLoaded@4")

DllExport BOOL __stdcall GameModIsLoaded(LPCSTR modName)
{
    if (!modName)
    {
        return false;
    }

    std::string modNameStr(modName);
    std::transform(modNameStr.begin(), modNameStr.end(), modNameStr.begin(), ::tolower);

    auto modList = modList::GetEraModList(true);
    return std::find(modList.begin(), modList.end(), modName) != modList.end();
}

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
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
