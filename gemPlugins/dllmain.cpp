// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include ".\headers\era.h"
#include ".\headers\HOMM3.h"

Patcher* _P;
PatcherInstance* _PI;
using namespace Era;


int __stdcall OnSwapMgrProc(HiHook* hook, _SwapMgr_ *swapMgr, _EventMsg_* msg)
{
 //   _C* mMouse = o_MouseMgr;
   
    
   // if ()
    {

    }
    
    _Dlg_* dlg = o_SwapMgr->dlg;
    dlg->x;
    int currentPlayer = o_ActivePlayer->id;

    y[55] = currentPlayer;
  //  if (msg.)
    {

    }
    y[56] = (*(int*) 0x5FFCA0);
    sprintf(o_TextBuffer, "%%0d is %0d, %%d is %d", 10000, 1);
    b_MsgBox(o_TextBuffer, 1);
    //Era::ExecErmCmd("IF:L^%y55 %y56^");
   // ret = CALL_2(void)
    return CALL_2(int, __thiscall, hook->GetDefaultFunc(), swapMgr, msg);
    
}

void HooksInit()

{

    //  _GEM->WriteLoHook(0x4A99C0, OnHeroPickupRes);
    //_PI->WriteLoHook(0x5B0100, OnAdventureDlgCreate);
   // _PI->WriteLoHook(0x403F00, OnResourceBarDlgUpdate);
  //  _PI->WriteLoHook(0x417380, OnResourceBarDlgUpdate);
    _PI->WriteHiHook(0x5B0100, SPLICE_, EXTENDED_, THISCALL_, OnSwapMgrProc);

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
            ConnectEra();
            //RegisterHandler(OnBattleReplay, "OnBattleReplay");

            _P = GetPatcher();
            _PI = _P->CreateInstance((char*)"ERA.NewMainMenuItems");
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

