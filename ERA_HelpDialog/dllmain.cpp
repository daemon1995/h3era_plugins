// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

namespace dllText
{
    LPCSTR PLUGIN_VERSION = "0.01";
    LPCSTR INSTANCE_NAME = "EraPlugin.Help.daemon_n";
    LPCSTR PLUGIN_AUTHOR = "daemon_n";
    //	const char* PROJECT_NAME = "$(ProjectName)";
    LPCSTR  PLUGIN_DATA = __DATE__;
}



Patcher* globalPatcher;
PatcherInstance* _PI;


// main window proc 
_LHF_(MainWindow_F1)
{


    if (!main::HelpDlg::instance)
    {
        P_SoundManager->ClickSound();
        // GetKeyState Call


        
        main::HelpDlg dlg(800, 600);
        main::HelpDlg::instance = &dlg;
        dlg.Start();
        main::HelpDlg::instance = nullptr;

        if (!(STDCALL_1(SHORT, PtrAt(0x63A294), VK_F1) & 0x0800))
        {
            
            //Era::ExecErmCmd("IF:L^^");
            // skip "windows help" dlg
            // and pass F1 to default ingame proc 
        }



    }
    c->return_address = 0x4F877D;
    return NO_EXEC_DEFAULT;

    return EXEC_DEFAULT;

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    static BOOL pluginIsOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;

            Era::ConnectEra();
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            _PI->WriteLoHook(0x4F8751, MainWindow_F1);

        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

