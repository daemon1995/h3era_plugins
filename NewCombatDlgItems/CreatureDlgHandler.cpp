#include "CreatureDlgHandler.h"

using namespace h3;
bool ShowStackAvtiveSpells(H3CombatCreature* stack, bool isRMC);


bool main_isRMC = false;
H3CombatCreature* currentStack = nullptr;

int __stdcall gem_Dlg_CreatureInfo_BattleCtor(HiHook* hook, H3CreatureInfoDlg* dlg, H3CombatCreature* mon, int x, int y, bool isLMC)
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

	if (800 - x < DLG_WIDTH)// set battle dlg new xPos limit
		x = 800 - DLG_WIDTH;
	if (x < 0)
		x = 0;
	if (600 - y < DLG_HEIGHT)
		y = 600 - DLG_HEIGHT;

	currentStack = mon;
	return THISCALL_5(int, hook->GetDefaultFunc(), dlg, mon, x, y, isLMC);
}


int __stdcall gem_Dlg_CreatureInfo_Proc(HiHook* hook, H3CreatureInfoDlg* dlg, H3Msg* msg)
{
	int itemId = *(int*)0x68C6B0;
	if ((itemId >= 1000 && itemId <= 1005
		|| itemId == WOG_CREATURE_EXP_BUTTON_ID
		|| itemId == DLG_SPELLS_BTTN_ID)
		&& msg->command == eMsgCommand::MOUSE_OVER)
	{
		H3DlgTextPcx* hint = dlg->GetTextPcx(224);

		if (hint)
			hint->SetText(dlg->GetH3DlgItem(itemId)->GetHint());
		//	
	}

	if (msg->itemId == WOG_CREATURE_EXP_BUTTON_ID
		&& msg->command == eMsgCommand::MOUSE_BUTTON) // if mouse ckick then call exp dlg
	{
		if (msg->subtype == h3::eMsgSubtype::LBUTTON_CLICK) // 
			main_isRMC = false;
		else if (msg->subtype == h3::eMsgSubtype::RBUTTON_DOWN)
		{
			main_isRMC = true;
			msg->itemId = -1;
		}
		THISCALL_0(signed int, 0x7645BB); // call wog creature dlg
	}

	return THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
}



bool CreatureDlgHandler::AlignItems()
{

	H3DlgTextPcx* hint = dlg->GetTextPcx(224); //
	if (hint)
	{
		//hint->d
		hint->SetY(DLG_HEIGHT - hint->GetHeight() - 7); //set new hint yPos
		//hint->SetX((DLG_WIDTH - hint->GetWidth()) / 2); // align center
	}

	H3DlgText* name = dlg->GetText(203); //set new description pos
	if (name)
		name->SetX((DLG_WIDTH - name->GetWidth()) / 2); // align center

	H3DlgText* description = dlg->GetText(-1); //set new description pos
	if (description)
		description->SetY(189); //set new description yPos
	else
	{
		description = dlg->GetText(1);
		if (description)
			description->SetY(185);
	}


	H3DlgDef* morale = dlg->GetDef(219); //set new morale postion
	if (morale)
	{
		morale->SetY(DLG_HEIGHT - morale->GetHeight() - 48);
		H3DlgText* _m = dlg->GetText(3006);
		if (_m) // if creature stats text dlg is active
			_m->SetY(morale->GetY() + morale->GetHeight() - _m->GetHeight());
	}
	H3DlgDef* luck = dlg->GetDef(220);//set new luck postion
	if (luck)
	{
		luck->SetY(DLG_HEIGHT - luck->GetHeight() - 48);
		H3DlgText* _l = dlg->GetText(3007);
		if (_l)// if creature stats text dlg is active
			_l->SetY(luck->GetY() + luck->GetHeight() - _l->GetHeight());
	}

	H3DlgDefButton* upgrade = dlg->GetDefButton(300);//set new upgrade bttn postion
	if (upgrade)
	{
		upgrade->SetX(180);
		upgrade->SetY(307);
	}

	H3DlgCustomButton* creatureCast = dlg->GetCustomButton(301);//set new cast button postion like for faerie dragons
	if (creatureCast) // if creature can cast
	{
		creatureCast->SetX(180); // 
		//creatureCast->SetY(309);
		if (!description && H3CreatureInformation::Get()[dlg->creatureId].description) // create description field
		{
			description = H3DlgText::Create(22,
				189,
				255,
				55,
				H3CreatureInformation::Get()[dlg->creatureId].description,
				NH3Dlg::Text::TINY,
				4,
				-1,
				eTextAlignment::HLEFT);
			dlg->AddItem(description);
		}
	}

	return false;
}

bool CreatureDlgHandler::AddExperienceButton()
{
	bool isNPC = !(dlg->creatureId < 174 || dlg->creatureId > 191);
	if (!isNPC || stack != nullptr)
	{
		constexpr int x_pos = 233;
		constexpr int y_pos = 307;

		H3DlgPcx* frame = H3DlgPcx::Create(x_pos - 1, y_pos - 1, -1, "box46x32.pcx");
		dlg->AddItem(frame);


		H3DlgDefButton* bttn = H3DlgDefButton::Create(x_pos, y_pos, WOG_CREATURE_EXP_BUTTON_ID, "CrExpBut.def", 0, 1, false, h3::eVKey::H3VK_E);


		H3String hint = isNPC ? Era::tr("gem_plugin.combat_dlg.creature_info.npc_hint") : Era::tr("gem_plugin.combat_dlg.creature_info.stack_exp_hint");
		//H3Messagebox(hint.String());
		bttn->SetHints(hint.String(), h3_NullString, true);
		dlg->AddItem(bttn);
	}

	return false;
}

int __fastcall CallSpellsDlg(H3Msg* msg)
{
	ShowStackAvtiveSpells(creature_dlg_stack, true);
	return false;
}

bool CreatureDlgHandler::AddSpellEfects()
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
	for (size_t i = 0; i < spellsToShow; i++)
	{
		int yPos = 42 * i + 47;
		int defId = 1000 + i;
		spellDef = H3DlgDef::Create(283, yPos, defId, "spellint.def", active_spells[i] + 1);

		H3String hint = H3GeneralText::Get()->GetText(612);
		H3String spellName = H3Spell::Get()[active_spells[i]].name;
		H3String spellDesc = H3Spell::Get()[active_spells[i]].description[0];

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
			H3DlgButton_proc callback = CallSpellsDlg;
			H3DlgCustomButton* dlgCallBttn = H3DlgCustomButton::Create(283, yPos + 42, DLG_SPELLS_BTTN_ID, spellListBtn, callback, 0, 1);
			dlgCallBttn->AddHotkey(h3::eVKey::H3VK_S);
			dlgCallBttn->SetHints(Era::tr("gem_plugin.combat_dlg.creature_info.spell_list_hint"), h3_NullString, true);
			dlg->AddItem(dlgCallBttn);
		}
	}

	return false;
}

//bool CreatureDlgHandler::AddCommanderSkills()
//{
//	return false;
//}




_LHF_(gem_Dlg_CreatureInfo_AddUpradeButton)
{
	c->Push(306); //set yPos for upgrade/cast_bttn frame
	c->Push(179);//set xPos for upgrade/cast_bttn frame
	c->return_address = h->GetAddress() + 0x7;
	return NO_EXEC_DEFAULT;
}

_LHF_(gem_Dlg_CreatureInfo_DescriptionCreate)
{

	c->Push(std::atoi(Era::tr("gem_plugin.combat_dlg.creature_info.height")));
	c->Push(255);
	c->return_address = h->GetAddress() + 0x7;
	return NO_EXEC_DEFAULT;
}


_LHF_(gem_Dlg_CreatureInfo_notBattle_Created)
{
	CreatureDlgHandler handler((H3CreatureInfoDlg*)c->eax, nullptr);
	return EXEC_DEFAULT;
}
_LHF_(gem_Dlg_CreatureInfo_BuyCreature)
{
	CreatureDlgHandler handler((H3CreatureInfoDlg*)c->eax, nullptr);
	return EXEC_DEFAULT;
}
_LHF_(gem_Dlg_BatttleCreatureInfo_Create)
{
	CreatureDlgHandler handler((H3CreatureInfoDlg*)c->eax, currentStack);
	return EXEC_DEFAULT;
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


void Dlg_CreatureInfo_HooksInit(PatcherInstance* pi)
{
	pi->WriteLoHook(0x5F4961, gem_Dlg_CreatureInfo_BuyCreature); //BuyCreatureInfoDlg
	pi->WriteLoHook(0x4C6B5B, gem_Dlg_CreatureInfo_notBattle_Created); //not BattleCreatureInfo
	pi->WriteLoHook(0x5F3EA5, gem_Dlg_BatttleCreatureInfo_Create); //BattleCreatureInfo


	pi->WriteHiHook(0x5F3700, SPLICE_, EXTENDED_, THISCALL_, gem_Dlg_CreatureInfo_BattleCtor);
	pi->WriteHiHook(0x5F4C00, SPLICE_, EXTENDED_, THISCALL_, gem_Dlg_CreatureInfo_Proc); // dlg proc
//	pi->WriteLoHook(0x5F5327, gem_Dlg_CreatureInfo_SetHint); //BattleCreatureInfo



	//_before description field create

	pi->WriteLoHook(0x5F488A, gem_Dlg_CreatureInfo_DescriptionCreate); //Buy dlg 
	pi->WriteLoHook(0x5F3E44, gem_Dlg_CreatureInfo_DescriptionCreate); //Combat Dlg 
	pi->WriteLoHook(0x5F446F, gem_Dlg_CreatureInfo_DescriptionCreate); // non combat dlg

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
	pi->WriteLoHook(0x5F6ED8, gem_Dlg_CreatureInfo_AddUpradeButton); // uprade button frame for non Battle
	pi->WriteLoHook(0x5F3D9E, gem_Dlg_CreatureInfo_AddUpradeButton); //spell cast button frame for Battle

	//hire creature dlg
	pi->WriteDword(0x5F45D8 + 1, DLG_HEIGHT); // set dlg height
	pi->WriteDword(0x5F45DD + 1, DLG_WIDTH); // set dlg width	
	pi->WriteDword(0x5F4712 + 1, DLG_HEIGHT); // set bg_pcx height
	pi->WriteDword(0x5F4717 + 1, DLG_WIDTH); // set bg_pcx width
	pi->WriteDword(0x5F48E9 + 1, 336); // set hint pcx width

	//non combat dlg
	pi->WriteDword(0x5F3F1B + 1, DLG_HEIGHT); // set non battle dlg height
	pi->WriteDword(0x5F3F20 + 1, DLG_WIDTH); // set non battle dlg width
	pi->WriteDword(0x5F406B + 1, DLG_HEIGHT); // set non battle bg_pcx height
	pi->WriteDword(0x5F4070 + 1, DLG_WIDTH); // set non battle bg_pcx width
	pi->WriteDword(0x5F44CE + 1, 336); // set hint pcx width


	//pi->WriteByte(0x5F4880 +1, 0x1); // set desciption item id in buy creature info dlg

	// combat creature dlg
	pi->WriteDword(0x5F3728 + 1, DLG_HEIGHT); // set non battle dlg height
	pi->WriteDword(0x5F372D + 1, DLG_WIDTH); // set non battle dlg width	
	pi->WriteDword(0x5F38CF + 1, DLG_HEIGHT); // set non battle bg_pcx height
	pi->WriteDword(0x5F38D4 + 1, DLG_WIDTH); // set non battle bg_pcx width
	pi->WriteDword(0x5F3DE6 + 1, newCastBtn); // castButton is now from resources
	pi->WriteByte(0x5F3DF2 + 1, defWidth); // set new width for castButton def
	pi->WriteByte(0x5F3DF0 + 1, defHeight); // set new height for castButton def
	pi->WriteDword(0x5F3DF4 + 1, yOk); // set new yPos for castButton def
	pi->WriteDword(0x5F3CDF + 1, 336); // set hint pcx width


}

