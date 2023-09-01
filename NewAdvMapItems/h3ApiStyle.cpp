#define _H3API_PLUGINS_
#define H3API_SINGLE_HEADER
#include "pch.h"
#include "MitrilDisplay.h"
#include "MapScroller.h"
using namespace h3;
#include "..\..\headers\era.cpp"   

Patcher* globalPatcher;
PatcherInstance* _PI;
//MithrilDisplay mithrilDisplay;

void __stdcall DoSth(const char*)
{

}

namespace db
{
	void echoB(int a)
	{
		Era::y[1] = a;

		Era::ExecErmCmd("IF:L^%y1^;");
	}

	void echo(const char* a)
	{
		sprintf(Era::z[1], a);
		Era::ExecErmCmd("IF:L^%z1^;");
	}
	void echoA(float a)
	{
		Era::e[1] = a;
		Era::ExecErmCmd("IF:L^%e1^;");
	}

	void dump(int a)
	{
		Era::y[1] = a;
		Era::ExecErmCmd("IF:M^%y1^;");
	}
	void dump(const char* a)
	{
		sprintf(Era::z[1], a);
		Era::ExecErmCmd("IF:M^%z1^;");
	}

}


void CreExpoFix_Apply();

_LHF_(HooksInit)
{

	if (H3GameWidth::Get() >= 840)
		new MithrilDisplay(_PI, Era::ExecErmCmd, Era::tr("gem_plugin.mithril_display.popup_hint")); // one of the best crutch
	//new MapScroller(_PI, Era::ExecErmCmd, db::echo); // one of the best crutch

	CreExpoFix_Apply();



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

			_PI = globalPatcher->CreateInstance(const_cast<char*>("ERA.daemon_n.NewAdvMapItems"));
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