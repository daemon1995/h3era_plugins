// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
using namespace h3;

Patcher* globalPatcher;
PatcherInstance* _PI;

std::vector<H3String> modList;

int  __stdcall Dlg_SelectScenario_Dlg(HiHook* h, H3Msg* msg)
{
    if (msg->itemId == 4500 && msg->subtype == eMsgSubtype::LBUTTON_CLICK)
    {

        EraMenuDlg::Create();

    }

    return THISCALL_1(int, h->GetDefaultFunc(), msg);
}

_LHF_(DlgMainMenu_BeforeRun)
{
    EraMenuDlg::Create();

    return EXEC_DEFAULT;
}

void __stdcall NewScenarioDlg_Create(HiHook* hook, H3SelectScenarioDialog* dlg, H3Msg* msg)
{
    THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);

    H3DlgCaptionButton* bttn = dlg->GetCaptionButton(4444);
    if (bttn)
    {
        bttn->AddHotkey(h3::eVKey::H3VK_W);


    }
    bttn = dlg->CreateCaptionButton(bttn->GetX(), bttn->GetY() + 45, bttn->GetWidth(), bttn->GetHeight(), 4500, bttn->GetDef()->GetName(), "ERA options", h3::NH3Dlg::Text::SMALL, 0);
    if (bttn)
    {
        bttn->SetClickFrame(1);
        bttn->AddHotkey(h3::eVKey::H3VK_E);
    }
}
_LHF_(SelectScenarioDlg_Campaign_BeforeRun)
{
    auto dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);
    if (dlg)
    {
        H3DlgCaptionButton* bttn = dlg->CreateCaptionButton(600, 199 + 45, 24, 14, 4500, "Iokay.def", "ERA options", h3::NH3Dlg::Text::SMALL, 0);
        if (bttn)
        {
            bttn->SetClickFrame(1);
            bttn->AddHotkey(h3::eVKey::H3VK_E);
        }

    }
    return EXEC_DEFAULT;
}



_LHF_(HooksInit)
{

   // auto modList = new std::vector<std::string>();
    ModListReader::Read(modList);
    _PI->WriteHiHook(0x5FFAC0, THISCALL_, Dlg_SelectScenario_Dlg);

    //    _PI->WriteHiHook(0x779213, CALL_, EXTENDED_, THISCALL_, Dlg_WoG_Options_Show);
    _PI->WriteHiHook(0x579CE0, SPLICE_, EXTENDED_, THISCALL_, NewScenarioDlg_Create);
    _PI->WriteLoHook(0x45AE90, SelectScenarioDlg_Campaign_BeforeRun); // load campaign

    _PI->WriteLoHook(0x4EF259, DlgMainMenu_BeforeRun); //MAIN Main Menu Dlg Run

    //_PI->WriteLoHook(0x4F0900, SelectScenarioDlg_Campaign_BeforeRun); // campaign from MM

    

   // _PI->WriteHiHook(0x513993, SPLICE_, EXTENDED_, THISCALL_, NewScenarioDlg_Create);

    
    return EXEC_DEFAULT;
}


void __stdcall Dlg_WoG_Options_Show(HiHook* hook, int a1);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static bool pluginIsLoaded = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsLoaded)
        {
            pluginIsLoaded = true;

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("ERA.OptionsMenu.Plugin");
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

