#define _H3API_PLUGINS_
#define H3API_SINGLE_HEADER
#include "pch.h"


using namespace h3;
//#include "..\..\headers\era.cpp"   

Patcher* globalPatcher;
PatcherInstance* _PI;





void CreExpoFix_Apply();
void DrawPcx16ResizedBicubic(H3LoadedPcx16* _this, H3LoadedPcx16* src_pcx, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);




_LHF_(HooksInit)
{

	if (H3GameWidth::Get() >= 840)
		new MithrilDisplay(_PI, Era::tr("gem_plugin.mithril_display.popup_hint")); // one of the best crutch
	//new MapScroller(globalPatcher->CreateInstance("MapScroll.ERA.daemon_n.plugin"));
	MapScroller::Init(globalPatcher->CreateInstance("MapScroll.ERA.daemon_n.plugin"));
	AdventureMapHints::Init(globalPatcher->CreateInstance("Pickup.daemon_n.plugin"));

	//if (1)
		//CombatHints::instance(globalPatcher->CreateInstance("CombatHints.ERA.daemon_n.plugin"));
	

	


//	_PI->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_Create);
//	_PI->WriteHiHook(0x412164, THISCALL_, FooBar);
	//CreExpoFix_Apply();



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
			
			_PI = globalPatcher->CreateInstance("ERA.daemon_n.NewAdvMapItems");
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