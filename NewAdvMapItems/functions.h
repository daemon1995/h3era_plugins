#pragma once
#include ".\h3api_single\H3API.hpp"
#include ".\headers\era.h" 


#define o_HD_X (*(INT*)0x401448)
#define o_HD_Y (*(INT*)0x40144F)
#define MITHRIL_DEF_ID 3233
#define MITHRIL_TEXT_ID (UINT16)3234
#define MITHRIL_FRAME_ID 3235

using namespace Era;
using namespace h3;


int __stdcall OnAdventureDlgCreate(LoHook* h, HookContext* c);
int __stdcall OnResourceBarDlgUpdate(LoHook* h, HookContext* c);
int __stdcall BeforeHotseatMsgBox(LoHook* h, HookContext* c);
void SetFrameColor( H3BaseDlg* advDlg, const int& plColor);
