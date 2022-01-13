// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include "..\oldHeroesHeaders\headers\era.h"
#include "..\oldHeroesHeaders\headers\HoMM3.h"

#include <iostream>


using namespace Era;
// функция получения JSON строк методом ERA

char* GetEraJSON(const char* json_string_name) {
    return tr(json_string_name);
}

Patcher* _P;
PatcherInstance* _PI;

namespace dllText
{
    const char* PLUGIN_NAME = "New Main Menu Items";
    const char* PLUGIN_AUTHOR = "daemon_n";
    const char* PLUGIN_DATA = "30.12.2021";
}

int lobbyMenuCounter = 0;

const int logoId = 666;
void Dlg_NewLogo_Create(_Dlg_* dlg)
{
    const char* pcxName = "hmm3logo.pcx";


    sprintf(o_TextBuffer, pcxName);
    int xPos = atoi(GetEraJSON("nmmi.pos.xPos"));
    int yPos = atoi(GetEraJSON("nmmi.pos.yPos"));
    
    dlg->AddItem(_DlgStaticPcx8_::Create(xPos, yPos, logoId, o_TextBuffer)); // add pcx8
 
}
int __stdcall gem_Dlg_MainMenu_Create(LoHook* hook, HookContext* c) //at the and of the Create function
{
    _Dlg_* dlg = (_Dlg_*)c->edi; //edi - from IDA
    Dlg_NewLogo_Create(dlg);
    return EXEC_DEFAULT;
}

int __stdcall gem_Dlg_MainMenu_NewGame(LoHook* hook, HookContext* c)// before 
{
    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_NewLogo_Create(dlg);
    return EXEC_DEFAULT;
}

int __stdcall gem_Dlg_MainMenu_LoadGame(LoHook* hook, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_NewLogo_Create(dlg);
    lobbyMenuCounter = 1;

    return EXEC_DEFAULT;
}

int __stdcall gem_Dlg_MainMenu_CampaignGame(LoHook* hook, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_NewLogo_Create(dlg);
    return EXEC_DEFAULT;
}

int __stdcall gem_Dlg_LobbyMenu_NewGame(LoHook* hook, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    Dlg_NewLogo_Create(dlg);
    lobbyMenuCounter = 0;

    return EXEC_DEFAULT;
}
int __stdcall gem_Dlg_LobbyMenu_ShowAvailableScenarios(LoHook* hook, HookContext* c)
{
   // Era::ExecErmCmd("IF:L^^");

    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    _DlgItem_* logo = dlg->GetItem(logoId);



    if (lobbyMenuCounter == 1)
    {
        if (logo) { logo->Show(); }
        else
        {
            Dlg_NewLogo_Create(dlg);
            logo = dlg->GetItem(logoId);
            logo->Hide();
            return EXEC_DEFAULT;

        }
        lobbyMenuCounter = 0;
    }
    else
    {
        if (logo) { logo->Hide(); }
        else
        {
            Dlg_NewLogo_Create(dlg);
            logo = dlg->GetItem(logoId);
            logo->Hide();
        }
        lobbyMenuCounter = 1;
    }

    return EXEC_DEFAULT;
}



int __stdcall gem_Dlg_LobbyMenu_ShowRandomMap(LoHook* hook, HookContext* c) //call RMG dlg,
{
  //  Era::ExecErmCmd("IF:L^^");

    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    _DlgItem_* logo = dlg->GetItem(logoId);
    

    if (lobbyMenuCounter == 2)
    {
        if (logo) { logo->Show(); }
        lobbyMenuCounter = 0;
    }
    else
    {
        if (logo) { logo->Hide(); }

        lobbyMenuCounter = 2;
    }

    return EXEC_DEFAULT;
}
int __stdcall gem_Dlg_LobbyMenu_ShowAdvancedOptions(LoHook* hook, HookContext* c)
{
    //Era::ExecErmCmd("IF:L^^");

    _Dlg_* dlg = (_Dlg_*)c->ecx; //ecx  because it's a class method call - by Strigo
    _DlgItem_* logo = dlg->GetItem(logoId);

    if (!logo)
    {
        return EXEC_DEFAULT;
    }

    if (lobbyMenuCounter == 3)
    {
        if (logo) { logo->Show(); }
        lobbyMenuCounter = 0;
    }
    else
    {
        if (logo) { logo->Hide(); }
        lobbyMenuCounter = 3;
    }

    return EXEC_DEFAULT;
}



void HooksInit()
{   
    //Dlg's
    _PI->WriteLoHook(0x4FBCA4, gem_Dlg_MainMenu_Create);
    _PI->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_NewGame);
    _PI->WriteLoHook(0x4EF665, gem_Dlg_MainMenu_LoadGame);
    _PI->WriteLoHook(0x4F0799, gem_Dlg_MainMenu_CampaignGame); //goes from new game

    _PI->WriteLoHook(0x4F0B63, gem_Dlg_LobbyMenu_NewGame); //goes from new game
    _PI->WriteLoHook(0x580180, gem_Dlg_LobbyMenu_ShowAdvancedOptions); //goes from new game, tnx too RK
    _PI->WriteLoHook(0x5813D0, gem_Dlg_LobbyMenu_ShowRandomMap); //goes from new game, tnx too RK
    _PI->WriteLoHook(0x580D40, gem_Dlg_LobbyMenu_ShowAvailableScenarios); //goes from new game, tnx too RK

   //_PI->WriteLoHook(0x57A29B, gem_Dlg_LobbyMenu_LoadGame); //goes from new game

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

            _P = GetPatcher();
            _PI = _P->CreateInstance(dllText::PLUGIN_NAME);
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

