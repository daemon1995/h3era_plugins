// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
// #include "framework.h"
using namespace h3;

namespace dllText
{
const char *PLUGIN_VERSION = "1.14";
const char *INSTANCE_NAME = "EraPlugin.ObjectsExtender.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
// const char* PROJECT_NAME = "$(ProjectName)";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText
void __stdcall OnReportVersion(Era::TEvent *e)
{

    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}
Patcher *globalPatcher;
PatcherInstance *_PI;

/*
1. Assign settings from settings dlg with lmitizer - Done
2. Make Dlg Scroll and Input text workin - Done
3. Add Default settings reset - Done
4. Fix Resized Pictures and add new enable button - Done
5. Assign native global limit arrays with Object Limitizer - Done
6. Fix/Rewrite Loop sounds - Done
7. Check several objects.txt merging - Done
8. Dlg open is slow - Done
. ???
. Remove Testing code; Done
. Release; Done

9. Add each zone type settings:
    a. horizontal scroll bar
    b. ...
10. Add saving/loading settings for users;
11. Add default Creature banks support;
12. Remove all extenders code from the plugin:
    a. create API for ObjectExtender class
    b. create unordered map<std::pair<int objectType, int objectSubtype>, std::shared_ptr<ObjectExtender>> of extenders
    c. call events for all extenders from this plugin

13. Create complex objects extender support:
    !!
        create switch for each type of complex object that converts to:
        switch (objectType)

        case
        a. spell scrolls
        b. hero prisons;
        c. pandora's box;
14. Add object ONLY type settings:
    a. create groups of subtypes general settings;
    b. user can create custom groups of subtypes;
    c. dlg should automatically create items but not remove current so reusing (limit number of the groups);
15. Add properties replacer for existing objects by defName/type/subptype triple: Done
    a. load original list into set;
    b. load new
16. fix Dlg Memory leaks with new ERA memory check tools
*/

_LHF_(CrBanksTxt_AfterLoad)
{

    editor::RMGObjectsEditor::Get();

    //! Get the CreatureBanksManager and initialize it
    cbanks::CreatureBanksExtender::Get();
    shrines::ShrinesExternder::Get();
    warehouses::WarehousesExtender::Get();
    gazebo::GazeboExtender::Get();
    colosseumOfTheMagi::ColosseumOfTheMagiExtender::Get();
    // wateringPlace::WateringPlaceExtender::Get();
    wog::WoGObjectsExtender::Get();
    //! Set patches for the RMG_SettingsDlg
    rmgdlg::RMG_SettingsDlg::SetPatches(_PI);

    return EXEC_DEFAULT;
}
namespace EraMemory
{
extern volatile size_t *allocatedMemorySize;
}
void __stdcall OnAdventureMapRightMouseClick(Era::TEvent *e)
{
  //  Era::ShowMessage(Era::IntToStr(*EraMemory::allocatedMemorySize).c_str());
  //  new int[25];
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static BOOL plugin_On = 0; //!< Flag to indicate if the plugin is on

    // TODO: Perform actions based on the reason for calling

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        //!< Attach process, initialize the plugin if it hasn't been initialized before

        //! Check if the plugin is already on
        if (!plugin_On)
        {
            plugin_On = 1;

            //! Connect to the Era framework
            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");

            //! Get the global patcher
            globalPatcher = GetPatcher();
            // Era::RegisterHandler(OnAdventureMapRightMouseClick, "OnAdventureMapRightMouseClick");
            //! Create an instance of the plugin
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            _PI->WriteLoHook(0x4EDE42, CrBanksTxt_AfterLoad);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
