// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
using namespace h3;

void __stdcall OnAfterWog(Era::TEvent* event);
_LHF_(Crtraits_RightAfterLoad);

_LHF_(HooksInit)
{

    LanguageSelectionDlg::Init();
    
    MonsterHandler::Init();


    return EXEC_DEFAULT;
}

Patcher* globalPatcher;
PatcherInstance* _PI;
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    static bool pluginIsOn = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;

            Era::ConnectEra();
            if (Era::GetVersionNum()>=3906)
            {
                globalPatcher = GetPatcher();
                _PI = globalPatcher->CreateInstance("ERA.Multilingual.plugin");
                _PI->WriteLoHook(0x4EEAF2, HooksInit);
                Era::RegisterHandler(OnAfterWog, "OnAfterWog ");
                _PI->WriteLoHook(0x4EDE9D, Crtraits_RightAfterLoad);

            }


        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

