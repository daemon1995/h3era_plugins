#include "pch.h"
#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_
#define _H3API_MESSAGES_
#define _H3API_EXCEPTION_
#include <iostream>

#include ".\h3api_single\H3API.hpp"
#include ".\headers\era.h"    
#include "functions.cpp"

Patcher* globalPatcher;
PatcherInstance* _PI;


//bool isHdMod = Era::TPluginExists((char)*"hd_wog.dll");

using namespace Era;
using namespace h3;


void HooksInit()

{  
   //  _GEM->WriteLoHook(0x4A99C0, OnHeroPickupRes);
    _PI->WriteLoHook(0x4021B2, OnAdventureDlgCreate);
    _PI->WriteLoHook(0x403F00, OnResourceBarDlgUpdate);
    _PI->WriteLoHook(0x417380, OnResourceBarDlgUpdate);
    _PI->WriteLoHook(0x559270, BeforeHotseatMsgBox);

}

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
       
            globalPatcher = GetPatcher();
            
            _PI = globalPatcher->CreateInstance("ERA.NewAdvMapItems");
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