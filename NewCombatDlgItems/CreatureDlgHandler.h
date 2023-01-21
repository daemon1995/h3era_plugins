#pragma once
#include "header.h"
constexpr int DLG_SPELLS_BTTN_ID = 4443;
constexpr int WOG_CREATURE_EXP_BUTTON_ID = 4444;
constexpr int DLG_WIDTH = 350;
constexpr int DLG_HEIGHT = 387;

const char* newOkBtn = "iOkay2.def";
const char* newCastBtn = "iMagic.def";
const char* spellListBtn = "iBaff.def";

using namespace h3;
extern PatcherInstance* _PI;
extern H3CombatCreature* creature_dlg_stack;

#define WOG_STACK_EXPERIENCE_ON *(bool*)0x02772730

struct StackActiveSpells
{
	H3Vector<INT32> activeSpellsId;
};
class CreatureDlgHandler
{
	H3CreatureInfoDlg* dlg = nullptr;
	bool expOn = false;
	H3CombatCreature* stack = nullptr;
public:

	CreatureDlgHandler(H3CreatureInfoDlg* dlg, H3CombatCreature* stack) :
		dlg(dlg), stack(stack), expOn(WOG_STACK_EXPERIENCE_ON)
	{
		if (dlg)
		{
			//dlg->AddItem(H3DlgDef::Create(220, 220, "iokay32.def"),false);
			AlignItems();
			if (expOn && dlg->GetDefButton(30722))
				AddExperienceButton();
			if (this->stack != nullptr)
				AddSpellEfects();
		}
	}

	bool SetWitdt;

	bool AlignItems();

	bool AddExperienceButton();
	bool AddSpellEfects();
	bool AddCommanderSkills();

};



void Dlg_CreatureInfo_HooksInit(PatcherInstance* pi);
