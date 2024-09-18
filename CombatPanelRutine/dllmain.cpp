// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
//#include "anotherFiles.cpp"


Patcher* globalPatcher;
PatcherInstance* _PI;


_LHF_(arch_OnCombatCreaturePanel_Create)
{

	// !!IF:M^^;
	 //H3DlgPcx16* dlgPcx = H3DlgPcx16::Create(12, 12, "skslt.PCX");
	 //if (dlgPcx)
	 //{

	 //    dlgPcx->SetWidth(12);
	 //    dlgPcx->SetHeight(12);
	 //    //H3CombatMonsterPanel* dlgPanel = *reinterpret_cast<H3CombatMonsterPanel**>(c->ebp - 0x10);
	 //    //dlgPanel->AddItem(dl)


	 //    H3Vector<H3DlgItem*>* items = reinterpret_cast<H3Vector<H3DlgItem*>*>(c->esi);
	 //    if (items)
	 //    {
	 //        items->Add(dlgPcx);

	 //    }

	 //}

	auto dlgg = P_CombatManager->dlg;
	if (dlgg)
	{

		if (auto item = dlgg->GetH3DlgItem(2010))
		{
			item->Hide();
		}
	}


	return EXEC_DEFAULT;

	H3Dlg dlg(H3GameWidth::Get(), H3GameHeight::Get());

	for (size_t i = 0; i < 200; i++)
	{
		for (size_t k = 0; k < 200; k++)
		{
			H3DlgDefButton* b = H3DlgDefButton::Create(i, k, int(eControlId::OK), NH3Dlg::Assets::OKAY_DEF, 0, 1, FALSE, 0);
			dlg.AddItem(b);
		}
	}

	H3DlgDefButton* bttn = dlg.CreateOKButton();
	bttn->AddHotkey(NH3VKey::H3VK_ESCAPE);

	dlg.Start();

	return EXEC_DEFAULT;
}

extern "C" __declspec(dllexport) int Foo(int  arg)
{
	if (arg)
	{
		_PI->ApplyAll();
		H3Messagebox::RMB("Applied");
	}
	else
	{
		_PI->UndoAll();

		H3Messagebox::RMB("undo done");

	}
	return 1;
}


int __stdcall AICalculateMapPosWeight(HiHook* h,const H3Hero* hero, const DWORD a2, const DWORD a3)
{


	LONGLONG result = FASTCALL_3(int, h->GetDefaultFunc(), hero, a2, a3);

	constexpr int MAX_INT = 50000;
	constexpr int MIN_INT = -50000;

	
	if (result > MAX_INT)
		result = MAX_INT;
	else if (result < MIN_INT)
		result = MIN_INT;

	return static_cast<int>(result);
}


int __stdcall Army_Get_AI_Value(HiHook* h, H3Army* army)
{
	ULONGLONG result= 0;

	if (army)
	{

		int i = 6;
		do
		{
			if (army->type[i] != eCreature::UNDEFINED)
				result += static_cast<__int64>(army->count[i]) * P_CreatureInformation[army->type[i]].aiValue;

		} while (i--);
		constexpr int MAX_INT = MAXINT32;

		if (result > MAX_INT >> 3)
			result =  100000000;

	}


	return static_cast<int>(result);
}

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



			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance("test.plugin.totest");

		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

