// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"


Patcher* globalPatcher;
PatcherInstance* _PI;

namespace dllText
{
	constexpr const char* PLUGIN_AUTHOR = "daemon_n";
	constexpr const char* PLUGIN_VERSION = "1.3";
	constexpr const char* PLUGIN_DATA = __DATE__;
	constexpr const char* INSTANCE_NAME = "EraPlugin.MainMenuLogo.daemon_n";
}


_LHF_(HooksInit)
{
	MainMenuPlugin::Get();


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

		if (!plugin_On)

		{
			plugin_On = 1;
			Era::ConnectEra(hModule,dllText::INSTANCE_NAME);
			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
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

