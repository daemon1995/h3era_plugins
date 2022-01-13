#include "pch.h"
#include "..\oldHeroesHeaders\headers\era.h"
#include "..\oldHeroesHeaders\h3api\H3API.hpp"

#include <iostream>

Patcher* globalPatcher;
PatcherInstance* _GEM;

using namespace Era;
using namespace h3;


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{

    static _bool_ plugin_On = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        //if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;
            ConnectEra();
            RegisterHandler(OnGameEnter, "OnGameEnter");
            RegisterHandler(OnBattleScreenMouseClick, "OnBattleScreenMouseClick");



            globalPatcher = GetPatcher();
            _GEM = globalPatcher->CreateInstance("Bastion.daemon.plugin");
            HooksInit();


        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}