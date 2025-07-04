#include "pch.h"

using namespace h3;


//void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance* _PI);


BOOL ShowStackAvtiveSpells(H3CombatCreature* stack, bool isRMC, H3DlgItem* clickedItem)
{

	int arr_size = sizeof(stack->activeSpellDuration) / sizeof(INT32);
	int activeSpellsNum = stack->activeSpellNumber;
	int _sqrt = static_cast<int>(floor(sqrt(activeSpellsNum)));
	int columns = _sqrt;

	int rows = activeSpellsNum / columns;
	if (activeSpellsNum % columns)
		rows++;
	if (rows > columns)
	{
		rows--;
		columns++;
	}
	H3DefLoader _def("spellint.def");
	int d_w = _def->widthDEF;
	int d_h = _def->heightDEF;
	int width = (d_w + 5) * columns + 35;
	int height = (d_h + 5) * rows + 35;

	H3Dlg* dlg = new H3Dlg(width, height);


	int x = 20, y = 20;
	int duration = 0, counter = 0;
	for (INT32 i = 0; i < arr_size; i++)
	{
		duration = stack->activeSpellDuration[i];
		if (duration)
		{

			H3DlgDef* def = H3DlgDef::Create(x, y, _def->GetName(), i + 1);
			dlg->AddItem(def);


			libc::sprintf(h3_TextBuffer, "x%d", duration);
			//str.Append(duration);
			H3DlgText* text = H3DlgText::Create(x, y + 25, d_w, 14, h3_TextBuffer, h3::NH3Dlg::Text::TINY, 1, 0, eTextAlignment::MIDDLE_RIGHT);
			if (i != NH3Spells::eSpell::BERSERK && i != NH3Spells::eSpell::DISRUPTING_RAY && i != NH3Spells::eSpell::BIND)
				dlg->AddItem(text);
			if (++counter == columns)
			{
				counter = 0;
				x = 20;
				y += d_h + 5;
			}
			else
				x += d_w + 5;
		}
	}
	//


	if (isRMC)
		dlg->PlaceAtMouse();
	else if (clickedItem != nullptr)
	{
		int xPos = clickedItem->GetAbsoluteX();
		int yPos = clickedItem->GetAbsoluteY();
		IntAt((int)dlg + 0x18) = Clamp(0, xPos, H3GameWidth::Get() - width - 200);  // 200 is width of adventure bar on right
		IntAt((int)dlg + 0x1C) = Clamp(0, yPos, H3GameHeight::Get() - height - 48); // 48 is height of resource bar on bottom
	}
	dlg->RMB_Show();

	delete dlg;
	return false;
}

_LHF_(Dlg_CreatureInfo_RmcProc)
{
	H3Msg* msg = reinterpret_cast<H3Msg*>(c->esi);
	int item_id = msg->itemId;
	auto creature_dlg_stack = *reinterpret_cast<H3CombatCreature**>(0x2860280);
	if ((item_id > 220 && item_id < 224
		|| item_id >= 3000 && item_id < 3003)
		&& msg->subtype == eMsgSubtype::RBUTTON_DOWN
		&& creature_dlg_stack->activeSpellNumber)
	{
		ShowStackAvtiveSpells(creature_dlg_stack, true, nullptr);
		msg->itemId = -1;
	}

	return EXEC_DEFAULT;
}

//_LHF_(Dlg_CreatureInfo_Battle_BeforeCreate)
//{
//	creature_dlg_stack = reinterpret_cast<H3CombatCreature*>(c->edi);
//	return EXEC_DEFAULT;
//}



//_LHF_(Dlg_Debug_eax)
//{
//	H3DlgText* dlg_text = reinterpret_cast<H3DlgText*>(c->eax);
//	H3Messagebox(dlg_text->GetH3String());
//	return EXEC_DEFAULT;
//}

constexpr int DLG_SPELLS_BTTN_ID = 4443;
constexpr int WOG_CREATURE_EXP_BUTTON_ID = 4444;
constexpr int DLG_WIDTH = 350;
constexpr int DLG_HEIGHT = 387;

const char* newOkBtn = "iOkay2.def";
const char* newCastBtn = "iMagic.def";
const char* spellListBtn = "iBaff.def";
const char* gCreatureDlgBgName = "gemCrIBg.pcx";
const char* gCreatureDlgHintBarName = "gemCrVar.pcx";
const char* n_DlgExpMon = "DlgCrExp.def";
const char* n_SkillSlotPcx = "skillslt.pcx";

#define WOG_STACK_EXPERIENCE_ON *reinterpret_cast<bool*>(0x02772730)
#include <string>
using namespace h3;
BOOL ShowStackAvtiveSpells(H3CombatCreature* stack, bool isRMC, H3DlgItem* clickedItem);
void Dlg_CreatureInfo_HooksInit(PatcherInstance* pi);

BOOL main_isRMC = false;
BOOL DrawCreatureSkillsList(int firstSkillIndex = 0);
//std::map<std::string, std::string> names;
BOOL ParseText(LPCSTR text)
{


	std::string str(text);
	auto result = std::find_if(str.begin(), str.end(), [](char ch) { return ch != ' '; });

	


	if (result != str.end())
	{

	}



	return false;


}
CreatureDlgHandler::CreatureDlgHandler(H3CreatureInfoDlg* dlg, H3CombatCreature* stack, H3Army* army, int armySlotIndex) :
	dlg(dlg), stack(stack), army(army), armySlotIndex(armySlotIndex), wogStackExperience(WOG_STACK_EXPERIENCE_ON)
{
	if (dlg)
	{
		//dlg->AddItem(H3DlgDef::Create(220, 220, "iokay32.def"),false);
		AlignItems();
		if (wogStackExperience && dlg->GetDefButton(30722))
			AddExperienceButton();
		if (this->stack != nullptr)
			AddSpellEfects();
	}
}


H3CreatureInfoDlg* __stdcall H3CreatureInfoDlg_BattleCtor(HiHook* h, H3CreatureInfoDlg* dlg, H3CombatCreature* mon, int x, int y, int z)
{


	y -= 30; // make dlg start higher cause of new size

	if (y < 0)
	{
		if (P_CombatManager->dlg->GetHeight() == 600) // hd mod combat dlg height check
			y = 0; // non dlg changes
		else if (y < -15) // new base y-value
			y = -15;
	}

	x -= 30;



	Era::SetAssocVarIntValue("gem_a", (int)y);
	Era::ExecErmCmd("IF:L^%i(gem_a)^;");

	if (800 - x < DLG_WIDTH)// set battle dlg new xPos limit
		x = 800 - DLG_WIDTH;
	if (x < 0)
		x = 0;
	if (600 - y < DLG_HEIGHT)
		y = 600 - DLG_HEIGHT;



	H3CreatureInfoDlg* result = THISCALL_5(H3CreatureInfoDlg*, h->GetDefaultFunc(), dlg, mon, x, y, z);// , 0, 1);// , y, isLMC);

	CreatureDlgHandler handler(result, *reinterpret_cast<H3CombatCreature**>(0x2860280));
	//creature_dlg_stack = mon;

	//	auto result = FASTCALL_5(H3CreatureInfoDlg*, h->GetDefaultFunc(), dlg, mon, xa, x, y);

	return result;// result;
	//return EXEC_DEFAULT;
}



void __fastcall CreatureDlgSrollbar_Proc(INT32 tickId, H3BaseDlg* dlg)
{

	DrawCreatureSkillsList(tickId);

}


BOOL CreatureDlgHandler::AlignItems()
{

	H3DlgTextPcx* hint = dlg->GetTextPcx(224); //
	if (hint)
		hint->SetY(DLG_HEIGHT - hint->GetHeight() - 7); //set new hint yPos

	H3DlgText* name = dlg->GetText(203); //set new description pos
	if (name)
		name->SetX((DLG_WIDTH - name->GetWidth()) / 2); // align center

	H3DlgDef* morale = dlg->GetDef(219); //set new morale postion
	if (morale)
	{
		morale->SetY(DLG_HEIGHT - morale->GetHeight() - 46);
		morale->SetX(morale->GetX() + 1);
		H3DlgText* _m = dlg->GetText(3006);
		if (_m) // if creature stats text dlg is active
			_m->SetY(morale->GetY() + morale->GetHeight() - _m->GetHeight());
	}
	H3DlgDef* luck = dlg->GetDef(220);//set new luck postion
	if (luck)
	{
		luck->SetY(DLG_HEIGHT - luck->GetHeight() - 46);
		luck->SetX(luck->GetX() + 1);

		H3DlgText* _l = dlg->GetText(3007);
		if (_l)// if creature stats text dlg is active
			_l->SetY(luck->GetY() + luck->GetHeight() - _l->GetHeight());
	}

	H3DlgDefButton* upgrade = dlg->GetDefButton(300);//set new upgrade bttn postion
	if (upgrade)
	{
		upgrade->SetX(233);
		upgrade->SetY(307);
	}

	H3DlgText* description = dlg->GetText(-1); //set new description pos
	if (!description)
		description = dlg->GetText(1);

	if (description)
	{
		LPCSTR text = description->GetH3String().String();
		if (text && libc::strcmpi(text, h3_NullString))
		{
			ParseText(text);


		}
	}



	int x = std::atoi(Era::tr("gem_plugin.combat_dlg.creature_info.description.x"));
	int y = std::atoi(Era::tr("gem_plugin.combat_dlg.creature_info.description.y"));
	int width = std::atoi(Era::tr("gem_plugin.combat_dlg.creature_info.description.width"));
	int height = std::atoi(Era::tr("gem_plugin.combat_dlg.creature_info.description.height"));
	eTextAlignment align = (eTextAlignment)std::atoi(Era::tr("gem_plugin.combat_dlg.creature_info.description.alignment"));
	if (!x)			x = 24;
	if (!y)			y = 189;
	if (!width)			width = 250;
	if (!height)		height = 55;

	if (description)
	{
		description->SetWidth(width); //set new description yPos
		description->SetHeight(height); //set new description yPos
		description->SetX(x); //set new description xPos
		description->SetY(y); //set new description xPos
		description->SetAlignment(align);
	}


	H3DlgCustomButton* creatureCast = dlg->GetCustomButton(301);//set new cast button postion like for faerie dragons
	if (creatureCast) // if creature can cast
	{
		creatureCast->SetX(126); // 
		//creatureCast->SetY(309);
		if (!description && H3CreatureInformation::Get()[dlg->creatureId].description) // create description field
		{
			description = H3DlgText::Create(
				x,
				y,
				width,
				height,
				H3CreatureInformation::Get()[dlg->creatureId].description,
				NH3Dlg::Text::TINY,
				4,
				-1,
				align);
			dlg->AddItem(description);
		}
	}



	// JackSlater block - adds a panel for creature skills
	if (description)
	{
		description->DeActivate();

		if (!Era::IsCommanderId(dlg->creatureId))
		{




			if (CreateCreatureSkillsList())
			{



			}

			if (auto* pcx = H3LoadedPcx16::Load(n_SkillSlotPcx))
			{
				int skill_width = pcx->width;
				int skill_height = pcx->height;

				constexpr int SKILLS_VIEW_LIMIT = 6;
				dlgSkillPcx.clear();
				dlgSkillPcx.reserve(SKILLS_VIEW_LIMIT);

				for (INT32 i = 0; i < SKILLS_VIEW_LIMIT; i++)
				{
					if (H3DlgPcx16* creature_skill_slot = H3DlgPcx16::Create(x + i * skill_width - 4, y - 6, skill_width, skill_height, -1, pcx->GetName()))
					{
						dlg->AddItem(creature_skill_slot);
						dlgSkillPcx.push_back(creature_skill_slot);
					}
				}

				const int creature_skills_number = creatureSkills.size();
				int scroll_height = 0;

				if (creature_skills_number > SKILLS_VIEW_LIMIT)
				{
					// scroll
					scroll_height = 16;
					auto* scroll = H3DlgScrollbar::Create(x - 4, y + skill_height - 6, skill_width * SKILLS_VIEW_LIMIT, scroll_height, 2020, creature_skills_number - dlgSkillPcx.size() + 1, CreatureDlgSrollbar_Proc, false, 1, true);
					dlg->AddItem(scroll);
				}

				// skill desc
				description->SetY(y + scroll_height + skill_height + 2);

				pcx->Dereference();

				DrawCreatureSkillsList();
			}
		}
	}

	return false;
}

BOOL CreatureDlgHandler::AddExperienceButton()
{

	if (dlg)
	{

		if (auto backPcx8 = dlg->GetH3DlgItem(200))
		{
			const int boxW = 200;

			const int boxX = backPcx8->GetX() - boxW;
			const int boxH = 100;
			H3DlgPcx16* boxFrame = H3DlgPcx16::Create(boxX, backPcx8->GetY(),nullptr);
			boxFrame->SetWidth(boxW);
			boxFrame->SetHeight(boxH);
			H3LoadedPcx16* boxPcx = H3LoadedPcx16::Create(boxW, boxH);
			boxPcx->BackgroundRegion(0, 0, boxW, boxH, false);
			boxPcx->FrameRegion(0, 0, boxW, boxH, false,0,false);
			//boxPcx->BevelArea(1, 1, boxW - 2, boxH - 2);
			boxPcx->DarkenArea(1, 1, boxW - 2, boxH - 2, 50);
			boxFrame->SetPcx(boxPcx);
			dlg->AddItem(boxFrame);


			//if (auto pcx = boxFrame->GetPcx())
			//{
			//	pcx->Destroy();
			//	boxFrame->SetPcx(nullptr);
			//}

		}
	}

	bool isNPC = !(dlg->creatureId < 174 || dlg->creatureId > 191);
	if (!isNPC || stack != nullptr)
	{
		constexpr int x_pos = 180;
		constexpr int y_pos = 307;

		H3DlgPcx* frame = H3DlgPcx::Create(x_pos - 1, y_pos - 1, -1, "box46x32.pcx");
		dlg->AddItem(frame);


		H3DlgDefButton* bttn = H3DlgDefButton::Create(x_pos, y_pos, WOG_CREATURE_EXP_BUTTON_ID, "CrExpBut.def", 0, 1, false, h3::eVKey::H3VK_E);
		H3String hint = isNPC ? Era::tr("gem_plugin.combat_dlg.creature_info.npc_hint") : Era::tr("gem_plugin.combat_dlg.creature_info.stack_exp_hint");
		bttn->SetHints(hint.String(), h3_NullString, true);
		dlg->AddItem(bttn);
	}

	return false;
}

BOOL CreatureDlgHandler::AddSpellEfects()
{
	const int xPos = 182;
	const int yPos = 307;

	H3Vector<INT32> active_spells(stack->activeSpellNumber);
	int counter = 0;


	if (stack->activeSpellNumber)
	{
		int arr_size = sizeof(stack->activeSpellDuration) / sizeof(INT32);

		for (INT32 i = 0; i < arr_size; ++i)
		{
			if (stack->activeSpellDuration[i])
				active_spells[counter++] = i;
		}
	}
	//	DebugInt(stack->activeSpellNumber);
	bool needToExpnd = stack->activeSpellNumber > 6;
	int spellsToShow = needToExpnd ? 5 : stack->activeSpellNumber;

	//int x = 283
	H3DlgDef* spellDef;
	H3DlgText* durTextItem;
	for (INT32 i = 0; i < spellsToShow; i++)
	{
		int yPos = 42 * i + 47;
		int defId = 1000 + i;
		spellDef = H3DlgDef::Create(283, yPos, defId, "spellint.def", active_spells[i] + 1);

		H3String hint = H3GeneralText::Get()->GetText(612);
		H3String spellName = H3Spell::Get()[active_spells[i]].name;
		H3String spellDesc = H3Spell::Get()[active_spells[i]].description[0];
		if (spellDesc == h3_NullString)
			spellDesc = spellName;
		switch (active_spells[i])
		{
		case h3::eSpell::BIND:
			sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(681), spellName.String(), H3GeneralText::Get()->GetText(682));
			break;
		case h3::eSpell::BERSERK:
			sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(681), spellName.String(), H3GeneralText::Get()->GetText(683));
			break;
		case h3::eSpell::DISRUPTING_RAY:
			sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(681), spellName.String(), H3GeneralText::Get()->GetText(684));
			break;
		default:
			sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(612), spellName.String(), stack->activeSpellDuration[active_spells[i]]);
			break;
		}

		spellDef->SetHints(h3_TextBuffer, spellDesc.String(), true);
		//spellDef->SetHint(H3Spell::Get()[active_spells[i]].description[0]);
		dlg->AddItem(spellDef);

		if (stack->activeSpellDuration[active_spells[i]] // if stack has this spell active
			&& active_spells[i] != NH3Spells::eSpell::BERSERK // and not permanent effect
			&& active_spells[i] != NH3Spells::eSpell::DISRUPTING_RAY
			&& active_spells[i] != NH3Spells::eSpell::BIND)
		{
			H3String duration = "";
			duration.Append("x").Append(stack->activeSpellDuration[active_spells[i]]);
			durTextItem = H3DlgText::Create(spellDef->GetX() + spellDef->GetWidth() - 24,
				spellDef->GetY() + spellDef->GetHeight() - 12,
				24, //width
				12, // heigth
				duration.String(),
				NH3Dlg::Text::TINY,
				1,
				0,
				eTextAlignment::BOTTOM_RIGHT);
			durTextItem->SetHints(h3_TextBuffer, spellDesc.String(), true);
			dlg->AddItem(durTextItem);
		}


		if (i == 4 && needToExpnd)
		{
			H3DlgDefButton* dlgCallBttn = H3DlgDefButton::Create(283, yPos + 42, DLG_SPELLS_BTTN_ID, spellListBtn, 0, 1, false, eVKey::H3VK_S);
			dlgCallBttn->SetHints(Era::tr("gem_plugin.combat_dlg.creature_info.spell_list_hint"), h3_NullString, true);
			dlg->AddItem(dlgCallBttn);
		}
	}

	return false;
}

CreatureDlgHandler::~CreatureDlgHandler()
{
	//	creatureSkills.clear();

}


int __stdcall H3CreatureInfoDlg_Proc(HiHook* hook, H3CreatureInfoDlg* dlg, H3Msg* msg)
{
	int lastHoverdItemId = *(int*)0x68C6B0; // needed for stting hints at button
	if ((lastHoverdItemId >= 1000 && lastHoverdItemId <= 1005
		|| lastHoverdItemId == WOG_CREATURE_EXP_BUTTON_ID
		|| lastHoverdItemId == DLG_SPELLS_BTTN_ID)
		&& msg->command == eMsgCommand::MOUSE_OVER)
	{
		H3DlgTextPcx* hint = dlg->GetTextPcx(224);

		if (hint)
			hint->SetText(dlg->GetH3DlgItem(lastHoverdItemId)->GetHint());
	}

	if (msg->command == eMsgCommand::MOUSE_BUTTON
		&& msg->subtype != h3::eMsgSubtype::LBUTTON_DOWN)
	{
		main_isRMC = msg->subtype == h3::eMsgSubtype::RBUTTON_DOWN ? true : false;

		if (msg->itemId == WOG_CREATURE_EXP_BUTTON_ID) // if mouse ckick then call exp dlg
			THISCALL_0(signed int, 0x7645BB); // call wog creature dlg
		else if (msg->itemId == DLG_SPELLS_BTTN_ID) // if clicked at spell list bttn
		{
			auto creature_dlg_stack = *reinterpret_cast<H3CombatCreature**>(0x2860280);
			ShowStackAvtiveSpells(creature_dlg_stack, main_isRMC, dlg->GetH3DlgItem(DLG_SPELLS_BTTN_ID)); // call dlg with cliecked item

		}
	}

	return THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
}






_LHF_(gem_Dlg_CreatureInfo_AddCreatureCastButton)
{
	c->Push(306); //set yPos for cast_bttn frame
	c->Push(125);//set xPos for cast_bttn frame
	c->return_address = h->GetAddress() + 0x7;
	
	return NO_EXEC_DEFAULT;
}

_LHF_(gem_Dlg_CreatureInfo_AddCreatureUpradeButton)
{
	c->Push(306); //set yPos for upgrade frame
	c->Push(232);//set xPos for upgrade frame
	c->return_address = h->GetAddress() + 0x7;
	return NO_EXEC_DEFAULT;
}

// combat cr expo stack 0071B748



H3CreatureInfoDlg* __stdcall H3CreatureInfoDlg_NotBattleCtor(HiHook* h,
	H3CreatureInfoDlg* dlg,
	H3Army* army,
	int slotId,
	const DWORD hero,
	const H3Town* town,
	int x,
	int y,
	const int monGrade,
	const int a9,
	const int a10,
	const int a11
)
{

	auto result = THISCALL_11(H3CreatureInfoDlg*, h->GetDefaultFunc(), dlg, army, slotId, hero, town, x, y, monGrade, a9, a10, a11);

	if (dlg->GetY() + DLG_HEIGHT + 145 > P_AdventureMgr->dlg->GetHeight())
	{
		IntAt((int)dlg + 0x1C) = P_AdventureMgr->dlg->GetHeight() - DLG_HEIGHT - 145;
	}

	CreatureDlgHandler handler(result, nullptr, army, slotId);

	return result;
	//	return EXEC_DEFAULT;
}


H3CreatureInfoDlg* __stdcall H3CreatureInfoDlg_BuyCtor(HiHook* h,
	H3CreatureInfoDlg* dlg,
	const eCreature monId,
	int x,
	int y,
	const DWORD a5)
{
	auto result = THISCALL_5(H3CreatureInfoDlg*, h->GetDefaultFunc(), dlg, monId, x, y, a5);

	CreatureDlgHandler handler(result);
	return result;
}


_LHF_(Wnd_BeforeExpoDlgShow)
{
	if (main_isRMC)
	{
		H3Dlg* wndDlg = (H3Dlg*)c->esi;
		wndDlg->RMB_Show();
		c->return_address = h->GetAddress() + 21;
		main_isRMC = false;
		return NO_EXEC_DEFAULT;
	}

	return EXEC_DEFAULT;
}

_LHF_(Before_WndNPC_DLG)
{
	if (main_isRMC)
	{
		H3Dlg* wndDlg = (H3Dlg*)c->esi;
		wndDlg->RMB_Show();
		c->return_address = h->GetAddress() + 0x20;
		main_isRMC = false;
		return NO_EXEC_DEFAULT;
	}

	return EXEC_DEFAULT;
}




std::vector<H3DlgPcx16*> CreatureDlgHandler::dlgSkillPcx = {};
std::vector<CreatureSkill> CreatureDlgHandler::creatureSkills = {};


EXTERN_C __declspec(dllexport) void AddExternalCreatureSkill(const char* name, const char* description, const char* pcx16Name)
{
	//const int expId;




}


BOOL CreatureDlgHandler::CreateCreatureSkillsList()
{

	int montype = dlg->creatureId;

	// make dlg creature active for wog creature expo bonuses

	int expoCreatureType = CDECL_1(int, 0x716C8E, stack);

	// call fucntion that writes experience bonus into buffer
	CDECL_2(void, 0x728120, expoCreatureType, montype);
	//CrExpBonLine* creatureWogExpBonuses = reinterpret_cast<CrExpBonLine*>(0x0847D98);
	DWORD crExpo = 0;
	if (stack)
	{
		crExpo = CDECL_1(DWORD, 0x728110, expoCreatureType);
	}
	else if (army && armySlotIndex >= 0 && armySlotIndex <= 6)
	{
		DWORD expType = 0;
		DWORD expData = 0;

		// try to find this creature type
		if (CDECL_4(int, 0x71A1B7, army, armySlotIndex, &expType, &expData))
		{
			crExpo = CDECL_2(DWORD, 0x718617, expType, expData);
		}
	}



	int experience = 0;
	if (crExpo)
	{
		experience = IntAt(crExpo);
	}

	CDECL_5(void, 0x71EF2B, montype, dlg->numberCreatures, experience, crExpo, 0);
	_DlgCreatureExpoInfo_* dlgCreatureExpoInfo = reinterpret_cast<_DlgCreatureExpoInfo_*>(0x845880);


	creatureSkills.clear();
	UINT32 picsCount = dlgCreatureExpoInfo->IcoPropertiesCount;
	creatureSkills.reserve(picsCount * 2);

	if (auto* skillsDef = H3LoadedDef::Load(n_DlgExpMon))
	{

		if (auto* skillSlt = H3LoadedPcx16::Load(n_SkillSlotPcx))
		{


			H3LoadedPcx16* tempPcx = H3LoadedPcx16::Create(skillsDef->widthDEF, skillsDef->heightDEF);
			for (size_t i = 0; i < picsCount; i++)
			{
				int skillPicIndex = (int)dlgCreatureExpoInfo->IcoProperties[i];
				H3LoadedPcx16* skillPic = H3LoadedPcx16::Create(skillSlt->width, skillSlt->height);

				memcpy(skillPic->buffer, skillSlt->buffer, skillPic->buffSize);
				memset(skillPic->buffer, 0, skillPic->buffSize);

				//	skillsDef->DrawToPcx16(0, 159, skillPic, 0, 0);

				skillsDef->DrawToPcx16(0, skillPicIndex, tempPcx, 0, 0);

				resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(skillPic, tempPcx, tempPcx->width, tempPcx->height, 2, 2, skillPic->width - 4, skillPic->height - 4);

				creatureSkills.emplace_back(*new CreatureSkill{ 0,0,0,0,skillPic });
			}
			tempPcx->Destroy();
			//H3Messagebox::RMB(Era::IntToStr(type).c_str());
			for (size_t k = 0; k < 20; k++)
			{
				CrExpBonLine* line = reinterpret_cast<CrExpBonLine*>(0x847D98 + k * 17);
			}

			skillSlt->Dereference();
		}



		skillsDef->Dereference();

	}

	// JackSlater block - trying to access an army stack and exp
//	H3Hero* hero = reinterpret_cast <H3Hero*>(c->esi);
//int army_slot = IntAt(c->ebp + 0xC);
//DWORD creature_exp_struct = CDECL_2(DWORD, 0x718617, 1, hero->id + (army_slot << 16)); // it works
//H3Messagebox::RMB(Era::IntToStr(hero->id).c_str());
//H3Messagebox::RMB(Era::IntToStr(army_slot).c_str());




	return 0;
}

BOOL DrawCreatureSkillsList(int firstSkillIndex)
{
	auto& skillsPictures = CreatureDlgHandler::dlgSkillPcx;
	auto& skills = CreatureDlgHandler::creatureSkills;
	int picNum = skillsPictures.size() > skills.size() ? skills.size() : skillsPictures.size();

	for (int i = 0; i < picNum; i++)
	{

		int skillIndex = firstSkillIndex + i;
		skillsPictures[i]->SetPcx(skills[skillIndex].pcx16);
		skillsPictures[i]->Refresh();

	}
	//Era::ExecErmCmd("IF:L^^");
	//skillsPictures[0]->GetParent()->Redraw();
	return false;
}





CreatureSkill::~CreatureSkill()
{
	if (pcx16)
	{
		//pcx16->Destroy();
	//	pcx16 = nullptr;
	}
}
void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance* pi)
{
	pi->WriteLoHook(0x5F4C5D, Dlg_CreatureInfo_RmcProc);

}

void Dlg_CreatureInfo_HooksInit(PatcherInstance* pi)
{

	pi->WriteHiHook(0x5F45B0, THISCALL_, H3CreatureInfoDlg_BuyCtor); //BuyCreatureInfoDlg
	pi->WriteHiHook(0x5F3EF0, THISCALL_, H3CreatureInfoDlg_NotBattleCtor); //BattleCreatureInfo
	pi->WriteHiHook(0x764B38, THISCALL_, H3CreatureInfoDlg_BattleCtor); //BattleCreatureInfo

	pi->WriteHiHook(0x5F4C00, THISCALL_, H3CreatureInfoDlg_Proc); // dlg proc



	H3DLL wndPlugin = h3::H3DLL::H3DLL("wog native dialogs.era");
	if (wndPlugin.dataSize)
	{
		int pluginHookAddress = wndPlugin.NeedleSearch<6>({ 0x0F,0x8C, 0xC8,0xFE,0xFF,0xFF }, 6); // Thanks so much to RK for lesson
		if (pluginHookAddress)
			_PI->WriteLoHook(pluginHookAddress, Wnd_BeforeExpoDlgShow);
		pluginHookAddress = wndPlugin.NeedleSearch<3>({ 0x3D,0x68,0x02 }, 15);

		if (pluginHookAddress)
			_PI->WriteLoHook(pluginHookAddress, Before_WndNPC_DLG);
	}

	pi->WriteHexPatch(0x5F3C9E, "9090909090");//skip default adding spells view

	//pi->WriteWord(0x5F3E10, 0x9090); //allow Faerie Dragon description at LMC in combat creature info dlg -- failed, so added it in handler

	//pi->WriteByte(0x5F3E3A +1, 0x1); // set desciption item id in combat creature info dlg


	//All Dlg's stuff

	// set buttons and frames new position
	constexpr UINT8 defWidth = 46;
	constexpr UINT8 defHeight = 32;
	// IOKAY button
	constexpr INT xOK = DLG_WIDTH - defWidth - 18;
	constexpr INT yOk = DLG_HEIGHT - defHeight - 48;

	pi->WriteDword(0x5F6CBA + 1, newOkBtn); // iOkay now created from my def lol
	pi->WriteByte(0x5F6CC6 + 1, defWidth); // set new width for iokay def
	pi->WriteByte(0x5F6CC4 + 1, defHeight); // set new height for iokay def
	pi->WriteDword(0x5F6CCD + 1, xOK); // set new xPos for iokay def
	pi->WriteDword(0x5F6CC8 + 1, yOk); // set new yPos for iokay def 

	// IOKAY button frame
	pi->WriteDword(0x5F6C5B + 1, 0x68C6B4); // frame for "IOKAY" now is same as for "dismiss" box46x32.pcx
	pi->WriteByte(0x5F6C67 + 1, 48); // set frame new draw limits (width)
	pi->WriteByte(0x5F6C65 + 1, 34); // set frame new draw limits (height)
	pi->WriteDword(0x5F6C6E + 1, xOK - 1); // set new xPos for frame
	pi->WriteDword(0x5F6C69 + 1, yOk - 1); // set new yPos for frame

	// dismiss creature bttn
	pi->WriteByte(0x5F71A6 + 1, 127); // set new xPos for bttn
	pi->WriteDword(0x5F71A1 + 1, yOk); // set new yPos for bttn

	// dismiss creature bttn frame
	pi->WriteByte(0x5F714D + 1, 126); // set new xPos for frame
	pi->WriteDword(0x5F7148 + 1, yOk - 1); // set new yPos for frame


	pi->WriteByte(0x5F4466, 0x51); // push ECX ==1 instead EDI == -1 to set as description itemId  in non combat creature info dlg
	//allow creature description at LMC in non combat creature info dlg 
	pi->WriteWord(0x5F4434, 0x9090); //skip upgrade check
	pi->WriteWord(0x5F4439, 0x9090); //skip LMC check

	//change frame pos for cast in combat or upgrade dlg
	pi->WriteLoHook(0x5F6ED8, gem_Dlg_CreatureInfo_AddCreatureUpradeButton); // uprade button frame for non Battle
	pi->WriteLoHook(0x5F3D9E, gem_Dlg_CreatureInfo_AddCreatureCastButton); //spell cast button frame for Battle

	//hire creature dlg
	pi->WriteDword(0x5F45D8 + 1, DLG_HEIGHT); // set dlg height
	pi->WriteDword(0x5F45DD + 1, DLG_WIDTH); // set dlg width	
	pi->WriteDword(0x5F4712 + 1, DLG_HEIGHT); // set bg_pcx height
	pi->WriteDword(0x5F4717 + 1, DLG_WIDTH); // set bg_pcx width
	pi->WriteDword(0x5F48E9 + 1, 336); // set hint pcx width
	pi->WriteDword(0x5F4708 + 1, gCreatureDlgBgName); // set new BG name
	pi->WriteDword(0x5F48DB + 1, gCreatureDlgHintBarName); // set new hintBAr name

	//non combat dlg
	pi->WriteDword(0x5F3F1B + 1, DLG_HEIGHT); // set non battle dlg height
	pi->WriteDword(0x5F3F20 + 1, DLG_WIDTH + 145); // set non battle dlg width
	pi->WriteDword(0x5F406B + 1, DLG_HEIGHT); // set non battle bg_pcx height
	pi->WriteDword(0x5F4070 + 1, DLG_WIDTH); // set non battle bg_pcx width
	pi->WriteDword(0x5F44CE + 1, 336); // set hint pcx width
	pi->WriteDword(0x5F4061 + 1, gCreatureDlgBgName); // set new BG name
	pi->WriteDword(0x5F44C0 + 1, gCreatureDlgHintBarName); // set new hintBAr name


	//pi->WriteByte(0x5F4880 +1, 0x1); // set desciption item id in buy creature info dlg

	// combat creature dlg
	pi->WriteDword(0x5F3728 + 1, DLG_HEIGHT); // set battle dlg height
	pi->WriteDword(0x5F372D + 1, DLG_WIDTH); // set battle dlg width	
	pi->WriteDword(0x5F38CF + 1, DLG_HEIGHT); // set battle bg_pcx height
	pi->WriteDword(0x5F38D4 + 1, DLG_WIDTH); // set battle bg_pcx width
	pi->WriteDword(0x5F3DE6 + 1, newCastBtn); // castButton is now from resources
	pi->WriteByte(0x5F3DF2 + 1, defWidth); // set new width for castButton def
	pi->WriteByte(0x5F3DF0 + 1, defHeight); // set new height for castButton def
	pi->WriteDword(0x5F3DF4 + 1, yOk); // set new yPos for castButton def
	pi->WriteDword(0x5F3CDF + 1, 336); // set hint pcx width
	pi->WriteDword(0x5F38C5 + 1, gCreatureDlgBgName); // set new BG name
	pi->WriteDword(0x5F3CD1 + 1, gCreatureDlgHintBarName); // set new hintBAr name

}


