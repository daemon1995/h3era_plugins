#define _H3API_PLUGINS_
#include "RandomCombatSimulator.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

static _ERH_(OnAfterWog) { randomcombat::Initialize(); }
static _ERH_(OnGameEnter) { randomcombat::GameEnter(); }
static _ERH_(OnGameLeave) { randomcombat::GameLeave(); }
static _ERH_(OnSetupBattlefield) { randomcombat::SetupBattlefield(); }
static _ERH_(OnBeforeFastQuitToGameMenu) { randomcombat::RestoreBattleState(); }
static _ERH_(OnAfterReloadLanguageData) { randomcombat::ReloadButtonText(); }

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        constexpr LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
        Era::ConnectEra(module, instanceName);
        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(instanceName);
        _REH_(OnAfterWog);
        _REH_(OnGameEnter);
        _REH_(OnGameLeave);
        _REH_(OnSetupBattlefield);
        _REH_(OnBeforeFastQuitToGameMenu);
        _REH_(OnAfterReloadLanguageData);
    }
    return TRUE;
}
