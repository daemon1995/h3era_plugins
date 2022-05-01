#include "pch.h"

#include <iostream>

#include ".\headers\era.h"    
#include "functions.cpp"

Patcher* globalPatcher;
PatcherInstance* _PI;


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
            
            _PI = globalPatcher->CreateInstance(const_cast<char*>("ERA.NewAdvMapItems"));
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