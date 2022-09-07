// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "header.h"
#include "battle_Dlg_MonPreview.cpp"
#include "battle_Dlg_SpellsPreview.cpp"

using namespace h3;
_LHF_(Dlg_CreatureInfo_RmcProc);
_LHF_(Dlg_CreatureInfo_Battle_BeforeCreate);


_LHF_(HooksInit)
{

   // path.Append("\\new_battle_interface_text.ini");
    //H3Ini* myIni = new H3Ini;

    //_PI->WriteLoHook(0x462E2B, Battle_Dlg_Create);
    if (std::atoi(Era::tr("gem_plugin.combat_dlg.enable.popup")))
    {
        _PI->WriteLoHook(0x46D6CB, Battle_Dlg_Create);
        _PI->WriteLoHook(0x46D97A, Battle_Dlg_StackInfo_Show);
        CreateResources();
        _PI->WriteDword(0x47205A + 1, 288 + DLG_HEIGHT_ADD); // left dlg height ++
        _PI->WriteDword(0x472092 + 1, 288 + DLG_HEIGHT_ADD); // right dlg height ++
        _PI->WriteDword(0x472061 + 1, 267 - DLG_HEIGHT_ADD); // left dlg x_pos ++
        _PI->WriteDword(0x472099 + 1, 267 - DLG_HEIGHT_ADD); // right dlg x_pos ++

    }
    if (std::atoi(Era::tr("gem_plugin.combat_dlg.enable.spells")))
    {
    _PI->WriteLoHook(0x5F4C5D, Dlg_CreatureInfo_RmcProc);
    _PI->WriteLoHook(0x46846A, Dlg_CreatureInfo_Battle_BeforeCreate);
    }

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
            Era::ConnectEra();
            _PI->WriteLoHook(0x4EEAF2, HooksInit);




        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

