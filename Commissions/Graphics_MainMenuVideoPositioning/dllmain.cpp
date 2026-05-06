// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

static _LHF_(MainMenu_OpenVideo)
{

    c->edx = 0;
    IntAt(c->ebp + 0x8) = 0;

    return EXEC_DEFAULT;
}

inline void MainProc()
{

    if (H3GameWidth::Get() < 808 && H3GameHeight::Get() < 608)
    {
        return;
    }
    // if the video menu patch is applied, then no need to apply the position patch
    const auto videoMenuApplied = WordAt(0x00597876) != 0x507C;
    if (!videoMenuApplied)
    {
        return;
    }

    _PI->WriteLoHook(0x00597876, MainMenu_OpenVideo);

    // BOOL enableChangePos = false;

    // const int x = EraJS::readInt("era.acredit_pos.x");
    // if (x < 0)
    //{
    //     pluginSettings.acreditX = Clamp(0, (H3GameWidth::Get() - 800) / 2 + x, H3GameWidth::Get() - 800);
    //     enableChangePos = true;
    // }
    // const int y = EraJS::readInt("era.acredit_pos.y");
    // if (y < 0)
    //{
    //     pluginSettings.acreditY = Clamp(0, (H3GameHeight::Get() - 600) / 2 + y, H3GameHeight::Get() - 600);
    //     enableChangePos = true;
    // }

    // if (enableChangePos)
    //{
    // }
}

static _LHF_(HooksInit)
{
    MainProc();
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
            _PI->WriteLoHook(0x4EEAF2, HooksInit); // SoD way; used for old plugins and early hooks set
            break;
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
