// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
using namespace h3;
namespace dllText
{
	constexpr const char* PLUGIN_AUTHOR = "daemon_n";
	constexpr const char* PLUGIN_VERSION = "1.0";
	constexpr const char* PLUGIN_DATA = __DATE__;
	constexpr const char* INSTANCE_NAME = "EraPlugin.LanguageSelectionDlg.daemon_n";
}


void __stdcall OnAfterWog(Era::TEvent* event);
_LHF_(Crtraits_RightAfterLoad);

_LHF_(HooksInit)
{

	LanguageSelectionDlg::Init();


	return EXEC_DEFAULT;
}

Patcher* globalPatcher;
PatcherInstance* _PI;
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{

	static bool pluginIsOn = false;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (!pluginIsOn)
		{
			pluginIsOn = true;

			Era::ConnectEra();
			if (Era::GetVersionNum() >= 3906)
			{
				globalPatcher = GetPatcher();
				_PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
				_PI->WriteLoHook(0x4EEAF2, HooksInit);


			}
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

