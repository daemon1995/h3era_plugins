// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "header.h"
using namespace h3;
Patcher* globalPatcher;
PatcherInstance* _PI;

_LHF_(Dlg_LobbyMenu_NewGame)
{
    H3Dlg* dlg = (H3Dlg*)c->ecx;
    if (dlg)
       dlg->GetH3DlgItem(130)->Hide();

    return EXEC_DEFAULT;
}
_LHF_(OnAfterMarketDlgArtifactPlace)
{
    H3Dlg* dlg = (H3Dlg*)c->edi;

    if (dlg)
    {
        H3DlgItem* it;
        for (int i = 63; i < 69; i++)
        {
            it= dlg->GetH3DlgItem(i);
            if (it)
            {
                it->Disable();
                dlg->CreatePcx(it->GetX(), it->GetY(), it->GetWidth(), it->GetWidth(), i-44, "DiBoxBck.pcx");
            }

        }
    }
    return EXEC_DEFAULT;
}

void HooksInit()
{

    _PI->WriteLoHook(0x4F0B63, Dlg_LobbyMenu_NewGame);
   // _PI->WriteByte(0x57C10C+2, 0); // skip building "Random Map Button
    _PI->WriteLoHook(0x5E7FC2, OnAfterMarketDlgArtifactPlace);
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

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("for_Helgtla.daemon.plugin");

            HooksInit();


        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

