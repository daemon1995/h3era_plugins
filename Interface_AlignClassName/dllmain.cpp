// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <string>
Patcher* globalPatcher;
PatcherInstance* _PI;


int __cdecl DlgHeroUpdate_OnSettingClassName(HiHook* h, char* buffer, char* text, DWORD level, DWORD className)
{
    
    std::string currentText(text);
    std::size_t pos = currentText.find(" %s");
    if (pos != std::string::npos)
    {
        std::string newStr("Custom Class Name and %s\n");
        currentText.erase(pos, 3);
        return CDECL_4(int, h->GetDefaultFunc(), buffer, newStr.append(currentText).c_str(), className, level);
    }

    return CDECL_4(int, h->GetDefaultFunc(), buffer, text, level, className);

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static bool plugin_On = 0;

    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("EraPlugin.ChangeHeroClassView.daemon_n");

            // hero update screen proc -> setting class name; CALL_ hook type, CDECL cause this is sprintf C-lib   
            _PI->WriteHiHook(0x4E1DFB, CDECL_, DlgHeroUpdate_OnSettingClassName);
            // set hero class name y pos
            _PI->WriteByte(0x04DEB45 + 1, 44);
            // set hero name y pos
            _PI->WriteByte(0x04DEAF1 + 1, 24);
           // _PI->WriteByte(0x4DEB3E + 1, 46);
            
            
            
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

