#define _H3API_PLUGINS_
#include "pch.h"


using namespace h3;
//#include "..\..\headers\era.cpp"   

Patcher* globalPatcher;
PatcherInstance* _PI;





//void CreExpoFix_Apply();




_LHF_(HooksInit)
{

	ERI::ExtendedResourcesInfo::Get(); // one of the best crutch
	//new MapScroller(globalPatcher->CreateInstance("MapScroll.ERA.daemon_n.plugin"));
	AdventureMapHints::Init(globalPatcher->CreateInstance("EraPlugin.AdventureMapHints.daemon_n"));
	
	return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{

	static _bool_ plugin_On = 0;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		//if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		if (!plugin_On)

		{
			plugin_On = 1;
			Era::ConnectEra();

			globalPatcher = GetPatcher();

			_PI = globalPatcher->CreateInstance("EraPlugin.NewAdvMapItems.daemon_n");
			_PI->WriteLoHook(0x4EEAF2, HooksInit);

		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}