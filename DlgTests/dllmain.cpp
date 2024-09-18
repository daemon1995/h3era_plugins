// dllmain.cpp : Определяет точку входа для приложения DLL.
//#define _H3API_PLUGINS_
#define _H3API_PLUGINS_

#include "framework.h"
#include "TestDlg.h"
//#include "..\..\headers\H3API_RK\single_header\H3API.hpp"

using namespace h3;


Patcher* globalPatcher;
PatcherInstance* _PI;

namespace db
{
	//void echo(int a)
	//{
	//	Era::y[1] = a;

	//	Era::ExecErmCmd("IF:L^%y1^;");
	//}

	//void echo(const char* a)
	//{
	//	sprintf(Era::z[1], a);
	//	Era::ExecErmCmd("IF:L^%z1^;");
	//}
	//void echo(float a)
	//{
	//	Era::e[1] = a;
	//	Era::ExecErmCmd("IF:L^%e1^;");
	//}

	//void dump(int a)
	//{
	//	Era::y[1] = a;
	//	Era::ExecErmCmd("IF:M^%y1^;");
	//}
	//void dump(const char* a)
	//{
	//	sprintf(Era::z[1], a);
	//	Era::ExecErmCmd("IF:M^%z1^;");
	//}

}


//void debug(int a);


TestDlg* p_dlg = nullptr;
int dlgWidth, dlgHeight;

int defDtorCounter = 0;
int itemDtorCounter = 0;
bool dtorCalled = false;
_LHF_(DlgDef_Dtor)
{
	if (dtorCalled)
		defDtorCounter += 1;
	return EXEC_DEFAULT;
}

_LHF_(DlgItem_Dtor)
{

	if (dtorCalled)
		itemDtorCounter += 1;
	return EXEC_DEFAULT;
}

H3LoadedPcx16* drawBuffer = nullptr;
//_Pcx16_* __thiscall Pcx16_Draw_ToPcx16(
//	_Pcx16_* pcx,
//	int srcX,
//	int srcY,
//	int width,
//	int height,
//	int buffer,
//	int dstX,
//	int dstY,
//	int maxX,
//	int maxY,
//	int scanline,
//	int TransparentColor)

_LHF_(TownScreen_EndOfRedraw)
{
	//TestDlg dlg(500, 500, -1, -1);
	//dlg.CreateOKButton();
	//dlg.Start();



	P_WindowManager->screenPcx16->DrawToPcx16(0, 0, 1, drawBuffer);

	
	//drawBuffer->DrawToPcx16(0, 0, 1, P_WindowManager->screenPcx16, 25, 25);

	P_WindowManager->screenPcx16->buffer;
	THISCALL_12(VOID, 0x44DF80, drawBuffer, 0, 0, dlgWidth, dlgHeight, P_WindowManager->screenPcx16->buffer, 200, 115, 500, 500, P_WindowManager->screenPcx16->scanlineSize, 1); // , x, y, dest->width, dest->height, dest->scanlineSize, transparent);

	return EXEC_DEFAULT;
}
_LHF_(DlgTown_AfterCreate)
{

	return EXEC_DEFAULT;
}

_LHF_(MainWindow_F1)
{

	c->return_address = 0x4F877D;
	return NO_EXEC_DEFAULT;


	return EXEC_DEFAULT;

}
_LHF_(HooksInit)
{

	//_PI->WriteLoHook(0x4FBD71, gem_Dlg_MainMenu_Create);
	//_PI->WriteLoHook(0x4EA8B5, DlgDef_Dtor);
	// 
	//_PI->WriteLoHook(0x5FE9F9, DlgItem_Dtor);
	//_PI->WriteLoHook(0x5D5926, TownScreen_EndOfRedraw);
	//_PI->WriteLoHook(0x5C681E, DlgTown_AfterCreate);

	dlgWidth = H3GameWidth::Get() - 100;
	dlgHeight = IntAt(0x5C38EF + 1);

	
	IntAt(0x5C38F6 + 1) = dlgWidth;
	//IntAt(0x5C38EF + 1) = dlgHeight;


	//_PI->WriteDword(0x5C38F6 +1, )
	
	//drawBuffer = H3LoadedPcx16::Create(P_WindowManager->screenPcx16->width, P_WindowManager->screenPcx16->width);
	//drawBuffer = H3LoadedPcx16::Create(dlgWidth, dlgHeight);
	
	//_PI->WriteLoHook(0x4F8767, MainWindow_F1);

	_PI->WriteWord(0x4F870B, 0x9090);
	return EXEC_DEFAULT;
}


static _LHF_(NewScenarioDlg_Create);


void __stdcall NewScenarioDlg_Create(HiHook* hook, H3SelectScenarioDialog* dlg, H3Msg* msg)
{
	THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);

	H3DlgCaptionButton* bttn = dlg->GetCaptionButton(4444);
	if (bttn)
	{
		bttn->AddHotkey(h3::eVKey::H3VK_W);


	}
	bttn = dlg->CreateCaptionButton(bttn->GetX(), bttn->GetY() + 45, bttn->GetWidth(), bttn->GetHeight(), 4500, bttn->GetDef()->GetName(), "ERA options", h3::NH3Dlg::Text::SMALL, 0);
	if (bttn)
	{
		bttn->SetClickFrame(1);
		bttn->AddHotkey(h3::eVKey::H3VK_E);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{


	case DLL_PROCESS_ATTACH:


		globalPatcher = GetPatcher();
		_PI = globalPatcher->CreateInstance("ERA.daemon_n.testDlg");
		_PI->WriteLoHook(0x4EEAF2, HooksInit);

		// if (ver.era()      )
		{
			Era::ConnectEra();
		}


	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

