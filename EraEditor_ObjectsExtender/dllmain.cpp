#include "framework.h"
#include "ObjectExtenders/DwellingsExtender.h"
#include "ObjectDefinitions.h"
#include "ObjectText.h"
#include "TextHandlers/ArtifactHandler.h"
#include "TextHandlers/MapObjectHandler.h"
#include "TextHandlers/TextOverrides.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    static bool initialized = false;
    if (reason == DLL_PROCESS_ATTACH && !initialized)
    {
        // ERA may scan editor plugins from the game. Import the editor runtime only in its own process.
        if (!Eramap::IsMapEditor())
            return TRUE;
        constexpr LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
        if (!Eramap::ConnectEra(module, instanceName))
            return FALSE;
        globalPatcher = GetPatcher();
        if (!globalPatcher)
            return FALSE;
        _PI = globalPatcher->CreateInstance(instanceName);
        if (!_PI)
            return FALSE;
        initialized = true;
        editorObjects::TextOverrides::Init();
        ArtifactHandler::Init();
        MapObjectHandler::Init();
        editorObjects::ObjectDefinitions::Init();
        editorObjects::DwellingsExtender::Init();
        editorObjects::ObjectText::Init();
    }
    return TRUE;
}
