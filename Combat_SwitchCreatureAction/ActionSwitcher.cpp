#include "ActionSwitcher.h"
namespace switcher
{

	constexpr int DEFAULT_TRIGGER_ID = 200000;
	int trggerId = DEFAULT_TRIGGER_ID;

	_LHF_(CombatStackAccpetsTurn)
	{
		// this is init button hook 

		// here we collect base data and assign to xVars

		// get xVars ptr
		Era::TXVars* xVarsBeforeCall = Era::GetArgXVars();
		// assign data to pass
/*           (*xVarsBeforeCall)[0] = stackId;
		   (*xVarsBeforeCall)[1] = creautreId;
		   (*xVarsBeforeCall)[2] = data1;
		   (*xVarsBeforeCall)[3] = data2;
		   (*xVarsBeforeCall)[4] = data3;*/

		   // call erm function
		Era::FireErmEvent(trggerId);

		// get chnged args after handling by all external erm/plugins
		Era::TXVars* xVars = Era::GetRetXVars();

		//data1 = (*xVars)[2];
		//data2 = (*xVars)[3];
		//data3 = (*xVars)[4];

		// setting button/data stuff next

		return EXEC_DEFAULT;

	}

	void HooksInit()
	{
		_PI->WriteLoHook(0x423423, CombatStackAccpetsTurn);
		Era::AllocErmFunc("ErmTriggerFromPlugin", trggerId);

	}


	void __stdcall ErmTriggerFromPlugin(Era::TEvent* e)
	{


	}
	constexpr int NETMSG_ID = 1989;
#define NONE 0

	void SendNetData_AttackType(int type)
	{

		// if Network combat
		if (IntAt(0x69959C) && type != NONE)
		{
			H3CombatManager* bm = P_CombatManager->Get();
			H3Game* game = P_Game->Get();
			if (bm->isHuman[0] >= 0 && bm->isHuman[1] >= 0)
			{
				//  if (o_GameMgr->IsPlayerHuman(bm->playerID[1])) {
				 //     if (o_GameMgr->IsPlayerHuman(bm->playerID[0])) {
				int activeID = game->GetPlayerID();
				int meID = bm->heroOwner[bm->currentActiveSide];

				if (activeID == meID) {
					_dword_ targetPlayerID = DwordAt(0x697790 - 4 * bm->currentActiveSide);

					INT32 netData[6];
					netData[0] = -1;        // recipient_id
					netData[1] = 0;         // field_04
					netData[2] = NETMSG_ID; // msg_id
					netData[3] = sizeof(netData);        // size
					netData[4] = 0;         // field_10
					netData[5] = type;      // shortData

					// SendData()
					FASTCALL_4(INT32, 0x5549E0, (_dword_)&netData, targetPlayerID, 0, 1);
					//       }
					//   }
				}
			}
		}
	}


	ActionSwitcher::ActionSwitcher() :IGamePatch(_PI)
	{
		CreatePatches();
	}






	void ActionSwitcher::CreatePatches() noexcept
	{
		if (!m_isInited)
		{



			// _pi


			m_isInited = true;









			// id = 2003 (настройки)
			_PI->WriteByte(0x46B3D9 + 1, 4);
			_PI->WriteByte(0x46B3D3 + 1, 44);
			_PI->WriteDword(0x46B3C9 + 1, (int)"icm003q.def");

			// id = 2001 (сдаться)
			_PI->WriteByte(0x46B2CF + 1, 51);
			_PI->WriteByte(0x46B2CB + 1, 44);
			_PI->WriteDword(0x46B2BF + 1, (int)"icm001q.def");

			// id = 2002 (убежать)
			_PI->WriteByte(0x46B354 + 1, 98);
			_PI->WriteByte(0x46B34E + 1, 44);
			_PI->WriteDword(0x46B344 + 1, (int)"icm002q.def");

			// id = 2004 (автобитва)
			_PI->WriteByte(0x46B45E + 1, 145);
			_PI->WriteByte(0x46B458 + 1, 44);
			_PI->WriteDword(0x46B44E + 1, (int)"icm004q.def");

			// id = 2300 (тактика - следующее существо)
			_PI->WriteDword(0x46BC1F + 1, 192);
			_PI->WriteDword(0x46B800 + 1, 188);

			// id = 30722 (тактика - начать битву)
			_PI->WriteDword(0x46BC8F + 1, 398);
			_PI->WriteDword(0x46BC88 + 1, 188);
			_PI->WriteDword(0x46BC7C + 1, (int)"icm012q.def");

			// id = 2005 (окно лога)
			_PI->WriteDword(0x46B807 + 1, 194);
			_PI->WriteDword(0x46B800 + 1, 390);

			// id = 2006 (стрелка лога вверх)
			_PI->WriteDword(0x46B87E + 1, 590);

			// id = 2007 (стрелка лога вниз)
			_PI->WriteDword(0x46B917 + 1, 590);

			// id = 2008 (колдовство)
			_PI->WriteDword(0x46B4E6 + 1, 611);
			_PI->WriteByte(0x46B4E0 + 1, 44);
			_PI->WriteDword(0x46B4D6 + 1, (int)"icm005q.def");

			// id = 2009 (ждать)
			_PI->WriteDword(0x46B56E + 1, 705);
			_PI->WriteByte(0x46B568 + 1, 44);
			_PI->WriteDword(0x46B55E + 1, (int)"icm006q.def");

			// id = 2010 (в защите)
			_PI->WriteDword(0x46B5F6 + 1, 752);
			_PI->WriteByte(0x46B5F0 + 1, 44);
			_PI->WriteDword(0x46B5E6 + 1, (int)"icm007q.def");

			// id = 2 (чат)
			_PI->WriteDword(0x471ED9 + 1, 195);
			_PI->WriteDword(0x471ECF + 1, 390);


		}

	}

	ActionSwitcher& ActionSwitcher::Get()
	{
		static ActionSwitcher instance;
		return instance;	// TODO: insert return statement here
	}

#define NONE 0
#define MELEE 1
#define SHOOT 2
#define CAST 3
#define MOOVE 4
#define RETURN 5
#define NO_RETURN 6
	void SwitcherText::Load() noexcept
	{

		//ButtonName = GetJsonStr("choose_attack_type.name");

		ButtonHints[NONE] = EraJS::read("choose_attack_type.hint.none");
		ButtonRmc[NONE] = EraJS::read("choose_attack_type.rmc.none");

		ButtonHints[MELEE] = EraJS::read("choose_attack_type.hint.melee");
		ButtonRmc[MELEE] = EraJS::read("choose_attack_type.rmc.melee");

		ButtonHints[SHOOT] = EraJS::read("choose_attack_type.hint.shoot");
		ButtonRmc[SHOOT] = EraJS::read("choose_attack_type.rmc.shoot");

		ButtonHints[CAST] = EraJS::read("choose_attack_type.hint.cast");
		ButtonRmc[CAST] = EraJS::read("choose_attack_type.rmc.cast");

		ButtonHints[MOOVE] = EraJS::read("choose_attack_type.hint.moove");
		ButtonRmc[MOOVE] = EraJS::read("choose_attack_type.rmc.moove");

		ButtonHints[RETURN] = EraJS::read("choose_attack_type.hint.return");
		ButtonRmc[RETURN] = EraJS::read("choose_attack_type.rmc.return");

		ButtonHints[NO_RETURN] = EraJS::read("choose_attack_type.hint.no_return");
		ButtonRmc[NO_RETURN] = EraJS::read("choose_attack_type.rmc.no_return");

		AutoBattleQuestion = EraJS::read("choose_attack_type.autobattle_question");

	}

	SwitcherText::SwitcherText()
	{
		Load();
	}

}