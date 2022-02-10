#include "pch.h"
#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_
#define _H3API_MESSAGES_

#include "..\h3api_single\H3API.hpp"
//#include "h3api\H3Dialogs\H3Messagebox.hpp"
#include "..\headers\era.h"    

#include <iostream>

Patcher* globalPatcher;
PatcherInstance* _GEM;
#define o_HD_X (*(INT*)0x401448)
#define o_HD_Y (*(INT*)0x40144F)
using namespace Era;
using namespace h3;

int __stdcall OnAdventureDlgCreate(LoHook* h, HookContext* c)
{
    //h3::ms
  //    _H3Dlg_* dlg = *(_Dlg_**)(c->ebp + 0x8);
   // h3::H3Dlg *dlg = *(H3Dlg**)(c->ebp + 0x8);
    h3::H3BaseDlg* dlg = (H3BaseDlg*)c->ebx;
    //windo  wnd = P_WindowMgr.operator*;
    //dlg.getcur;
    if (dlg)
    {   
 
        dlg->CreateDef(o_HD_X - 450, o_HD_Y - 25, 3233, "smalres.def", 0, 0, 0, 0);
        dlg->CreateText(o_HD_X - 430, o_HD_Y - 25, 250, 22, "2000", "smalfont.fnt", eTextColor::WHITE, 3234, eTextAlignment::HLEFT);
       // dlg->CreateText()
    }//dlg->CreateEdit(1,)
    return EXEC_DEFAULT;
}

int __stdcall OnCurDayBarDlgUpdate(LoHook* h, HookContext* c)
{
    H3BaseDlg* dlg;
    
    h3:H3AdventureManager* adv = P_AdventureManager;
        dlg=  adv->dlg;
       // if (y[1])
        {
            //Era::ExecErmCmd("UN:Cy1/84/4/?y2");
           // dlg = (H3BaseDlg*)y[2];
           // dlg = (H3BaseDlg*)c->ecx;

            //  if (dlg->GetItem(540))
            if (dlg )
            {

              //  Era::ExecErmCmd("IF:L^^");

               // if (dlg->GetH3DlgItem(3234))
                {

                    H3DlgItem* it = dlg->GetH3DlgItem(3234);
                    h3::H3Player* player;
                    ///player[1]
                    //P_ActivePlayer;
                    int iam = 2;// PH3PlayersInfo;
                  //  it->SetX(it->GetX() - 13);
                    Era::y[1] = iam;
                    Era::ExecErmCmd("IF:L^%y1^");

                    Era::ExecErmCmd("OW:Cd/?y1 Ry1/7/?y2");
                    //(H3DlgText*) it;
                    int mithril = Era::y[2];
                    it->Cast<H3DlgText>()->SetText((std::to_string(mithril)).c_str());

                  //  h3vargs::Stdcall_Variadic<void>(0x57CA50, it, "123");
                   // (reinterpret_cast<void(__stdcall*)>(0x57CA50), it, "123");
                 //   dlg->GetH3DlgItem(3234)->Refresh();


                    //(it)( H3DlgText::SetText("22"));
                   // reinterpret_cast<H3DlgText*> (it)->SetText( "123" );

                   // it->ParentRedraw();

                   // it->SetText(("123" + std::to_string(mithril)).c_str());

                   // it->Refresh();
                }
            }
        }
    return EXEC_DEFAULT;
}
int __stdcall OnResourceBarDlgUpdate(LoHook* h, HookContext* c)
{
    H3BaseDlg* dlg = (H3BaseDlg*)c->ecx;
    if (dlg )
    {        
       //  dlg->GetH3DlgItem(3233)->Refresh();
       //  dlg->GetH3DlgItem(3234)->Refresh();
    }
    return EXEC_DEFAULT;
}

void HooksInit()
{
    _GEM->WriteLoHook(0x4021AF, OnAdventureDlgCreate);
  //  _GEM->WriteLoHook(0x403F15, OnResourceBarDlgUpdate);
    _GEM->WriteLoHook(0x417428, OnCurDayBarDlgUpdate);

    
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
          //  RegisterHandler(OnGameEnter, "OnGameEnter");
          //  RegisterHandler(OnBattleScreenMouseClick, "OnBattleScreenMouseClick");

            

            globalPatcher = GetPatcher();
            _GEM = globalPatcher->CreateInstance("ERA.daemon.plugin");
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