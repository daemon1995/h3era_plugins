// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"


//extern void __stdcall ReallocProhibitionTables(Era::TEvent* event);
extern void __stdcall LoadConfigs(Era::TEvent* event);


Patcher* globalPatcher;
PatcherInstance* _PI;


bool __stdcall LoadArtraitTxt(HiHook* h)
{

    bool result = CDECL_0(bool, h->GetDefaultFunc());
    //  P_ArtifactSetup[279].name = "help";

    bool readResult = false;
    const int artQtyDword = DwordAt(0x44CCA8);
    const int artQty = artQtyDword / 4 - 2;

    for (size_t i = 0; i <= artQty; i++)
    {
        //if (i == 269)
        //{
        //    H3ArtifactSetup setup = P_ArtifactSetup[i];
        //    int a = 5;
        //}


        //LPCSTR artName = EraJS::read(H3String::Format("artifacts.%d.name", i).String(), readResult);
        //if (readResult)
        //{
        //    P_ArtifactSetup[i].name = artName;// Era::tr(H3String::Format("artifacts.%d.name", i).String());
        //}
    }

    //for (size_t i = 100; i <= artQty; i++)
    //{
    //    P_ArtifactSetup[i].disabled = true;
    //}

    return result;
}

void __stdcall Z_OnAfterCreateWindow(Era::TEvent* event)
{
    //ReallocProhibitionTables(event);
    LoadConfigs(event);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static BOOL plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;

            //! Get the global patcher
            globalPatcher = GetPatcher();

            //! Create an instance of the plugin
            _PI = globalPatcher->CreateInstance("js_Emerald");


            Era::RegisterHandler(Z_OnAfterCreateWindow, "OnAfterCreateWindow");

            //_PI->WriteHiHook(0x044CA20, CDECL_, LoadArtraitTxt);
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
