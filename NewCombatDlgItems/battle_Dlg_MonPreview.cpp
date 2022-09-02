#include "header.h"
using namespace h3;

//int stack_dlg_ = 0;
//H3LoadedPcx16* pcx_arr[79];


H3Vector<H3LoadedPcx16*> smaller_spellint(0);
H3LoadedPcx* creature_stat_bg[2];



_LHF_(Battle_Dlg_Create)
{
	H3CombatMonsterPanel* dlg_panel = (H3CombatMonsterPanel*)c->eax;

	if (dlg_panel)
	{
		int height = dlg_panel->GetHeight();

		if (height < 200 || dlg_panel->GetX()>3000)
			return EXEC_DEFAULT;


		for (H3DlgItem* it : dlg_panel->GetItems())
		{
			int id = it->GetID() - 2200;
			switch (id)
			{
			case 2: case 4: case 6: case 8:
				it->Cast<H3DlgText>()->SetText("");
				break;
			default:
				break;
			}
		}

		
		H3DlgPcx* _bg = H3DlgPcx::Create(0, 70 + DLG_HEIGHT_ADD, creature_stat_bg[0]->width, creature_stat_bg[0]->height, 777, nullptr);
		if (dlg_panel->GetX() < 400)
		{			
			_bg->SetPcx(creature_stat_bg[0]);
			_bg->AdjustColor(P_CombatManager->heroOwner[0]);
		}
		else
		{
			_bg->SetPcx(creature_stat_bg[1]);
			int player_id = P_CombatManager->hero[1]>0 ? P_CombatManager->heroOwner[1] : P_CombatManager->heroOwner[0];
			_bg->AdjustColor(player_id);
		}
		
		
		dlg_panel->AddItem(_bg);

		H3DlgDef* def;
		H3DlgText* text;

		int item_id = 4000;
		int x_pos, y_pos;
		// add new PS pictures
		for (int i = 0; i < 4; i++)
		{
			y_pos = 75 + i * 12;
			def = H3DlgDef::Create(9, y_pos, item_id++, "Primint.def", i);
			dlg_panel->AddItem(def);
		}

		// add new actions picures && text
		for (int i = 0; i < 4; i++)
		{
			y_pos = 134 + i/2 * 28;
			x_pos = 9 + (i & 1) * 32;
			if (i == 3)
				def = H3DlgDef::Create(x_pos, y_pos, item_id++, "WDWCint.def", 0);
			else
				def = H3DlgDef::Create(x_pos, y_pos, item_id++, "WDWCint.def", i);
			dlg_panel->AddItem(def);

			text = H3DlgText::Create(x_pos -2, y_pos + 2, 30, 12, "", NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_RIGHT);
			dlg_panel->AddItem(text);
		}

		// add mew moral pictrures && text
		for (int i = 0; i < 2; i++)
		{
			y_pos = 131 + DLG_HEIGHT_ADD + i * 12;
			def = H3DlgDef::Create(9, y_pos, item_id++, "MrlLcki.def", i * 3);
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
			y_pos = 224 + i / 2 * 19;

			H3DlgPcx16* _pcx = H3DlgPcx16::Create(x_pos, y_pos, i_width, i_height, item_id++, nullptr);
			_pcx->SetPcx(smaller_spellint[0]);
			dlg_panel->AddItem(_pcx);

			x_pos = 5 + (i & 1) * 34;
			y_pos = 224 + i / 2 * 19;

			text = H3DlgText::Create(x_pos + 10, y_pos + 8, 24, 12, "", NH3Dlg::Text::TINY, 1, item_id++, eTextAlignment::MIDDLE_RIGHT);
			dlg_panel->AddItem(text);
		}
	}

	return EXEC_DEFAULT;
}


void Hel_SetActionType(H3CombatCreature* stack, H3DlgDef* def, H3DlgText* text)
{
	if (stack->IsWaiting())
	{
		def->SetFrame(3);
	//	text->SetText(Era::tr("1_gem_battle.wait"));
	}
	else if (stack->IsDefending())
	{
		def->SetFrame(4);
//		text->SetText(Era::tr("1_gem_battle.def"));
	}
	else if (stack->IsDone())
	{
		def->SetFrame(6);
	//	text->SetText(Era::tr("1_gem_battle.done"));
	}
	else
	{
		def->SetFrame(5);
	//	text->SetText(Era::tr("1_gem_battle.active"));
	}
	return;
}

_LHF_(Battle_Dlg_StackInfo_Show)
{
	if (*(int*)0x698818 == 1);
	{
		//stack_dlg_shown = true;

		H3Hero* hero = (H3Hero*)(c->ebp + 0xC);

		H3CombatMonsterPanel* dlg_panel = (H3CombatMonsterPanel*)c->edi;
		if (dlg_panel)
		{
			H3CombatCreature* stack = (H3CombatCreature*)(c->esi);
			H3Vector<INT16> active_spells(stack->activeSpellNumber);
			int counter = 0;
			if (stack->activeSpellNumber)
			{
				int arr_size = sizeof(stack->activeSpellDuration) / sizeof(INT32);
				
				for (INT8 i = arr_size -1; i >=0; --i)
				{
					if (stack->activeSpellDuration[i])
						active_spells[counter++] = i;
				}
			}


			H3DlgDef* def;
			H3DlgText* text;
			int frame_id, luck, morale, spell_id;
			int it_id = 0;


			for (H3DlgItem* it : dlg_panel->GetItems())
			{
				it_id = it->GetID();


				switch (it_id)
				{

				case 4004:
					def = it->Cast<H3DlgDef>();
					text = def->GetPreviousItem()->Cast<H3DlgText>();
					Hel_SetActionType(stack, def, text);
					break;
				case 4007:
					text = it->Cast<H3DlgText>();
					if (stack->retaliations > 200)
						text->SetText("99+");
					else
						text->SetText(std::to_string(stack->retaliations).c_str());
					break;

				case 4009:
					it->Cast<H3DlgText>()->SetText(std::to_string(stack->info.spellCharges).c_str());
					break;
				case 4011:
					if (stack->info.flags &4) 
						it->Cast<H3DlgText>()->SetText(std::to_string(stack->info.numberShots).c_str());
					else
						it->Cast<H3DlgText>()->SetText("-");

					break;
				case 4012:
					def = it->Cast<H3DlgDef>();
					morale = stack->morale;
					frame_id = morale > 0 ? 1 : morale < 0 ? 0 : 2;
					def->SetFrame(frame_id);
					text = def->GetPreviousItem()->Cast<H3DlgText>();
					text->SetText(std::to_string(morale).c_str());
					break;
				case 4014:
					def = it->Cast<H3DlgDef>();
					luck = stack->luck;
					frame_id = luck > 0 ? 4 : luck < 0 ? 3 : 5;
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
							if (counter>=0
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



void CreateSmallerPics(H3LoadedDef* src, INT32 count, int w , int h)
{
	//

	smaller_spellint.Resize(count);

	H3Palette565* pal = src->palette565;

	constexpr int scale = 2; // Scale of the pic
	int i_width = w / scale;
	int i_height = h / scale;

	H3LoadedPcx16* buf = H3LoadedPcx16::Create( w, h);
	int color_type = o_BPP;
	int it_height = i_height - (color_type != 32);
	for (INT i = 0; i < count; i++)
	{
		smaller_spellint[i] = H3LoadedPcx16::Create(i_width, i_height);

		src->GetGroupFrame(0, i)->DrawToPcx16(0, 0, w, h, buf, 0, 0, pal);

		if (smaller_spellint[i] != nullptr)
		{
			int pix_atX = -scale / 2, pix_atY = -scale / 2;

			for (int k = 0; k < i_width; k++)
			{
				pix_atX += scale;

				for (int j = 0; j < it_height; j++)
				{
					pix_atY += scale;					
					*smaller_spellint[i]->GetPixel888(k, j) = H3ARGB888(buf->GetPixel888(pix_atX, pix_atY)->GetColor());
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
	H3LoadedDef* spell_int_def = H3LoadedDef::Load("spellint.def");

	int width = spell_int_def->widthDEF;
	int height = spell_int_def->heightDEF;


	CreateSmallerPics(spell_int_def, spell_int_def->groups[0]->count, width, height);

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
