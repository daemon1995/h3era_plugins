#include "pch.h"
#include "MonsterHandler.h"



void ChangeMonTable()
{
	const int MAX_MON_ID = IntAt(0x4A1657);





	H3CreatureInformation* info = *reinterpret_cast<H3CreatureInformation**>(0x6747B0);
	for (size_t i = 0; i < MAX_MON_ID; i++)
	{
		info[i].attack = 12;// EraJS::read(h3_TextBuffer);//P_CreatureInformation
	}
}

void __stdcall OnAfterWog(Era::TEvent* event)
{



	ChangeMonTable();


	//for (size_t i = 0; i < MAX_MON_ID; i++)
	//{
	//	sprintf(h3_TextBuffer, "era.locale.monster.%d.name", i);
	//	//const char* = "era.locale.monster."
	//	//Era::RedirectMemoryBlock()
	//	//if (!EraJS::isEmpty(h3_TextBuffer))
	//	{
	//		P_CreatureInformation[i].namePlural = EraJS::read(h3_TextBuffer);//P_CreatureInformation
	//		P_CreatureInformation[i].attack = 12;// EraJS::read(h3_TextBuffer);//P_CreatureInformation
	//	}
	//}
	//H3CreatureInformation* info = *reinterpret_cast<H3CreatureInformation**>(0x6747B0);
	//for (size_t i = 0; i < MAX_MON_ID; i++)
	//{
	//	sprintf(h3_TextBuffer, "era.locale.monster.%d.name", i);
	//	//const char* = "era.locale.monster."
	//	//Era::RedirectMemoryBlock()
	//	//if (!EraJS::isEmpty(h3_TextBuffer))
	//	{
	//		P_CreatureInformation[i].namePlural = EraJS::read(h3_TextBuffer);//P_CreatureInformation
	//		info[i].attack = 12;// EraJS::read(h3_TextBuffer);//P_CreatureInformation
	//	}
	//}
}
_LHF_(Crtraits_RightAfterLoad)
{
	ChangeMonTable();

	const int MAX_MON_ID = IntAt(0x4A1657);
	
	std::string jsonKey;
	auto table = H3CreatureInformation::Get();
	//PH3CreatureInformation
	//0x6747B0
	// 
	//0x6747B0



	return EXEC_DEFAULT;
}
void __stdcall OnAfterErmInstruction(Era::TEvent* event)
{

}

void MonsterHandler::Init()
{
	//Era::RegisterHandler(OnAfterErmInstruction, "OnAfterErmInstruction");
	//_PI->WriteLoHook(0x4EF247, Crtraits_RightAfterLoad); //MAIN Main Menu Dlg Run


}