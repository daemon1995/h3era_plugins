// dllmain.cpp : Определяет точку входа для приложения DLL.
//#include "header.h"

#include "era_menu_creator.cpp"
#include <fstream>

using namespace h3;
//h3::H3String prefix = "era_menu";
Patcher* globalPatcher;
PatcherInstance* _PI;
const bool JSONKeyExists(H3String str) { return !str.Empty() && !str.Equals(Era::tr(str.String()));  }

bool ReadModList()
{

    char buff[256];
    GetCurrentDirectoryA(200, buff);
    H3String path = buff;
    path.Append("\\Mods\\list.txt");
    std::ifstream fread(path.String(), std::ifstream::in);
  //  H3SEHandler seh;

    //try
    bool is_open = fread.is_open();
    if (is_open)
    {
        //return 1;

        std::string str;
        int i = 0;
        dlg_mods.reserve(25);

        for ( i = 0; fread.good(); i++)
        {
            std::getline(fread, str);
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);

            H3String key_name = BASE_KEY + str.c_str() + ".name"; //Era::tr("era_menu.internal.read_error");
            std::transform(key_name.begin(), key_name.end(), key_name.begin(), ::tolower);

            if (JSONKeyExists(key_name) && !str.empty())
            {
              //  H3String key_hint = base_key + str.c_str() + ".hint"; //Era::tr("era_menu.internal.read_error");
              //  H3String key_popup = base_key + str.c_str() + ".popup"; //Era::tr("era_menu.internal.read_error");
              //  H3Messagebox();

                dlg_mods.emplace_back(key_name.Append(str.c_str()).String());//, key_hint.String(), key_popup.String()));
            }


        }
        dlg_mods.shrink_to_fit();
        fread.close();

    }
    else
    {
     //   H3Messagebox(Era::tr("era_menu.internal.read_error"));

    }

    return is_open;

}


int  __stdcall Dlg_SelectScenario_Dlg(HiHook *h, H3Msg *msg)
{
    if (msg->itemId==4500 && msg->subtype==eMsgSubtype::LBUTTON_CLICK)
    {
        CreateSth();

    }
         

    return THISCALL_1(int, h->GetDefaultFunc(), msg);
}
int __stdcall HooksInit(LoHook *h, HookContext* c)
{    
    _PI->WriteHiHook(0x5FFAC0, SPLICE_, EXTENDED_, THISCALL_, Dlg_SelectScenario_Dlg);

    //_PI->WriteHiHook(0x779213, CALL_, EXTENDED_, THISCALL_, Dlg_WoG_Options_Show);
    _PI->WriteHiHook(0x579CE0, SPLICE_, EXTENDED_, THISCALL_, NewScenarioDlg_Create);
    ReadModList();
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static _bool_ plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;
            Era::ConnectEra();
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance((char*)"era_menu.daemon.plugin");
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

