#include "battle_Dlg_MonPreview.h"


using namespace h3;

//int stack_dlg_ = 0;
//H3LoadedPcx16* pcx_arr[79];


H3Vector<H3LoadedPcx16*> smaller_spellint(0);
H3LoadedPcx* creature_stat_bg[2];

bool res_allocated[3] = { 0,0,0 };

_LHF_(Battle_Dlg_Create)
{
	//H3Ini* myIni = new H3Ini;


	H3CombatMonsterPanel* dlg_panel = (H3CombatMonsterPanel*)c->eax;

	if (dlg_panel && dlg_panel->GetHeight() > 200)
	{
		H3DlgDef* def;
		H3DlgText* text;

		int x_pos, y_pos;

		// add new modified BG
		H3DlgPcx* _bg = H3DlgPcx::Create(0, 70 + DLG_HEIGHT_ADD, creature_stat_bg[0]->width, creature_stat_bg[0]->height, 777, nullptr);
		static int call_counter = 0;
		bool side = call_counter++ & 1;
		int player_id = P_CombatManager->hero[side] > 0 ? P_CombatManager->heroOwner[side] : P_CombatManager->heroOwner[!side];
		_bg->SetPcx(creature_stat_bg[side]);
		_bg->AdjustColor(player_id);

		dlg_panel->AddItem(_bg);

		for (auto it : dlg_panel->GetItems())
		{
			int id = it->GetID();
			if (id >= 2210 && id <= 2213)
			{
				//x_pos
				if (id % 2 == 0)
					it->Cast<H3DlgText>()->SetAlignment(eTextAlignment::MIDDLE_RIGHT);
				else
					it->SetY(it->GetY() + 40);
			}

		}
		int item_id = 4000;
		for (int i = 4; i < 6; i++)
		{
			x_pos = 9;
			y_pos = 12 * i - 3 + DLG_HEIGHT_ADD;
			H3String temp = "gem_plugin.combat_dlg.";
			temp.Append(i);
			text = H3DlgText::Create(x_pos, y_pos, 60, 12, Era::tr(temp.String()), NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_LEFT);
			dlg_panel->AddItem(text);
		}



		item_id = 4004;
		// add new actions picures && text
		if (res_allocated[1])
		{
			for (int i = 0; i < 4; i++)
			{
				y_pos = 134 + i / 2 * 28;
				x_pos = 9 + (i & 1) * 32;
				if (i == 3)
					def = H3DlgDef::Create(x_pos, y_pos, item_id++, "WDWCint.def", 0);
				else
					def = H3DlgDef::Create(x_pos, y_pos, item_id++, "WDWCint.def", i);
				dlg_panel->AddItem(def);

				text = H3DlgText::Create(x_pos - 2, y_pos + 2, 30, 12, "", NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_RIGHT);
				dlg_panel->AddItem(text);
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				x_pos = 9;
				y_pos = 12 * i + 74 + DLG_HEIGHT_ADD;
				//json_name + i;Era::tr((json_name + i).String())
				H3String temp = "gem_plugin.combat_dlg.";
				temp.Append(i);
				text = H3DlgText::Create(x_pos, y_pos, 30, 12, Era::tr(temp.String()), NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_LEFT);
				dlg_panel->AddItem(text);

				text = H3DlgText::Create(x_pos, y_pos, 60, 12, "", NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_RIGHT);
				dlg_panel->AddItem(text);
			}
			//H3Messagebox("0");

		}
		// add mew moral pictrures && text
		item_id = 4012;
		for (int i = 0; i < 2; i++)
		{
			y_pos = 131 + DLG_HEIGHT_ADD + i * 12;

			H3String def_name = i ? "ilcks.def" : "imrls.def";
			def = H3DlgDef::Create(9, y_pos, item_id++, def_name.String(), 3);
			dlg_panel->AddItem(def);

			text = H3DlgText::Create(9, y_pos, 60, 12, "", NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_RIGHT);
			dlg_panel->AddItem(text);
		}

		int i_width = smaller_spellint[0]->width;
		int i_height = smaller_spellint[0]->height;
		item_id = 4100;
		for (int i = 0; i < 12; i++)
		{
			x_pos = 5 + (i & 1) * 34;
			y_pos = 168 + DLG_HEIGHT_ADD + i / 2 * 19;

			H3DlgPcx16* _pcx = H3DlgPcx16::Create(x_pos, y_pos, i_width, i_height, item_id++, nullptr);
			_pcx->SetPcx(smaller_spellint[0]);
			dlg_panel->AddItem(_pcx);

			//	x_pos = 5 + (i & 1) * 34;
			//	y_pos = 224 + i / 2 * 19;

			text = H3DlgText::Create(x_pos + 10, y_pos + 8, 24, 12, "", NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_RIGHT);
			dlg_panel->AddItem(text);
		}
	}

	return EXEC_DEFAULT;
}


void Hel_SetActionType(H3CombatCreature* stack, H3DlgDef* def, H3DlgText* text)
{

	if (stack->IsWaiting())
		text->SetText(Era::tr("gem_plugin.combat_dlg.wait"));
	else if (stack->IsDefending())
		text->SetText(Era::tr("gem_plugin.combat_dlg.def"));
	else if (stack->IsDone())
		text->SetText(Era::tr("gem_plugin.combat_dlg.done"));
	else
		text->SetText(Era::tr("gem_plugin.combat_dlg.active"));
	return;
}

_LHF_(Battle_Dlg_StackInfo_Show)
{
	if (*(int*)0x698818 == 1)
	{
		//stack_dlg_shown = true;


		H3CombatMonsterPanel* dlg_panel = (H3CombatMonsterPanel*)c->edi;
		if (dlg_panel)
		{
			H3CombatCreature* stack = (H3CombatCreature*)(c->esi);
			H3Vector<INT16> active_spells(stack->activeSpellNumber);
			int counter = 0;
			if (stack->activeSpellNumber)
			{
				int arr_size = sizeof(stack->activeSpellDuration) / sizeof(INT32);

				for (INT8 i = arr_size - 1; i >= 0; --i)
				{
					if (stack->activeSpellDuration[i])
						active_spells[counter++] = i;
				}
			}


			H3DlgDef* def;
			H3DlgText* text;
			int frame_id, luck, morale, spell_id, losses;
			int it_id = 0;
			H3Hero* hero;
			//H3Messagebox();
			H3String str = "";
			for (H3DlgItem* it : dlg_panel->GetItems())
			{
				it_id = it->GetID();


				switch (it_id)
				{
				case 2207:
					hero = dlg_panel == P_CombatManager->dlg->leftMonsterPopup ? P_CombatManager->hero[0] : P_CombatManager->hero[1];

					if (stack->type == eCreature::BALLISTA && hero)
					{
						str.Append((hero->primarySkill[0] + 1) * stack->info.damageLow);
						if (stack->info.damageLow != stack->info.damageHigh) str.Append("-").Append((hero->primarySkill[0] + 1) * stack->info.damageHigh);
						it->Cast<H3DlgText>()->SetText(str.String());
					}
					break;
				case 2209:
					it->Cast<H3DlgText>()->SetText(std::to_string(stack->info.hitPoints).c_str());
					break;
				case 2210:
					it->Cast<H3DlgText>()->SetText(std::to_string(stack->info.hitPoints - stack->healthLost).c_str());
					break;
				case 2212:
					losses = stack->numberAtStart - stack->numberAlive;
					str.Append(losses);
					it->Cast<H3DlgText>()->SetText(str.String());

					break;
				case 4005:
					//def = it->Cast<H3DlgDef>();
					def = nullptr;
					text = it->Cast<H3DlgText>();
					Hel_SetActionType(stack, def, text);
					break;
				case 4007:
					text = it->Cast<H3DlgText>();
					if (stack->retaliations > 200)	text->SetText("99+");
					else	text->SetText(std::to_string(stack->retaliations).c_str());

					break;
				case 4009:
					if (stack->info.flags & 4)	it->Cast<H3DlgText>()->SetText(std::to_string(stack->info.numberShots).c_str());
					else	it->Cast<H3DlgText>()->SetText("-");
					break;

				case 4011:
					it->Cast<H3DlgText>()->SetText(std::to_string(stack->info.spellCharges).c_str());
					break;

				case 4012:
					def = it->Cast<H3DlgDef>();
					morale = stack->morale;
					//if (res_allocated[2])	frame_id = morale > 0 ? 1 : morale < 0 ? 0 : 2;
					//else
					frame_id = std::min(std::max(morale, -3), 3) + 3;

					def->SetFrame(frame_id);
					text = def->GetPreviousItem()->Cast<H3DlgText>();
					text->SetText(std::to_string(morale).c_str());
					break;
				case 4014:
					def = it->Cast<H3DlgDef>();
					luck = stack->luck;
					frame_id = std::min(std::max(luck, -3), 3) + 3;
					def->SetFrame(frame_id);

					text = def->GetPreviousItem()->Cast<H3DlgText>();
					text->SetText(std::to_string(luck).c_str());
					break;
				default:
					if (it_id >= 4100)
					{
						if (it_id & 1)//if text item
						{
							--counter;
							H3String duration = "";
							if (counter >= 0
								&& active_spells[counter] != NH3Spells::eSpell::BERSERK
								&& active_spells[counter] != NH3Spells::eSpell::DISRUPTING_RAY)
								duration.Append("x").Append(stack->activeSpellDuration[active_spells[counter]]);
							it->Cast<H3DlgText>()->SetText(duration.String());
						}
						else// if picture item
						{
							spell_id = counter > 0 ? active_spells[counter - 1] + 1 : 0;
							it->Cast<H3DlgPcx16>()->SetPcx(smaller_spellint[spell_id]);
						}
					}

					break;
				}
			}
		}
	}
	return EXEC_DEFAULT;
}


void CreateSmallerPics(H3LoadedDef* src, H3Vector<H3LoadedPcx16*>& dst_vec, INT32 count, int w, int h)
{
	//

	dst_vec.Resize(count);

	H3Palette565* pal = src->palette565;

	constexpr int scale = 2; // Scale of the pic
	int i_width = w / scale;
	int i_height = h / scale;

	H3LoadedPcx16* buf = H3LoadedPcx16::Create(w, h);
	int color_type = o_BPP;
	int it_height = i_height - (color_type != 32);
	for (INT i = 0; i < count; i++)
	{
		dst_vec[i] = H3LoadedPcx16::Create(i_width, i_height);

		src->GetGroupFrame(0, i)->DrawToPcx16(0, 0, w, h, buf, 0, 0, pal);

		if (dst_vec[i] != nullptr)
		{
			int pix_atX = -scale / 2, pix_atY = -scale / 2;

			for (int k = 0; k < i_width; k++)
			{
				pix_atX += scale;

				for (int j = 0; j < it_height; j++)
				{
					pix_atY += scale;
					*dst_vec[i]->GetPixel888(k, j) = H3ARGB888(buf->GetPixel888(pix_atX, pix_atY)->GetColor());
				}
				pix_atY = -scale / 2;
			}
		}

	}

	buf->Destroy();

	return;

}
void CreateResources()
{

	//int game_version = globalPatcher->VarGetValue<int>("HD.ExeVersion", 1);
	//if (game_version > 30)
	//{
	//	res_allocated[0] = H3LoadedDef::Load("Primint.def")->GetName()[0] == 'P';
	//	res_allocated[1] = H3LoadedDef::Load("WDWCint.def")->GetName()[0] == 'W';
	//	res_allocated[2] = H3LoadedDef::Load("MrlLcki.def")->GetName()[0] == 'M';
	//}


//	H3Lod::
	//H3LodItem* it = H3LodItem::
	H3LoadedDef* spell_int_def = H3LoadedDef::Load("spellint.def");

	int width = spell_int_def->widthDEF;
	int height = spell_int_def->heightDEF;


	CreateSmallerPics(spell_int_def, smaller_spellint, spell_int_def->groups[0]->count, spell_int_def->widthDEF, spell_int_def->heightDEF);

	H3LoadedPcx* realbg = H3LoadedPcx::Load("CCrPop.pcx");
	constexpr int heightOffset = 70;

	width = realbg->width;
	height = realbg->height - heightOffset;
	constexpr int newWidth = 298;
	for (size_t i = 0; i < 2; i++)
	{
		creature_stat_bg[i] = H3LoadedPcx::Create(h3_NullString, width, height);
		realbg->DrawToPcx(0, heightOffset, width, height, creature_stat_bg[i], 0, 0, 1);
	}

	return;
}

void Dlg_MonPreview_HooksInit(PatcherInstance* pi)
{
	CreateResources();

	constexpr int dlg_height = 288 + DLG_HEIGHT_ADD;
	constexpr int dlg_y = 267 - DLG_HEIGHT_ADD + 13;
	pi->WriteDword(0x47205A + 1, dlg_height); // left dlg height ++
	pi->WriteDword(0x472092 + 1, dlg_height); // right dlg height ++
	pi->WriteDword(0x472061 + 1, dlg_y); // left dlg y_pos ++
	pi->WriteDword(0x472099 + 1, dlg_y); // right dlg y_pos ++
	pi->WriteLoHook(0x46D6CB, Battle_Dlg_Create);
	pi->WriteLoHook(0x46D97A, Battle_Dlg_StackInfo_Show);
}