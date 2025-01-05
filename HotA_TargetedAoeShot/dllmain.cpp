// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

Patcher* globalPatcher;
PatcherInstance* _PI;


bool isChooseShootSplashMagog = false;

int __stdcall Y_BattleMrg_SubProc(HiHook* hook, H3CombatManager* bm, H3Msg* msg)
{
	if (msg->command == 1) {

		if (msg->subtype == 34) { //кнопка G
			//_BattleStack_* actStack = bm->GetCurrentStack();
			H3CombatCreature* actStack = bm->activeStack;
			if (
				actStack->type == eCreature::MAGOG 
				&& THISCALL_2(char, 0x442610, actStack, 0
				) /*CanCreatureShoot*/) 
			{
				if (!isChooseShootSplashMagog) {
					isChooseShootSplashMagog = true;

					int x, y, target;
					H3POINT::GetCursorPosition(x, y); // o_GetIngameCursorPos(&x, &y);

					FASTCALL_4(void, 0x59EF60, bm, 0, 21, 1);
					isChooseShootSplashMagog = false;
					//bm->Field<_int_>(+60) = 7;	// bm->Action			(BG:A)
					bm->action = NH3Combat::NBattleAction::eBattleAction::SHOOT;
				}
			}
		}
	}
	return THISCALL_2(int, hook->GetDefaultFunc(), bm, msg);
}

void __stdcall Y_Battle_SetCursorCadreMagogs(HiHook* hook, H3MouseManager* mouse, int frame, int type)
{
	if (isChooseShootSplashMagog) {
		frame = 18; type = 2; // H3MouseManager
	}
	return THISCALL_3(void, hook->GetDefaultFunc(), mouse, frame, type);
}

char __stdcall Y_BattleMrg_Attack(HiHook* hook, H3CombatCreature* actStack, int gex, int a2)
{

	return THISCALL_3(char, hook->GetDefaultFunc(), actStack, gex, a2);
}

H3CombatCreature* __stdcall Y_BattleMrg_GetMonAtPos(HiHook* hook, int gex)
{
	H3CombatCreature* stackAtPos;

	// эта функция пока не пашет номрально

	stackAtPos = THISCALL_1(H3CombatCreature*, hook->GetDefaultFunc(), gex);

	if (isChooseShootSplashMagog && !stackAtPos) {
		H3CombatManager* bm = P_CombatManager->Get();
		H3CombatCreature* st = bm->activeStack; //    o_BattleMgr->GetCurrentStack();
		stackAtPos = &bm->stacks[1 - st->side][20]; //&o_BattleMgr->stack[1 - st->side][20];
		int x, y;
		H3POINT::GetCursorPosition(x, y); // o_GetIngameCursorPos(&x, &y);
		stackAtPos->position = bm->SquareAtCoordinates(x, y);//stackAtPos->hex_ix = o_BattleMgr->GetHexIxAtXY(x, y);
		stackAtPos->numberAlive = 1;//stackAtPos->count_current = 1;
		isChooseShootSplashMagog = false;
	}

	return stackAtPos;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
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

            //! Get the global patcher
            globalPatcher = GetPatcher();

            // THIS PLUGIN IS NOT NEEDED ANYMORE

            //! Create an instance of the plugin
            _PI = globalPatcher->CreateInstance("js_HotA_TargetedAoeShot");

			_PI->WriteHiHook(0x4746B0, SPLICE_, EXTENDED_, THISCALL_, Y_BattleMrg_SubProc); 		// нажатие клавиши G = 43
			_PI->WriteHiHook(0x59FB1E, CALL_, EXTENDED_, THISCALL_, Y_Battle_SetCursorCadreMagogs); // подмена курсора мыши в время "каста" магогами
			_PI->WriteHiHook(0x4457C4, CALL_, EXTENDED_, THISCALL_, Y_BattleMrg_GetMonAtPos); 		// при подготовке выстрела
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

