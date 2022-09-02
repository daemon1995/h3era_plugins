// dllmain.cpp : Определяет точку входа для приложения DLL.
//#include "header.h"
#include "battle_Dlg_MonPreview.cpp"
using namespace h3;

//_LHF_(Dlg_LobbyMenu_NewGame)
//{
//    H3Dlg* dlg = (H3Dlg*)c->ecx;
//    if (dlg)
//    {
//        H3DlgItem* it = dlg->GetH3DlgItem(128);
//        if (it)
//            it->Hide();
//
//        it = dlg->GetH3DlgItem(130);
//        if (it)
//            it->Hide();
//
//        it = dlg->GetH3DlgItem(4444);
//        if (it)
//            it->Hide();
//
//
//    }
//
//    return EXEC_DEFAULT;
//}
//_LHF_(OnAfterMarketDlgArtifactPlace)
//{
//    H3Dlg* dlg = (H3Dlg*)c->edi;
//
//    if (dlg)
//    {
//        H3DlgItem* it;
//        for (int i = 63; i < 69; i++)
//        {
//            it= dlg->GetH3DlgItem(i);
//            if (it)
//            {
//                it->Disable();
//              //  dlg->CreatePcx(it->GetX(), it->GetY(), it->GetWidth(), it->GetWidth(), i-44, "DiBoxBck.pcx");
//                
//            }
//
//        }
//       // int x = std::atoi(Era::tr("QuestName.ResQu06.x"));// != "QuestName.ResQu06.x" ? std::atoi(Era::tr("QuestName.ResQu06.x")) : dlg->GetH3DlgItem(63)->GetX();
//       // int y = std::atoi(Era::tr("QuestName.ResQu06.y"));// != "QuestName.ResQu06.y" ? std::atoi(Era::tr("QuestName.ResQu06.y")) : dlg->GetH3DlgItem(63)->GetY();
//      //  const char* pcxName = Era::tr("QuestName.ResQu06.name");// != "QuestName.ResQu06.name\0" ? Era::tr("QuestName.ResQu06.name") : "DiBoxBck.pcx";
//        //Era::PcxPngExists()
//      //  h3::H3Messagebox(Era::tr("QuestName.ResQu06.name"));
//        dlg->CreatePcx(dlg->GetH3DlgItem(63)->GetX(), dlg->GetH3DlgItem(63)->GetY(), 505, "TRMArt.pcx");
//
//    }
//    return EXEC_DEFAULT;
//}

_LHF_(HooksInit)
{
    //_PI->WriteLoHook(0x462E2B, Battle_Dlg_Create);
    _PI->WriteLoHook(0x46D6CB, Battle_Dlg_Create);
    _PI->WriteLoHook(0x46D97A, Battle_Dlg_StackInfo_Show);
   // _PI->WriteLoHook(0x46DAD0, Battle_Dlg_StackInfo_Dtor);
   // _PI->WriteLoHook(0x46FE20, Battle_Dlg_Dtor);
    CreateResources();
    Era::ConnectEra();
    //_PI->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_NewGame);
   // Hel_CreateNewDef();
    _PI->WriteDword(0x47205A + 1, 288 + DLG_HEIGHT_ADD); // left dlg height ++
    _PI->WriteDword(0x472092 + 1, 288 + DLG_HEIGHT_ADD); // right dlg height ++
    _PI->WriteDword(0x472061 + 1, 267 - DLG_HEIGHT_ADD); // left dlg x_pos ++
    _PI->WriteDword(0x472099 + 1, 267 - DLG_HEIGHT_ADD); // right dlg x_pos ++
       
    //_PI->WriteLoHook(0x46D6CB, Dlg_LobbyMenu_NewGame);
   // _PI->WriteByte(0x57C10C+2, 0); // skip building "Random Map Button
   // _PI->WriteLoHook(0x5E7FC2, OnAfterMarketDlgArtifactPlace);
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


        //if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("battle_dlg.daemon.plugin");
            _PI->WriteLoHook(0x4EEAF2, HooksInit);


        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

