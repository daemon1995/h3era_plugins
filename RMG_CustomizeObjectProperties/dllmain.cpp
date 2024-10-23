// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
//#include "framework.h"
using namespace h3;
Patcher* globalPatcher;
PatcherInstance* _PI;


/*!
  \brief Entry point of the DLL.

  \param hModule Handle to the DLL module.
  \param ul_reason_for_call Reason for calling the function.
  \param lpReserved Reserved for future use.

  \return TRUE if the function succeeds, FALSE otherwise.

  This function is the entry point of the DLL. It handles the different
  reasons for calling the function and initializes the plugin if it
  hasn't been initialized before.
*/


/*

1. Assign settings from settings dlg with lmitizer
2. Make Dlg Scroll and Input text working;
3. Add Default settings reset;
4. Fix Resized Pictures and add new enabele button;
5. Assign native global limit arrays with Object Limitizer
6. Fix/Rewrite Loop sounds
7. Check several objects.txt merging
8. Dlg open is slow
. ???
. Remove Testing code;
. Release;




*/

_LHF_(CrBanksTxt_AfterLoad)
{
	editor::RMGObjectsEditor::Get();

	//! Get the CreatureBanksManager and initialize it
	cbanks::CreatureBanksExtender::Get();
	shrines::ShrinesExternder::Get();
	warehouses::WarehousesExtender::Get();

	//! Set patches for the RMG_SettingsDlg
	rmgdlg::RMG_SettingsDlg::SetPatches(_PI);

	return EXEC_DEFAULT;
}



BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD ul_reason_for_call,
	LPVOID lpReserved)
{
	static BOOL plugin_On = 0;  //!< Flag to indicate if the plugin is on

	// TODO: Perform actions based on the reason for calling

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		//!< Attach process, initialize the plugin if it hasn't been initialized before

		//! Check if the plugin is already on
		if (!plugin_On)
		{
			plugin_On = 1;

			//! Connect to the Era framework
			Era::ConnectEra();

			//! Get the global patcher
			globalPatcher = GetPatcher();

			//! Create an instance of the plugin
			_PI = globalPatcher->CreateInstance("EraPlugin.ObjectsExtender.daemon_n");
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

