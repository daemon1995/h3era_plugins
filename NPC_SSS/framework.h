#pragma once

#define _H3API_PLUGINS_
#define _H3API_MESSAGES_
//#define _H3API_EXCEPTION_
#define _H3API_PATCHER_X86_
#define _WOG_
#define RESIZED_PCX

constexpr int NPC_DLG_LEFT_BTTN_ID = 220;
constexpr int NPC_DLG_RIGHT_BTTN_ID = 221;
constexpr int NPC_MAX_SKILLS = 15;
constexpr int WOG_OPT_STACK_EXPERIENCE = 900;
constexpr int DLG_CREATURE_INFO_MIN_SKILL_ID = 555;
constexpr int DLG_CREATURE_INFO_MAX_SKILL_ID = 569;
//constexpr int MON_COMMANDER_FIRST_A = 174;
//constexpr int MON_COMMANDER_LAST_D = 191;




int GetTxtStringIdBySkillId(int skillId);
char* Get_ITxt(int StrNum, int ColNum);
#include "..\headers\header.h"
#define o_dlgNPC reinterpret_cast<_DlgNPC_*>(0x28604D8)

//#include "..\..\headers\HoMM3.h"
