#include "framework.h"
//#ifndef PROJECT_NAME
//#define PROJECT_NAME ""

//#endif // !PROJECT_NAME

namespace dllText
{
	const char* PLUGIN_VERSION = "1.7";
	const char* INSTANCE_NAME = "EraPlugin.AssemblyInformation.daemon_n";
	const char* PLUGIN_AUTHOR = "daemon_n";
	//	const char* PROJECT_NAME = "$(ProjectName)";
	const char* PLUGIN_DATA = __DATE__;
}



Patcher* globalPatcher;
PatcherInstance* _PI;


void __stdcall OnReportVersion(Era::TEvent* e)
{

	// show plugin name, version and compilation time
	sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __TIME__);
	std::string temp(h3_TextBuffer);
	Era::ReportPluginVersion(temp.c_str());

}

int __stdcall GameStart(LoHook* h, HookContext* c)
{
	AssemblyInformation::Get();
	//info->LoadDataFromJson();
	UserNotification::Get();
	h->Undo();


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
			Era::ConnectEra();
			globalPatcher = GetPatcher();
			Era::RegisterHandler(OnReportVersion, "OnReportVersion");

			_PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
			_PI->WriteLoHook(0x4EDFFD, GameStart);


		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

SYSTEMTIME operator-(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl)
{
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
}