#define NH3API_FLAG_INLINE_HEADERS
#include "pch.h"
#include <nh3api\core.hpp>

void __stdcall HelloWorld(HiHook *hook, void *_this) noexcept
{

    // auto instance = MyInstance::Get().GetPatcherInstance();

    NormalDialog("Hello, World!");

    akCreatureTypeTraits[TCreatureType::CREATURE_BALLISTA].cost[6] = 1500;
}

extern "C" NH3API_DLLEXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        Patcher *patcher = GetPatcher();
        if (patcher == nullptr)
            return false;
        PatcherInstance *instance = patcher->CreateInstance("HD.Plugin.HelloWorld");
        if (instance)
        {
            instance->WriteHiHook(0x4EEE31, CALL_, EXTENDED_, THISCALL_, &HelloWorld);
        }
    }
    return true;
}
