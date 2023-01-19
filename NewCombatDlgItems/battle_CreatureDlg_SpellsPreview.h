#pragma once
#include "header.h"

using namespace h3;
H3CombatCreature* creature_dlg_stack = nullptr;

class SpellsDlg
	:public H3Dlg
{

public:
	SpellsDlg(int width, int height, int x = -1, int y = -1):H3Dlg(width, height, x,y)	{	}

	BOOL OnLeftClickOutside() override
	{
		this->Stop();
		return false;
	}
	BOOL OnRightClickOutside() override
	{
		this->Stop();
		return false;
	}
	BOOL OnLeftClick(INT itIndex, H3Msg& msg) override
	{
		this->Stop();
		return false;
	}
	BOOL OnRightClick(H3DlgItem* it) override
	{
		this->Stop();
		return false;
	}

};



void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance* _PI);
