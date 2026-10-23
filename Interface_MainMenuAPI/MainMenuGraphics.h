#pragma once
class MainMenuGraphics :
    public IGamePatch
{

	static MainMenuGraphics* instance;
    static H3BaseDlg* showingMenu;

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

    void H3SelectScenarioDlg__HandleLogo(H3BaseDlg* dlg, const int loopCounter);
    static int __stdcall DlgMainMenu_Redraw(HiHook* h, H3BaseDlg* dlg, BOOL8 redrawScreen,
                                           int firstItemId, int lastItemId);
    static int __stdcall DlgMainMenu_Show(HiHook* h, H3BaseDlg* dlg, int zOrder, BOOL8 draw);

private:
    static _LHF_(SelectScenarioDlg_NewGame);
    static _LHF_(SelectScenarioDlg_ShowAdvancedOptions);
    static _LHF_(SelectScenarioDlg_ShowRandomMap);
    static _LHF_(SelectScenarioDlg_ShowAvailableScenarios);

    MainMenuGraphics(PatcherInstance *_pi);
public:
    static void CreateLogo(H3BaseDlg *dlg);
    static void DrawLogo(H3Msg *msg);
    virtual void CreatePatches() final override;
    static MainMenuGraphics& Get();

};

