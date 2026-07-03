// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

#include "TextHandlers/ArtifactHandler.h"
#include "TextHandlers/HeroHandler.h"
#include "TextHandlers/MapObjectHandler.h"
#include "TextHandlers/MonsterHandler.h"
#include "TextHandlers/SpellHandler.h"
#include "TextHandlers/TownHandler.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
constexpr LPCSTR PLUGIN_AUTHOR = "daemon_n";
constexpr LPCSTR PLUGIN_VERSION = "1.1.0";
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";
} // namespace dllText

_ERH_(OnReportVersion)
{
    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}

static _ERH_(OnAfterWog)
{
    MonsterHandler::Init();
    ArtifactHandler::Init();
    SpellHandler::Init();
}

static _ERH_(OnAfterCreateWindow)
{
    HeroHandler::Init();
}
static _LHF_(HooksInit)
{
    MapObjectHandler::Init();
    TownHandler::Init();
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
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            _REH_(OnReportVersion);

            _REH_(OnAfterWog); // ERA way; used for new plugins and late hooks set
            _REH_(OnAfterCreateWindow);
            _PI->WriteLoHook(0x4EEAF2, HooksInit); // SoD way; used for old plugins and early hooks set
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
