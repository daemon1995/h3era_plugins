#include "pch.h"
#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_
#define _H3API_MESSAGES_
#define _H3API_EXCEPTION_

#include ".\h3api_single\H3API.hpp"
#include ".\headers\era.h"    

#include <iostream>
int patchesss = 0;
Patcher* globalPatcher;
PatcherInstance* _PI;

#define o_HD_X (*(INT*)0x401448)
#define o_HD_Y (*(INT*)0x40144F)
#define MITHRIL_DEF_ID 3233
#define MITHRIL_TEXT_ID 3234
#define MITHRIL_FRAME_ID 3235
//bool isHdMod = Era::TPluginExists((char)*"hd_wog.dll");

using namespace Era;
using namespace h3;

int __stdcall OnAdventureDlgCreate(LoHook* h, HookContext* c)
{

    if (o_HD_X > 810)
    {
        h3::H3ResourceBarPanel* dlg = (H3ResourceBarPanel*)c->eax;
        if (dlg)
        {
            H3SEHandler seh;
            try
            {

                H3DlgItem* mithrilDef = H3DlgDef::Create(600, 3, 20, 10, MITHRIL_DEF_ID, "SMALmith.def");
                //  CALL_1(_int_, __thiscall, 0x4CE6E0, this)
                 // P_gamem
                  //Era::y[1] = THISCALL_1(INT, 0x4CE6E0, *(INT*)0x699538);
                int saveY = Era::y[1];
                Era::y[1] = P_Game->GetPlayerID();
                Era::ExecErmCmd("OW:Ry1/7/?y1");

                H3DlgItem* mithriText = H3DlgText::Create(627, 3, 78, 22, std::to_string(Era::y[1]).c_str(), "smalfont.fnt", eTextColor::WHITE, MITHRIL_TEXT_ID, eTextAlignment::HLEFT);
                H3DlgItem* mithrilBack = H3DlgFrame::Create(600, 3, 82, 16, MITHRIL_FRAME_ID, 0xF711);
                dlg->AddItem(mithrilBack);   mithrilBack->Show();
                dlg->AddItem(mithrilDef);   mithrilDef->Show();
                dlg->AddItem(mithriText); 
                mithriText->Show();
                Era::y[1] = saveY;
                //*some stuff that can create an error...
                   // *
            }
            catch (const H3Exception & e)
                 {
                e.ShowInGame();
                Era::ExecErmCmd("IF:M^Disable \"NewAdvDlgItems.era\" plugin from Game Enhancement Mod\\eraplugins \
                    and report about that to daemon_n in Discord ^");

               //*
            }
            catch (const std::exception & e)
                 {
              //  H3Exception::LogError(e, custom_file_name);
           //     
            }
            



        }
    }
    return EXEC_DEFAULT;
}

int __stdcall OnResourceBarDlgUpdate(LoHook* h, HookContext* c)
{
   // P_AdventureManager->dlg->CreateBlackBox(10, 10, 25, 12);

    if (o_HD_X > 810)
    {
        H3BaseDlg* dlg = (H3BaseDlg*)c->ecx;

        if (dlg)
        {
            H3DlgText* myText = dlg->GetText(MITHRIL_TEXT_ID);
            if (myText)
            {
               // Era::y[1] = THISCALL_1(INT, 0x4CE6E0, *(INT*)0x699538);
                int saveY = Era::y[1];
                Era::y[1] = P_Game->GetPlayerID();
                Era::ExecErmCmd("OW:Ry1/7/?y1");

                myText->SetText(std::to_string(Era::y[1]).c_str());
                Era::y[1] = saveY;
            }
        }
    }

    return EXEC_DEFAULT;
}


void HooksInit()

{  
   
  //  _GEM->WriteLoHook(0x4A99C0, OnHeroPickupRes);
    _PI->WriteLoHook(0x4021B2, OnAdventureDlgCreate);
    _PI->WriteLoHook(0x403F00, OnResourceBarDlgUpdate);
    _PI->WriteLoHook(0x417380, OnResourceBarDlgUpdate);


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

            

            globalPatcher = GetPatcher();
            //004A481C
            
            _PI = globalPatcher->CreateInstance("ERA.NewAdvMapItems");
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