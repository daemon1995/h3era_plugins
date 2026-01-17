// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

Patcher *globalPatcher;
PatcherInstance *_PI;

#define COUNT_TYPES 10

char *ButtonName;

#define NONE 0
#define MELEE 1
#define SHOOT 2
#define CAST 3
#define MOOVE 4
#define RETURN 5
#define NO_RETURN 6

static int currentType = NONE;

_byte_ typesArray[COUNT_TYPES];
BYTE typesIterator;

_byte_ saveNpcTypesAction[156];

#define BTTN_ID 2020
_LHF_(LoHook_InitTxtFiles)
{

    switcher::ActionSwitcher::Get();
    return EXEC_DEFAULT;
}
_LHF_(Ancient_Calc)
{
    // get defender stack

    IntAt(0x2832700) = c->eax;
    return EXEC_DEFAULT;


}

int __stdcall WoG_GetStackDefence(HiHook *h, H3CombatCreature *_this, H3CombatCreature *attacker,
                                  const BOOL includeFrenzy)
{
    int finalDefence = 0;

    finalDefence = THISCALL_3(int, h->GetDefaultFunc(), _this, attacker, includeFrenzy);
    if (attacker && !P_CombatManager->IsHiddenBattle())
    {
        const int attackerType = attacker->type;
        if (attackerType == eCreature::BEHEMOTH || attackerType == eCreature::ANCIENT_BEHEMOTH || attackerType == 156)
        {
            int expectedDefence = THISCALL_3(int, 0x04422B0, _this, attacker, includeFrenzy);

            libc::sprintf(h3_TextBuffer, "attacker type: %d,  defense Expected/Final: %d / %d", attackerType,
                          expectedDefence, finalDefence);
            P_CombatManager->dlg->ShowHint(h3_TextBuffer, 1);
        }
    }
    return finalDefence;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static bool pluginIsOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;
            globalPatcher = GetPatcher();
            static LPCSTR moduleName = "EraPlugin.Combat.SwitchCreatureAction.daemon_n";
            _PI = globalPatcher->CreateInstance(moduleName);
            Era::ConnectEra(hModule, moduleName);

            //// инициализация параметров (при старте карты)
            //     _PI->WriteLoHook(0x4EEAC0, LoHook_InitTxtFiles);
           // _PI->WriteHiHook(0x075D3EC, THISCALL_, WoG_GetStackDefence);
            //// добавление кнопки на экран битвы
            //_PI->WriteLoHook(0x46B664, Y_AddChooseAttackButton);

            //// взаимодействия с кнопокой по ЛКМ и ПКМ
            //_PI->WriteHiHook(0x474714, CALL_, EXTENDED_, THISCALL_, Y_BattleMgr_MouseClickRMC_OnButton);
            //_PI->WriteLoHook(0x474764, Y_BattleMgr_ProcessAction_LMC);

            //// установка событий обработки типа действия
            //_PI->WriteLoHook(0x442635, Y_BattleMgr_CanStackShoot);
            //_PI->WriteLoHook(0x75E0E7, Y_BattleMgr_WOG_HarpyReturn);
            //_PI->WriteHiHook(0x41FA10, SPLICE_, EXTENDED_, THISCALL_, Y_BattleMgr_CanCast);

            //// проверка на возможность стрельбы уже во время исполнения атаки монстром
            //_PI->WriteCodePatch(0x445829, "%n", 12); // 12 nops
            //_PI->WriteLoHook(0x445829, Y_BattleMgr_MakeAttack_isStackCanShoot);

            //// нажатие на кнопку автоматической битвы
            //_PI->WriteLoHook(0x47478A, Y_ClickAutoBattleButton_A_id2004);

            //// при установке активного стека (обновить кнопку)
            //_PI->WriteLoHook(0x477D98, Y_BattleMgr_SetActiveStack);

            //// после каста героем (обновить кнопку)
            //_PI->WriteLoHook(0x478997, Y_LoHook_BM_AfterHeroSpellCast);

            //// восстанавливаем оригинальное действие кнопки "В защите"
            //// на которое ставит свой хук WOG
            //_PI->WriteHexPatch(0x47265C, "B8 08000000 E9 64FFFFFF");

            //// получение данных по сети
            //// send 0x4787BC
            //// get 0x473D27
            //_PI->WriteHiHook(0x473D27, CALL_, EXTENDED_, FASTCALL_, Y_HiHook_BM_GetNetData);

            // LPCSTR temp = "$(H3GameDir)";
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
