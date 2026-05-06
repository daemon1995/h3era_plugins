// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}
constexpr int maxTerrainTypes = 25;
struct BlockedInUnderworldSettings
{
    BOOL blocked[maxTerrainTypes]{};
} settings;

static void __stdcall RMGZone_InitGround(HiHook *h, H3RmgZoneGenerator *_this)
{

    // check if zone has town there
    const int townType = _this->townType;
    if (townType > -1 && townType < maxTerrainTypes)
    {
        // get town terrain type
        const int townTerrainType = ValueAt<int *>(0x0532F9E + 3)[townType];

        // if terrain type isn't blocked then set it as the result
        if (!settings.blocked[townTerrainType])
        {
            _this->ground = townTerrainType;
            return;
        }
    }

    // otherwise, call original function to set default terrain type from template
    THISCALL_1(void, h->GetDefaultFunc(), _this);
}

static _ERH_(OnAfterWog)
{

    if (EraJS::readInt("suft.rmg.zone_terrain.always_match_to_town"))
    {
        _PI->WriteHiHook(0x00532F80, THISCALL_, RMGZone_InitGround);

        libc::memset(settings.blocked, 0, sizeof(settings.blocked));

        for (int i = 0; i < maxTerrainTypes; i++)
        {
            bool readSuccess = false;

            libc::sprintf(h3_TextBuffer, "suft.rmg.zone_terrain.block_in_underworld.%i", i);
            BOOL readResult = EraJS::readInt(h3_TextBuffer, readSuccess);
            if (readSuccess && readResult)
                settings.blocked[i] = true;
        }
    }
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
            // _PI->WriteLoHook(0x4EEAF2, HooksInit); SoD way; used for old plugins and early hooks set
            _REH_(OnAfterWog); // ERA way; used for new plugins and late hooks set
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
