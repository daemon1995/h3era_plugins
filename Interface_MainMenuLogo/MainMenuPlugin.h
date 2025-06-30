#pragma once
class MainMenuPlugin :
    public IGamePatch
{

	static MainMenuPlugin* instance;


private:
    int lobbyMenuCounter = 0;

public:

    struct alignas(4) MenuInformation
    {
        BOOL enable = false;
        BOOL alwaysDraw = false;
        const char* pcxName = h3_NullString;

        int x =23;
        int y = 23;
        INT16 dlgItemId = 666;
        void LoadInfo(const char* jsonSubKey);
    } mainMenu, selectScenario;

private:

    //H3DlgPcx* H3BaseDlg__CreateLogo(H3BaseDlg* dlg, const MenuInformation&info);
    void H3SelectScenarioDlg__HandleLogo(H3BaseDlg* dlg, const int loopCounter);


private:

    static int __stdcall DlgMainMenu_Campaign_Run(HiHook* h, H3BaseDlg* dlg);
    static int __stdcall DlgMainMenu_NewLoad_Create(HiHook* h, H3BaseDlg* dlg, const int val);
    static int __stdcall DlgMainMenu_Create(HiHook* h, H3BaseDlg* dlg);
  //  static int __stdcall DlgMainMenu_Create(HiHook* h, H3BaseDlg* dlg);


    static _LHF_(SelectScenarioDlg_NewGame);
    static _LHF_(SelectScenarioDlg_ShowAdvancedOptions);
    static _LHF_(SelectScenarioDlg_ShowRandomMap);
    static _LHF_(SelectScenarioDlg_ShowAvailableScenarios);


    static int __stdcall DlgMainMenu_Proc(HiHook* h, H3Msg* msg);

    MainMenuPlugin(PatcherInstance *_pi);
public:
    virtual void CreatePatches() final override;
    static MainMenuPlugin& Get();

};

