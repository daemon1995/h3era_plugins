// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

using namespace h3;
Patcher* globalPatcher;
PatcherInstance* _PI;


ArtifactsData artifactsData;
PluginText pluginText;



SYSTEMTIME operator-(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl) {
	SYSTEMTIME t_res;
	FILETIME v_ftime;
	ULARGE_INTEGER v_ui;
	__int64 v_right, v_left, v_res;

	SystemTimeToFileTime(&pSr, &v_ftime);
	v_ui.LowPart = v_ftime.dwLowDateTime;
	v_ui.HighPart = v_ftime.dwHighDateTime;
	v_right = v_ui.QuadPart;

	SystemTimeToFileTime(&pSl, &v_ftime);
	v_ui.LowPart = v_ftime.dwLowDateTime;
	v_ui.HighPart = v_ftime.dwHighDateTime;
	v_left = v_ui.QuadPart;

	v_res = v_right - v_left;
	v_ui.QuadPart = v_res;
	v_ftime.dwLowDateTime = v_ui.LowPart;
	v_ftime.dwHighDateTime = v_ui.HighPart;

	FileTimeToSystemTime(&v_ftime, &t_res);
	return t_res;
}


EXTERN_C __declspec(dllexport) SYSTEMTIME& GetTimeDifference(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl, SYSTEMTIME& res)
{

	res = pSr - pSl;
	return res;


	//return pSr - pSl;
}


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

