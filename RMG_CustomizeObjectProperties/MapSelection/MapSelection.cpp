#include "../pch.h"
#include "MapSelection.h"
#include "MapSize.h"
#include "RandomMapOptions.h"
#include "MenuDialogs.h"
#include "../RMG_SettingsDlg.h"

namespace mapselection
{
void Init()
{
    static bool installed = false;
    if (installed)
        return;
    installed = true;
    rmgdlg::RMG_SettingsDlg::SetPatches(_PI);
    InstallScenarioName();
    MapSize_Init();
    RandomMapOptions_Init();
}
}
