// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include "..\oldHeroesHeaders\headers\era.h"
#include "..\oldHeroesHeaders\headers\HoMM3.h"
/*#include "..\heroes\headers\patcher_x86.hpp"
#include "..\heroes\headers\HoMM3_ids.h"
#include "..\heroes\headers\HoMM3_Base.h"
#include "..\heroes\headers\HoMM3_Res.h"
#include "..\heroes\headers\HoMM3_GUI.h"*/
#ifndef UNICODE
//#define UNICODE
#endif 
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
    const char* PLUGIN_VERSION = "2.47";

    const char* PLUGIN_NAME = "GemMainPLugin.era";
    const char* PLUGIN_AUTHOR = "daemon_n";
    const char* PLUGIN_DATA = "19.01.2022";
}



void Debug(int a=1)
{

    y[80] = a;
    ExecErmCmd("IF:L^%Y80^");
}


#define SpellInt_DEF (*(char**)0x5F6A3E)
//#define SPBID 3430

bool MMstrings[3] = { 0,0,0 };
float onlineVersion = 0;
float localVersion = 0;

char* GetEraJSON(const char* json_string_name) {
    return tr(json_string_name);
}
int __stdcall GameStart(LoHook* h, HookContext* c)
{
    h->Undo();

    if (std::atoi(GetEraJSON("gem_plugin.main_menu.current_version.display_version")) == 1)
    {
        MMstrings[0] = TRUE;
        if (std::atoi(GetEraJSON("gem_plugin.main_menu.current_version.read_registry")) == 1)
        {

            localVersion = getGameFromRegistry();

           // CALL_12(void, __fastcall, 0x4F6C00, (char*)std::to_string(localVersion).c_str(), 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);

        }

    }

    if (std::atoi(GetEraJSON("gem_plugin.main_menu.online_version.display_version")) == 1)
    {
        MMstrings[1] = TRUE;
        if (std::atoi(GetEraJSON("gem_plugin.main_menu.online_version.check_online")) == 1)
        {

            onlineVersion = checkOnlineVersion(GetEraJSON("gem_plugin.main_menu.online_version.remote_file"));
        }

    }

    if (std::atoi(GetEraJSON("gem_plugin.main_menu.era_version.display_version")) == 1)
    {
        MMstrings[2] = TRUE;
    }

    return EXEC_DEFAULT;
}
//int __stdcall Dlg_OnMouseMove(HiHook* hook, _Dlg_* dlg, int Xabs, int Yasb)

void Dlg_MainMenu_CreateText(_Dlg_* dlg, int x, int y, int length, string myText, int id)
{

    if (800 - x < length)
    {
        x = 800 - length;
    }
    if (600 - y < 16)
    {
        y = 600 - 16;
    }

    dlg->AddItem(_DlgStaticText_::Create(x, y, length, 16, (char*)myText.c_str(), (char*)"medfont2.fnt", 7, id, ALIGN_H_LEFT, 0));

}
void Dlg_MainMenu_Info(_Dlg_* dlg)
{


    if (MMstrings[0])
    {
        string jsonString = "gem_plugin.main_menu.current_version";

        string c_version;
        if (localVersion)
        {
            c_version = std::to_string(localVersion);
        }
        else
        {
            c_version = dllText::PLUGIN_VERSION;
        }

        if (std::atoi(GetEraJSON((jsonString + ".custom_version").c_str())))
        {
            c_version = GetEraJSON((jsonString + ".custom_version").c_str());

            while (c_version.length() < 4)
            {
                c_version += "0";

            }
        }

        int xPos = std::atoi(GetEraJSON((jsonString + ".x").c_str()));
        int yPos = std::atoi(GetEraJSON((jsonString + ".y").c_str()));
        int id = 550;
        //c_version = GetEraJSON("gem_plugin.version");
        int charLength = 7;

        std::string textLine = "ASSEMBLY v" + c_version.erase(4);

        std::string checkLine = GetEraJSON((jsonString + ".custom_text").c_str());
        if (checkLine.length())
        {
            textLine = checkLine;
        }
        int stringLength = std::atoi(GetEraJSON((jsonString + ".length_in_px").c_str()));

        if (stringLength <= 0)
        {
            stringLength = textLine.length() * charLength;
        }

        Dlg_MainMenu_CreateText(dlg, xPos, yPos, stringLength, textLine, id);
    }

    if (MMstrings[1])
    {
        string jsonString = "gem_plugin.main_menu.online_version";

        std::string textLine = "(latest v" + std::to_string(onlineVersion).erase(4) + ")";

        int xPos = std::atoi(GetEraJSON((jsonString + ".x").c_str()));
        int yPos = std::atoi(GetEraJSON((jsonString + ".y").c_str()));
        int id = 551;
        //c_version = GetEraJSON("gem_plugin.version");
        int charLength = 7;


        std::string checkLine = GetEraJSON((jsonString + ".custom_text").c_str());
        if (checkLine.length())
        {
            textLine = checkLine;
        }
        int stringLength = std::atoi(GetEraJSON((jsonString + ".length_in_px").c_str()));

        if (stringLength <= 0)
        {
            stringLength = textLine.length() * charLength;
        }

        Dlg_MainMenu_CreateText(dlg, xPos, yPos, stringLength, textLine, id);

    }

    _DlgItem_* wndText = dlg->GetItem(545);
    if (wndText)
    {
        wndText->Hide();
    }

    if (MMstrings[2])
    {
        string jsonString = "gem_plugin.main_menu.era_version";

        int xPos = std::atoi(GetEraJSON((jsonString + ".x").c_str()));
        int yPos = std::atoi(GetEraJSON((jsonString + ".y").c_str()));
        int id = 545;
        //c_version = GetEraJSON("gem_plugin.version");
        int charLength = 7;

        string textLine = "HoMM3 ERA " + (string)GetEraVersion();

        std::string checkLine = GetEraJSON((jsonString + ".custom_text").c_str());
        if (checkLine.length())
        {
            textLine = checkLine;
        }

        int stringLength = std::atoi(GetEraJSON((jsonString + ".length_in_px").c_str()));

        if (stringLength <= 0)
        {
            stringLength = textLine.length() * charLength;
        }

        Dlg_MainMenu_CreateText(dlg, xPos, yPos, stringLength, textLine, id);

    }


 }


int __stdcall gem_Dlg_MainMenu_Create(LoHook* hook, HookContext* c) //at the and of the Create function
{
    _Dlg_* dlg = (_Dlg_*)c->ecx;// -0x280); //edi - from IDA //changed to ecx cause i like ecx
    Dlg_MainMenu_Info(dlg);
    return EXEC_DEFAULT;
}



void HooksInit()
{   

    //Dlg's
 //   _GEM->WriteLoHook(0x4FBD71, gem_Dlg_MainMenu_Create);
  //  _GEM->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_NewGame);
  //  _GEM->WriteLoHook(0x4EF665, gem_Dlg_MainMenu_LoadGame);
 //   _GEM->WriteLoHook(0x4F0799, gem_Dlg_MainMenu_CampaignGame); //goes from new game
     _GEM->WriteLoHook(0x4FBD71, gem_Dlg_MainMenu_Create);
     _GEM->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_Create);
     _GEM->WriteLoHook(0x4EF665, gem_Dlg_MainMenu_Create);
     _GEM->WriteLoHook(0x4F0799, gem_Dlg_MainMenu_Create); //goes from new game






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
            _GEM = globalPatcher->CreateInstance((char*)"ERA.assembly.plugin");
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

