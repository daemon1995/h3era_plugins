// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

using namespace h3;
Patcher* globalPatcher;
PatcherInstance* _PI;


ArtifactsData artifactsData;
PluginText pluginText;






_LHF_(HooksInit)
{

	int artsAddedd = artifactsData.LoadJsonData();
	pluginText.LoadText();



	MovementProcedure::SetPatches(_PI);
	SpellsProcedure::SetPatches(_PI);
	BuildingProcedure::SetPatches(_PI);
	IncomeProcedure::SetPatches(_PI);
	ActionProcedure::SetPatches(_PI);


	//_PI->WriteHiHook(0x43B770,THISCALL_,)
	
	return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{

	static BOOL plugin_On = 0;  //!< Flag to indicate if the plugin is on

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		if (!plugin_On)
		{
			plugin_On = 1;

			globalPatcher = GetPatcher();

			//! Create an instance of the plugin
			_PI = globalPatcher->CreateInstance("daemon_n.ArtifactsAdditionalFeatures.plugin");

			_PI->WriteLoHook(0x4EEAF2, HooksInit);



			//
		}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

