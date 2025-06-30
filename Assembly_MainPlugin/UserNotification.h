#pragma once
#include "framework.h"
class UserNotification : public IGamePatch
{
	static UserNotification* instance;

    UserNotification(PatcherInstance *_pi);
    void CreatePatches() noexcept override;

    struct PluginText : public IPluginText
    {
        LPCSTR startGameWithoutOptionsFile;
        virtual void Load() noexcept override;
    } m_text;
    BOOL m_ignore = false;

    struct UpdateNotification
    {
        DWORD m_lastCheckTime = 0;
        BOOL32 m_needToCheck = true;
        DWORD32 m_chechInterval = 1000 * 60 * 15;
    } m_updateInfo;

    static _LHF_(H3SelectScenarioDialog__StartButtonClick);
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *e);

    static void __stdcall AdvMgrDlg_RedrawLog(HiHook *h, H3AdventureMgrDlg *dlg, bool redrawLog);

    static char __stdcall AdvMgr_DrawMap(HiHook *h, H3AdventureManager *advMan, const __int16 mapX, const __int16 mapY,
                                         const __int16 mapZ, const int hotSeat, char updateInfoPanel);

  public:
    static UserNotification &Get();
};
