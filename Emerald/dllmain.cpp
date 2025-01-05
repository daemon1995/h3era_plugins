// dllmain.cpp: определяет точку входа для приложения DLL.
#include "pch.h"


//#include "emerald.h"
//#include "..\..\headers\header.h"


typedef Era::TEvent* PEvent;

extern void __stdcall Emerald(PEvent e);
extern void __stdcall ReallocProhibitionTables(PEvent e);
extern void __stdcall LoadConfigs(PEvent e);


GAMEDATA save;
Patcher* globalPatcher;
PatcherInstance* emerald;

void __stdcall InitData (PEvent e)
{
}


void __stdcall StoreData (PEvent e)
{
	Era::WriteSavegameSection(sizeof(save), (void*)&save, PINSTANCE_MAIN);
}


void __stdcall RestoreData (PEvent e)
{
	Era::ReadSavegameSection(sizeof(save), (void*)&save, PINSTANCE_MAIN);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call, // TODO - из H3_API ???
                       LPVOID lpReserved
					 )
{
	static BOOL plugin_On = 0;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (!plugin_On)
		{
			plugin_On = 1;
			Era::ConnectEra();
			globalPatcher = GetPatcher();
			emerald = globalPatcher->CreateInstance(PINSTANCE_MAIN);

			//Storing data
			Era::RegisterHandler(InitData, "OnAfterErmInstructions");
			Era::RegisterHandler(StoreData, "OnSavegameWrite");
			Era::RegisterHandler(RestoreData, "OnSavegameRead");

			//опосля выделения памяти под динамику		
			Era::RegisterHandler(ReallocProhibitionTables, "OnAfterCreateWindow");

			//RegisterHandler(LoadConfigs, "OnBeforeErmInstructions");
			Era::RegisterHandler(LoadConfigs, "OnAfterCreateWindow");

			//основной патчинг
			Era::RegisterHandler(Emerald, "OnAfterWoG");
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
