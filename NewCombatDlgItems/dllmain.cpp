// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

Patcher* globalPatcher;
PatcherInstance* _PI;
void Dlg_MonPreview_HooksInit(PatcherInstance* pi);
void Dlg_CreatureInfo_HooksInit(PatcherInstance* pi);
void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance* pi);

//#include "battle_Dlg_MonPreview.cpp"


using namespace h3;

_LHF_(HooksInit)
{

	if (EraJS::readInt("gem_plugin.combat_dlg.enable.popup")) // if json is active
	{
		//Dlg_MonPreview_HooksInit(_PI);
		preview::MonPreview::Get();
	}

	if (EraJS::readInt("gem_plugin.combat_dlg.enable.creature_info")) // if json is active
		Dlg_CreatureInfo_HooksInit(_PI);

	if (EraJS::readInt("gem_plugin.combat_dlg.enable.spells")) // if json is active
		Dlg_CreatureSpellInfo_HooksInit(_PI);

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

			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance("EraPlugins.CreatureInformation.daemon_n");
			Era::ConnectEra(hModule,"EraPlugins.CreatureInformation.daemon_n" );
			_PI->WriteLoHook(0x4EEAF2, HooksInit);

		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
