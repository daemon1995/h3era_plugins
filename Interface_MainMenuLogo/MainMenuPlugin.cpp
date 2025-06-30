#include "pch.h"

MainMenuPlugin::MainMenuPlugin(PatcherInstance *pi) : IGamePatch(pi)
{
    mainMenu.LoadInfo("main");
    selectScenario.LoadInfo("select_scenario");
    CreatePatches();
}

H3DlgPcx *H3BaseDlg__CreateLogo(H3BaseDlg *dlg, const MainMenuPlugin::MenuInformation &info)
{
    auto logo = dlg->CreatePcx(info.x, info.y, info.dlgItemId, info.pcxName);
    if (logo)
        logo->DeActivate();
    return logo;
}

void MainMenuPlugin::H3SelectScenarioDlg__HandleLogo(H3BaseDlg *dlg, const int loopCounter)
{
    auto *logo = dlg->GetPcx(selectScenario.dlgItemId);

    if (lobbyMenuCounter == loopCounter)
    {
        if (logo)
        {
            logo->Show();
            lobbyMenuCounter = 0;
        }
        else
        {
            if (logo = H3BaseDlg__CreateLogo(dlg, selectScenario))
                logo->Hide();
        }
    }
    else
    {
        if (logo)
            logo->Hide();
        else
        {
            if (logo = H3BaseDlg__CreateLogo(dlg, selectScenario))
                logo->Hide();
        }
        lobbyMenuCounter = loopCounter;
    }
}

_LHF_(MainMenuPlugin::SelectScenarioDlg_NewGame)
{
    if (auto dlg = reinterpret_cast<H3BaseDlg *>(c->ecx))
        H3BaseDlg__CreateLogo(dlg, Get().selectScenario);
    Get().lobbyMenuCounter = 0;

    return EXEC_DEFAULT;
}
_LHF_(MainMenuPlugin::SelectScenarioDlg_ShowAvailableScenarios)
{
    Get().H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg *>(c->ecx), 1);
    return EXEC_DEFAULT;
}

_LHF_(MainMenuPlugin::SelectScenarioDlg_ShowRandomMap) // call RMG dlg,
{
    Get().H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg *>(c->ecx), 2);
    return EXEC_DEFAULT;
}
_LHF_(MainMenuPlugin::SelectScenarioDlg_ShowAdvancedOptions)
{
    Get().H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg *>(c->ecx), 3);
    return EXEC_DEFAULT;
}

inline int __stdcall MainMenuPlugin::DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg)
{
    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    H3BaseDlg__CreateLogo(dlg, Get().mainMenu);
    return result;
}
int __stdcall MainMenuPlugin::DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val)
{
    int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, val);
    H3BaseDlg__CreateLogo(dlg, Get().mainMenu);

    return result;
}

int __stdcall MainMenuPlugin::DlgMainMenu_Campaign_Run(HiHook *h, H3BaseDlg *dlg)
{
    auto &mm = Get().mainMenu;
    H3BaseDlg__CreateLogo(dlg, mm);
    auto *_h = mm.alwaysDraw ? _PI->WriteHiHook(0x5FFAC0, THISCALL_, DlgMainMenu_Proc) : nullptr;

    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    if (_h)
        _h->Destroy();
    return result;
}

int __stdcall MainMenuPlugin::DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{

    if (auto logo = msg->GetDlg()->GetPcx(Get().mainMenu.dlgItemId))
    {
        logo->Draw();
    }

    int res = FASTCALL_1(int, h->GetDefaultFunc(), msg);

    return res;
}

void MainMenuPlugin::CreatePatches()
{
    if (!m_isInited)
    {
        if (mainMenu.enable)
        {
            _PI->WriteHiHook(0x4FB930, THISCALL_, DlgMainMenu_Create);
            _PI->WriteHiHook(0x4D56D0, THISCALL_, DlgMainMenu_NewLoad_Create);
            _PI->WriteHiHook(0x4F0799, THISCALL_, DlgMainMenu_Campaign_Run); // goes from new game
            if (mainMenu.alwaysDraw)
            {
                _PI->WriteHiHook(0x4D5B50, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
                _PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
            }
        }
        if (selectScenario.enable)
        {
            _PI->WriteLoHook(0x4F0B6A, SelectScenarioDlg_NewGame);
            _PI->WriteLoHook(0x580180, SelectScenarioDlg_ShowAdvancedOptions);
            _PI->WriteLoHook(0x5813D0, SelectScenarioDlg_ShowRandomMap);
            _PI->WriteLoHook(0x580D40, SelectScenarioDlg_ShowAvailableScenarios);
        }

        m_isInited = true;
    }
}

MainMenuPlugin &MainMenuPlugin::Get()
{
    static MainMenuPlugin instance(_PI);
    return instance;
    // TODO: insert return statement here
}

void MainMenuPlugin::MenuInformation::LoadInfo(const char *jsonSubKey)
{
    bool readSuccess = false;
    std::string temp = EraJS::read(H3String::Format("nmmi.menu_info.%s.pcx_name", jsonSubKey).String(), readSuccess);
    if (readSuccess && !temp.empty())
    {
        pcxName = EraJS::read(H3String::Format("nmmi.menu_info.%s.pcx_name", jsonSubKey).String(), readSuccess);
        enable = EraJS::readInt(H3String::Format("nmmi.menu_info.%s.enable", jsonSubKey).String());
        x = EraJS::readInt(H3String::Format("nmmi.menu_info.%s.x", jsonSubKey).String());
        y = EraJS::readInt(H3String::Format("nmmi.menu_info.%s.y", jsonSubKey).String());
        dlgItemId = EraJS::readInt(H3String::Format("nmmi.menu_info.%s.item_id", jsonSubKey).String());
    }
}
