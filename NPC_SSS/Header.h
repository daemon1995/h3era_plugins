#pragma once

#define _H3API_PLUGINS_
#define _H3API_MESSAGES_
//#define _H3API_EXCEPTION_
#define _H3API_PATCHER_X86_
#define o_dlgNPC ((_DlgNPC_*)0x28604D8)
#define NPC_DLG_LEFT_BTTN_ID 220
#define NPC_DLG_RIGHT_BTTN_ID 221
#define NPC_MAX_SKILLS 15
#define WOG_OPT_STACK_EXPERIENCE 900
#define DLG_CREATURE_INFO_MIN_SKILL_ID 555
#define DLG_CREATURE_INFO_MAX_SKILL_ID 569
#define MON_COMMANDER_FIRST_A 174
#define MON_COMMANDER_LAST_D 191
//#include <iostream>


struct _Npc_;
_Npc_* GetNpc(int hero_id);
int GetTxtStringIdBySkillId(int skillId);
char* Get_ITxt(int StrNum, int ColNum);
#include "H3API.hpp"

#include "era.h"

#include "..\..\headers\WogClasses.h"
//#include "..\..\headers\HoMM3.h"
