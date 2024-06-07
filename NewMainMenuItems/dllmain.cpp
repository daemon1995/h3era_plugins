// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"

H3DlgPcx* H3BaseDlg__CreateLogo(H3BaseDlg* dlg, const int x = 23, const int y = 23);
void H3SelectScenarioDlg__HandleLogo(H3BaseDlg* dlg, const int loopCounter);
using namespace Era;
// функция получения JSON строк методом ERA

Patcher* _P;
PatcherInstance* _PI;

namespace dllText
{
	constexpr const char* PLUGIN_AUTHOR = "daemon_n";
	constexpr const char* PLUGIN_VERSION = "1.2";
	constexpr const char* PLUGIN_DATA = __DATE__;
	constexpr const char* INSTANCE_NAME = "EraPlugin.MainMenuLogo.daemon_n";
}

int lobbyMenuCounter = 0;

constexpr int logoId = 666;


_LHF_(gem_Dlg_LobbyMenu_NewGame)
{
	if (auto dlg = reinterpret_cast<H3BaseDlg*>(c->ecx))
		H3BaseDlg__CreateLogo(dlg, 23, 23);
	lobbyMenuCounter = 0;

	return EXEC_DEFAULT;
}
_LHF_(gem_Dlg_LobbyMenu_ShowAvailableScenarios)
{
	H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg*>(c->ecx), 1);
	return EXEC_DEFAULT;
}

_LHF_(gem_Dlg_LobbyMenu_ShowRandomMap) //call RMG dlg,
{
	H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg*>(c->ecx), 2);
	return EXEC_DEFAULT;
}
_LHF_(gem_Dlg_LobbyMenu_ShowAdvancedOptions)
{
	H3SelectScenarioDlg__HandleLogo(reinterpret_cast<H3BaseDlg*>(c->ecx), 3);
	return EXEC_DEFAULT;
}
H3DlgPcx* H3BaseDlg__CreateLogo(H3BaseDlg* dlg, const int x, const int y)
{
	constexpr const char* pcxName = "hmm3logo.pcx";

	return dlg->CreatePcx(x, y, logoId, pcxName);

}
void H3SelectScenarioDlg__HandleLogo(H3BaseDlg* dlg, const int loopCounter)
{
	auto* logo = dlg->GetPcx(logoId);

	if (lobbyMenuCounter == loopCounter)
	{
		if (logo)
		{
			logo->Show();
			lobbyMenuCounter = 0;
		}
		else
		{
			if (logo = H3BaseDlg__CreateLogo(dlg))
				logo->Hide();
		}
	}
	else
	{
		if (logo)  logo->Hide(); 
		else
		{
			if (logo = H3BaseDlg__CreateLogo(dlg))
				logo->Hide();
		}
		lobbyMenuCounter = loopCounter;
	}

}
_LHF_(DlgMainMenu_Create)
{
	if (auto dlg = reinterpret_cast<H3BaseDlg*>(c->ecx))
		H3BaseDlg__CreateLogo(reinterpret_cast<H3BaseDlg*>(c->ecx));

	return EXEC_DEFAULT;
}

void HooksInit()
{

	_PI->WriteLoHook(0x4EF259, DlgMainMenu_Create);
	_PI->WriteLoHook(0x4EF331, DlgMainMenu_Create);
	_PI->WriteLoHook(0x4EF668, DlgMainMenu_Create);
	_PI->WriteLoHook(0x4F0799, DlgMainMenu_Create); //goes from new game

	_PI->WriteLoHook(0x4F0B63, gem_Dlg_LobbyMenu_NewGame); //goes from new game
	_PI->WriteLoHook(0x580180, gem_Dlg_LobbyMenu_ShowAdvancedOptions); //goes from new game, tnx too RK
	_PI->WriteLoHook(0x5813D0, gem_Dlg_LobbyMenu_ShowRandomMap); //goes from new game, tnx too RK
	_PI->WriteLoHook(0x580D40, gem_Dlg_LobbyMenu_ShowAvailableScenarios); //goes from new game, tnx too RK

	return;

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
			//ConnectEra();

			_P = GetPatcher();
			_PI = _P->CreateInstance(dllText::INSTANCE_NAME);
			HooksInit();
		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

