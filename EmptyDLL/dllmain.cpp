// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "framework.h"




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


        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

