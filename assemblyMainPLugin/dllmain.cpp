// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include "..\oldHeroesHeaders\headers\era.h"
#include "..\oldHeroesHeaders\headers\HoMM3.h"
/*#include "..\heroes\headers\patcher_x86.hpp"
#include "..\heroes\headers\HoMM3_ids.h"
#include "..\heroes\headers\HoMM3_Base.h"
#include "..\heroes\headers\HoMM3_Res.h"
#include "..\heroes\headers\HoMM3_GUI.h"*/

#include <string>
#include <iostream>
#include "webFunctions.h"
#include "localFunctions.h"


using namespace Era;

const int ADV_MAP = 37;
const int CTRL_LMB = 4;
const int LMB_PUSH = 12;

Patcher* globalPatcher;
PatcherInstance* _GEM;

namespace dllText
{
    const char* PLUGIN_VERSION = "2.45";

    const char* PLUGIN_NAME = "GemMainPLugin.era";
    const char* PLUGIN_AUTHOR = "daemon_n";
    const char* PLUGIN_DATA = "31.12.2021";
}



void Debug(int a)
{

    y[80] = a;
    ExecErmCmd("IF:L^%Y80^");
}




#define SpellInt_DEF (*(char**)0x5F6A3E)
//#define SPBID 3430


float onlineVersion = 0;
float localVersion = 0;

char* GetEraJSON(const char* json_string_name) {
    return tr(json_string_name);
}
int __stdcall GameStart(LoHook* h, HookContext* c)
{
    h->Undo();

    if (std::atoi(GetEraJSON("gem_plugin.main_menu.display_version")) == 1)
    {

        if (std::atoi(GetEraJSON("gem_plugin.main_menu.read_registry")) == 1)
        {

            localVersion = getGameFromRegistry();
        }

        if (std::atoi(GetEraJSON("gem_plugin.main_menu.check_online"))== 1 )
        {
           // LPCWSTR siteForCheckingVersion = A2W_EX(fileLink.c_str(), fileLink.length());
           // DeleteUrlCacheEntry(GetEraJSON("gem_plugin.main_menu.remote_file"));

            onlineVersion = checkOnlineVersion(GetEraJSON("gem_plugin.main_menu.remote_file"));

        }
    }
    //  CALL_12(void, __fastcall, 0x4f6c00, "{~>resource.def:0}", 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
 // }

    return EXEC_DEFAULT;
}
//int __stdcall Dlg_OnMouseMove(HiHook* hook, _Dlg_* dlg, int Xabs, int Yasb)

void Dlg_MainMenu_Info(_Dlg_* dlg)
{
    if (std::atoi(GetEraJSON("gem_plugin.main_menu.display_version")) == 1)
    {
        string c_version;
        if (localVersion)
        {
            c_version = std::to_string(localVersion);

        }
        else
        {
            c_version = dllText::PLUGIN_VERSION;

        }
        if (std::atoi(GetEraJSON("gem_plugin.main_menu.custom_version")))
        {
            c_version = GetEraJSON("gem_plugin.main_menu.custom_version");
            //int length = c_version.length();

            while (c_version.length() < 4)
            {
                c_version += "0";

            }

        }

        //c_version = GetEraJSON("gem_plugin.version");
        sprintf(o_TextBuffer, "ASSEMBLY v%s", c_version.erase(4).c_str());
        dlg->AddItem(_DlgStaticText_::Create(580, 535, 200, 55, o_TextBuffer, (char*)"medfont2.fnt", 7, 550, ALIGN_H_LEFT|ALIGN_V_BOTTOM, 0));

        if (onlineVersion)
        {
            string c_version = std::to_string(onlineVersion);
            sprintf(o_TextBuffer, "(latest v%s)", c_version.erase(4).c_str());
            dlg->AddItem(_DlgStaticText_::Create(580, 547, 200, 55, o_TextBuffer, (char*)"medfont2.fnt", 7, 551, ALIGN_H_LEFT|ALIGN_V_BOTTOM, 0));

        }
    }

}
int __stdcall gem_Dlg_MainMenu_Create(LoHook* hook, HookContext* c) //at the and of the Create function
{
    _Dlg_* dlg = (_Dlg_*)c->edi; //edi - from IDA
    Dlg_MainMenu_Info(dlg);
    return EXEC_DEFAULT;
}

int __stdcall gem_Dlg_MainMenu_NewGame(LoHook* hook, HookContext* c)// before 
{
    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_MainMenu_Info(dlg);
    return EXEC_DEFAULT;
}

int __stdcall gem_Dlg_MainMenu_LoadGame(LoHook* hook, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_MainMenu_Info(dlg);
    return EXEC_DEFAULT;
}
int __stdcall gem_Dlg_MainMenu_CampaignGame(LoHook* hook, HookContext* c)
{
    _Dlg_ * dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_MainMenu_Info(dlg);
    return EXEC_DEFAULT;
}


void HooksInit()
{   

    //Dlg's
    _GEM->WriteLoHook(0x4FBCA4, gem_Dlg_MainMenu_Create);
    _GEM->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_NewGame);
    _GEM->WriteLoHook(0x4EF665, gem_Dlg_MainMenu_LoadGame);
    _GEM->WriteLoHook(0x4F0799, gem_Dlg_MainMenu_CampaignGame); //goes from new game

    //  _GEM->WriteHiHook(0x5FFCA0, SPLICE_, EXTENDED_, THISCALL_, Dlg_OnMouseMove);

   // _GEM->WriteLoHook(0x75AE24, gem_StartBattle);

    //_GEM->WriteLoHook(0x471FF9, NewIntBttnPos_BattleDlg_Create);
    //_GEM->WriteLoHook(0x5E7F76, OnOpenMarketWindowDlgLow);
   
    _GEM->WriteLoHook(0x597870, GameStart);
    return;

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

        //if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;
            ConnectEra();
            //RegisterHandler(OnBattleReplay, "OnBattleReplay");

            globalPatcher = GetPatcher();
            _GEM = globalPatcher->CreateInstance("ERA.assembly.plugin");
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

