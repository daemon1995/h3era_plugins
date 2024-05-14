// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
//#include "anotherFiles.cpp"


Patcher* globalPatcher;
PatcherInstance* _PI;


_LHF_(arch_OnCombatCreaturePanel_Create)
{

   // !!IF:M^^;
    //H3DlgPcx16* dlgPcx = H3DlgPcx16::Create(12, 12, "skslt.PCX");
    //if (dlgPcx)
    //{

    //    dlgPcx->SetWidth(12);
    //    dlgPcx->SetHeight(12);
    //    //H3CombatMonsterPanel* dlgPanel = *reinterpret_cast<H3CombatMonsterPanel**>(c->ebp - 0x10);
    //    //dlgPanel->AddItem(dl)


    //    H3Vector<H3DlgItem*>* items = reinterpret_cast<H3Vector<H3DlgItem*>*>(c->esi);
    //    if (items)
    //    {
    //        items->Add(dlgPcx);

    //    }

    //}

    auto dlgg = P_CombatManager->dlg;
    if (dlgg)
    {

        if (auto item = dlgg->GetH3DlgItem(2010))
        {
            item->Hide();
        }
    }


    return EXEC_DEFAULT;

    H3Dlg dlg(H3GameWidth::Get(),H3GameHeight::Get());

    for (size_t i = 0; i < 200; i++)
    {
        for (size_t k = 0; k < 200; k++)
        {
            H3DlgDefButton* b = H3DlgDefButton::Create(i, k, int(eControlId::OK), NH3Dlg::Assets::OKAY_DEF, 0, 1, FALSE, 0);
            dlg.AddItem(b);
        }
    }

    H3DlgDefButton* bttn =  dlg.CreateOKButton();
    bttn->AddHotkey(NH3VKey::H3VK_ESCAPE);

    dlg.Start();

    return EXEC_DEFAULT;
}

extern "C" __declspec(dllexport) int Foo(int  arg)
{
    if (arg)
    {
        _PI->ApplyAll();
        H3Messagebox::RMB("Applied");
    }
    else
    {
        _PI->UndoAll();

        H3Messagebox::RMB("undo done");

    }
    return 1;
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    static bool pluginIsOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        if (!pluginIsOn)
        {
            pluginIsOn = true;

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("test.plugin.totest");
            //Era::ConnectEra();
         //  auto p = _PI->WriteLoHook(0x4629DA, arch_OnCombatCreaturePanel_Create);
           



        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

