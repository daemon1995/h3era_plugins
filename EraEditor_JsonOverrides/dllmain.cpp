#include "framework.h"
#include "TextHandlers/ArtifactHandler.h"
#include "TextHandlers/MapObjectHandler.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
constexpr LPCSTR PLUGIN_AUTHOR = "daemon_n";
constexpr LPCSTR PLUGIN_VERSION = "1.2";
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";
} // namespace dllText
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            // ERA also scans this directory in the game: leave the DLL inert there.
            // The API is imported lazily, so no editor runtime runs before this guard.
            if (!Eramap::IsMapEditor()) return TRUE;
            if (!Eramap::ConnectEra(hModule, dllText::INSTANCE_NAME)) return FALSE;
            initialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            ArtifactHandler::Init();
            MapObjectHandler::Init();
            // TownHandler::Init();
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
