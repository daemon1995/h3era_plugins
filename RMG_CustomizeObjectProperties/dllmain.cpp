// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "Header.h"
#include ".\headers\era.h"

using namespace h3;
Patcher* globalPatcher;
PatcherInstance* _PI;

int GetIntFromJson(const char* keyName)
{
	std::string jsonText = Era::tr(keyName);
	return keyName != jsonText ? std::max(std::atoi(jsonText.c_str()), 0) : -1;
}

_LHF_(ChangeRMGLimits)
{
	h3::H3RmgRandomMapGenerator* rmgGnr = (h3::H3RmgRandomMapGenerator*)c->ecx;

	if (rmgGnr)
	{
		int objNumber = sizeof(rmgGnr->objectCountByType) / sizeof(INT32);

		H3RmgZoneGenerator* zone;
		int* ObjectLimitPerMap = (int*)0x69CE9C;
		int* ObjectlimitPerZone = (int*)0x69D244;

		H3String keyBaseName = "RMG.object_limit.";
		for (UINT32 i = 0; i < objNumber; i++)
		{
			int perMapFromJson = GetIntFromJson((keyBaseName + std::to_string(i).c_str() + ".map").String());
			if (perMapFromJson != -1)
				ObjectLimitPerMap[i] = perMapFromJson;
			else
			{
				if (perMapFromJson)
				{
					int perZoneFromJson = GetIntFromJson((keyBaseName + std::to_string(i).c_str() + ".zone").String());
					if (perZoneFromJson != -1)
						ObjectlimitPerZone[i] = perZoneFromJson;
				}
				else
					ObjectlimitPerZone[i] = 0;
			}
		}
	}
	
	return EXEC_DEFAULT;
}

BOOL8 ValidateRMGGenObject(H3RmgObjectGenerator* obj)
{
	//if (obj->type < 0) return false;
	//if (obj->subtype < 0) return false;
	if (obj->density < 0) return false;
	if (obj->value < 0) return false;

	return true;
}

_LHF_(CreatureBanksListCreation)
{


	H3Vector<H3RmgObjectGenerator>* crBankList = (H3Vector<H3RmgObjectGenerator>*)c->esi;

	constexpr int objNumber = 232;// sizeof(rmgGnr->objectCountByType) / sizeof(INT32);
	for (int i = eObject::NO_OBJ +1 ; i < objNumber; i++)
	{


		for (int j = 0; j < 100; j++)
		{
			
			//std::string baseJsonKeyName = "RMG.object_gen." + std::to_string(i) + "." + std::to_string(j) + ".";
			H3String keyBaseName = "RMG.object_gen.";

			int value = GetIntFromJson((keyBaseName + std::to_string(i).c_str() + "." + std::to_string(j).c_str() + ".value").String());
			if (value != eObject::NO_OBJ)
			{
				H3RmgObjectGenerator* crBankGen = new H3RmgObjectGenerator;
				
				crBankGen->type = i;
				crBankGen->subtype = j;
				crBankGen->value = value;
				crBankGen->density = GetIntFromJson((keyBaseName + std::to_string(i).c_str() + "." + std::to_string(j).c_str() + ".density").String());

				crBankGen->vTable = (H3RmgObjectGenerator::VTable*)0x00640B74;
				if (ValidateRMGGenObject(crBankGen))
					THISCALL_4(int, 0x5FE2D0, crBankList, crBankList->end(), 1u, &crBankGen);
				delete crBankGen;

			}
		}
	}

	return EXEC_DEFAULT;
}

_LHF_(RMG_ObjGenSimple_Ctor)
{	
	H3RmgObjectGenerator* obj = (H3RmgObjectGenerator*)c->eax;
	if (obj->type == eObject::TRADING_POST
		|| obj->type == eObject::TRADING_POST_SNOW)
	{
		c->ecx = 20000; // ecx value
		c->edx = 20; // edx density
	}

	return EXEC_DEFAULT;
}

//H3CreatureBankState arr3[1500];
_LHF_(ChangeRMGLimitsA)
{


	//_PI->WriteDword(0x47A3C1 + 1, (int)&newTable);
	//_PI->WriteDword(0x7B8300, (int)&guard_types);// guard types up to 5
	//	_PI->WriteDword(0x802ED0, (int)&award_creatures);// award creatures

	//_PI->WriteDword(0x47A3EC + 1, (int)&newTable);
//	_PI->WriteDword(0x7C8E54, (int)&newTable + 4);
//	_PI->WriteDword(0x47A4B6 +3, (int)&newTable+4);


	//*(int*)0x6961BC = 2048;
	//H3TextTable* txtTable = (H3TextTable*)c->eax;
	//coounter = txtTable->CountRows();
	// 
	//H3LoadedDef* def;
	//def->Load("123123.def");
	

	//Era::RedirectMemoryBlock((void*)0x7C8E50, sizeof(newTable), newTable);
	//int* ptr = (int*)&newTable[0];
	//*(int**)(c->ebp-4) = ptr +1;
	//static int cnt;
	//coounter++;
	//if (c->eax == 1 && once)
	//{
	//	//c->eax += 4;
	//	once = false;
	//	//*(int*)(c->ebp - 8) = 4;

	////	arr[cnt++] = (c->ebp - 8);// c->eax;
	//	///arr[cnt++] = (c->ebp - 8);// c->eax;

	//}
	return EXEC_DEFAULT;

	
}

//

//struct _CrBankMap_ {
//	long  DMonsterType[7]; // -1 - no
//	long  DMonsterNum[7];
//	long  Res[7];
//	long  BMonsterType; // -1 - no
//	char  BMonsterNum;
//	Byte _u1[3];
//	_ArtList_ Arts;
//};
//struct _CCrBank_ { // type 16
//	unsigned _u1 : 5; //
//	unsigned  Whom : 8; // ęňî čç čăđîęîâ őîňü ęîăäŕ çŕőîäčë
//	unsigned  Ind : 12; // číäĺęń ďî ďîđ˙äęó ńňđóęňóđ
//	unsigned  Taken : 1; // âç˙ň (1) čëč ĺůĺ íĺň (0)
//	unsigned _u2 : 6;
//};
//{0x47A4A8 + 3, DS(crbankt1), 4},
//{ 0x47A4AF + 3,DS(crbankt2),4 },
//{ 0x47A4B6 + 3,DS0(&CrBankTable[0].Name),4 },
//{ 0x47A68F + 1,DS0(&CrBankTable[BANKNUM].Name),4 },
//{ 0x47A3C1 + 1,DS(CrBankTable),4 },
//{ 0x47A3EC + 1,DS0(CrBankTable),4 },
//{ 0x67029C  ,DS0(CrBankTable),4 },
//{ 0x47A3BA + 1,DS0(BANKNUM),1 },
//{ 0x47A3E5 + 1,DS0(BANKNUM),1 },
//{ (Byte*)0x67037C,(Byte*)crbankt1,sizeof(crbanks1) * BANKNUM_0 },
//{ (Byte*)0x6702A0,(Byte*)crbankt2,sizeof(crbanks2) * BANKNUM_0 },
_LHF_(HooksInit)
{

	
	_PI->WriteLoHook(0x54C50C, ChangeRMGLimits);
//	_PI->WriteLoHook(0x539C73, CreatureBanksListCreation);
//	_PI->WriteLoHook(0x4EF252, gem_Dlg_MainMenu_Create);

	//memcpy(newTable, oldTable, sizeof(H3CreatureBankSetup) *21);
	
	//_PI->WriteLoHook(0x53465D, RMG_ObjGenSimple_Ctor);


	return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	static BOOL plugin_On = 0;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		//if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		if (!plugin_On)

		{

			plugin_On = 1;
			Era::ConnectEra();
			
			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance("RMG.daemon.plugin");


			_PI->WriteLoHook(0x4EEAF2, HooksInit);

		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
