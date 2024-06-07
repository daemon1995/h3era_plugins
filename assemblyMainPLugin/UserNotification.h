#pragma once
#include "pch.h"
class UserNotification :
    public IGamePatch
{

    UserNotification(PatcherInstance* _pi);
    void CreatePatches()  noexcept override;

    struct PluginText : public IPluginText
    {
        LPCSTR startGameWithoutOptionsFile;
        virtual void Load() noexcept override;
    } m_text;
    bool m_ignore= false;
    static _LHF_(H3SelectScenarioDialog__StartButtonClick);
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent* e);

public:
    static UserNotification& Get();

};

