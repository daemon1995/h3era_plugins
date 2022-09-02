#include "Header.h"

using namespace h3;
using namespace Era;
H3LoadedPcx16* npc_abils[NPC_MAX_SKILLS];
H3CombatCreature* current_stack;
_LHF_(Dlg_CreatureInfo_Battle_BeforeSettingText)
{
    H3CreatureInfoDlg* dlg = *(H3CreatureInfoDlg**)(c->ebp - 0x30);
    if (dlg)
    {
        
        int width = dlg->GetWidth();
        int height = 311;

        int a = 298;// dlg->GetWidth();
        int b = 80;

        H3DlgPcx* realBg = H3DlgPcx::Create(width, height, "CrStkPu.pcx"); // get real BGitem
        realBg->AdjustColor(0);
        H3LoadedPcx16* pcx = H3LoadedPcx16::Create(h3_NullString, a, b);
        realBg->GetPcx()->DrawToPcx16(0, height - b, a, b, pcx, 0, 0, true);

        H3DlgPcx16* _bg = H3DlgPcx16::Create(0, dlg->GetHeight() - b, a, b, 777, nullptr);
        //_bg->ColorToPlayer(0);
        _bg->SetPcx(pcx);
        dlg->AddItem(_bg, 0);

    }

    return EXEC_DEFAULT;
}


void SSS_CreateResources( H3LoadedPcx16* res_arr[], const char *src_def)
{
    H3LoadedDef* npcSkillDef = H3LoadedDef::Load(src_def);

    int x = 20;
    int y = 255;
    int imgOffset = 19;

    int width = npcSkillDef->widthDEF;
    int height = npcSkillDef->heightDEF;
    constexpr int scale = 3; // Scale of the pic
    int i_width = width / scale;
    int i_height = height / scale;
    H3LoadedPcx16* buf = H3LoadedPcx16::Create(width, height);

    int color_type = o_BPP;
    int it_height = i_height - (color_type != 32);
    for (INT8 i = 0; i < NPC_MAX_SKILLS; i++)
    {
        npcSkillDef->DrawToPcx16(0, 1 + i * 2, 0, 0, width, height, buf, 0, 0, false);
        res_arr[i] = H3LoadedPcx16::Create(i_width, i_height);

        int pix_atX = -scale / 2, pix_atY = -scale / 2;
        for (int k = 0; k < i_width; k++)
        {
            pix_atX += scale;

            for (int j = 0; j < it_height; j++)
            {
                pix_atY += scale;
                *res_arr[i]->GetPixel888(k, j) = H3ARGB888(buf->GetPixel888(pix_atX, pix_atY)->GetColor());
            }
            pix_atY = -scale / 2;
        }
    }


    buf->Destroy();
    return;

}


_LHF_(Dlg_CreatureInfo_Battle_AfterSettingText)
{
    H3CreatureInfoDlg* dlg = (H3CreatureInfoDlg*)(c->esi);
    if (dlg)
    {
        int monId = dlg->creatureId;
    //    H3Messagebox(current_stack->info.namePlural);
        if (monId >= MON_COMMANDER_FIRST_A && monId <= MON_COMMANDER_LAST_D)
        {

            bool side = monId > 182;

            int heroId = -1;// battlStack->GetOwner()->id;
            if (P_CombatManager->hero[side])
                heroId = P_CombatManager->hero[side]->id;

            _Npc_* npc;
            if (heroId != -1)
                npc = GetNpc(heroId);
            else
                npc = side == 0 ? (_Npc_*)0x2861E70 : (_Npc_*)0x2861F98;


            int npcSecSkillsBits = *(int*)((int)npc + 0x120);

            if (npcSecSkillsBits)
            {
                H3LoadedDef* npcSkillDef = H3LoadedDef::Load("dlg_npc3.def");
                int mask = 1;
                int counter = 0;
                int itemId = DLG_CREATURE_INFO_MIN_SKILL_ID;
                bool isLMC = dlg->GetH3DlgItem(30722) ? true : false;

                int x = 20;
                int y = 255;
                int imgOffset = 19;

                int i_width = npc_abils[0]->width;
                int i_height = npc_abils[0]->height;

                for (int i = 0; i < NPC_MAX_SKILLS; i++)
                {
                    if (npcSecSkillsBits & mask) // if skill is already learned
                    {
                        H3DlgPcx16* dlgPcx = H3DlgPcx16::Create(x, y, i_width, i_height, itemId++, nullptr);
                        if (isLMC) // seting hint for rmc and hower
                        {
                            char* hint = Get_ITxt(28 + GetTxtStringIdBySkillId(i), 1);
                            H3String hintRmc = "{~>";
                            hintRmc.Append(npcSkillDef->GetName()).Append(":0:").Append(1 + i * 2).Append("}\n\n").Append(Get_ITxt(64 + GetTxtStringIdBySkillId(i), 1)); // create long ERA string for pic
                            dlgPcx->SetHints(hint, hintRmc.String(), 1);
                        }

                        dlgPcx->SetPcx(npc_abils[i]);
                        dlg->AddItem(dlgPcx);
                        x += imgOffset;
                        counter++;
                        if (counter == 10 && isLMC || counter == 14 && !isLMC)// for LMC dlg post new pictures a line above
                        {
                            imgOffset *= -1;
                            x += imgOffset;
                            y += imgOffset;
                        }
                    }
                    mask <<= 1;
                }

                // set new desc without "[blabla]"
                H3String str = P_CreatureInformation[monId].description;
                int firstCh = str.FindFirstOf('[');
                int lastCh = str.FindFirstOf(']');
                if (firstCh >= 0 && lastCh >= 0 && lastCh > firstCh)
                    dlg->GetText(-1)->SetText(str.Erase(firstCh, lastCh + 1));

            }
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(Dlg_CreatureInfo_RmcProc)
{
    H3Msg* msg = (H3Msg*)(c->esi);
    int item_id = msg->itemId;
   // H3Messagebox(std::to_string(item_id).c_str());

    if (current_stack->activeSpellNumber
        && (item_id > 220 && item_id < 224
            || item_id >= 3000 && item_id < 3003)
        && msg->subtype == eMsgSubtype::RBUTTON_DOWN)
    {
        int arr_size = sizeof(current_stack->activeSpellDuration) / sizeof(INT32);
        int columns = 5 > current_stack->activeSpellNumber ? current_stack->activeSpellNumber : 5;
        int rows = current_stack->activeSpellNumber / columns + (bool)(current_stack->activeSpellNumber % columns);

        int d_w = H3LoadedDef::Load("spellint.def")->widthDEF;
        int d_h = H3LoadedDef::Load("spellint.def")->heightDEF;
        int width = (d_w + 5) * columns + 35;
        int height = (d_h + 5) * rows + 35;

        H3Dlg* dlg = new H3Dlg(width, height);


        int x = 20, y = 20;
        int dur = 0, counter = 0;
        for (INT8 i = 0; i < arr_size; i++)
        {
            dur = current_stack->activeSpellDuration[i];
            if (dur)
            {

                H3DlgDef* def = H3DlgDef::Create(x, y, "spellint.def", i + 1);
                H3String str = "x";
                str.Append(dur);
                H3DlgText* text = H3DlgText::Create(x, y + 25, d_w, 14, str.String(), h3::NH3Dlg::Text::TINY, 1, 0, eTextAlignment::MIDDLE_RIGHT);

                dlg->AddItem(def);
                if (i != NH3Spells::eSpell::BERSERK && i != NH3Spells::eSpell::DISRUPTING_RAY)
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

        dlg->PlaceAtMouse();
        dlg->RMB_Show();
        msg->itemId = -1;
    }

    return EXEC_DEFAULT;
}

_LHF_(Dlg_CreatureInfo_HintProc)
{
    H3Msg* msg = (H3Msg*)(c->esi);

    if (c->eax >= DLG_CREATURE_INFO_MIN_SKILL_ID)
    {
        int itemId = c->eax;
        H3CreatureInfoDlg* dlg = (H3CreatureInfoDlg*)msg->GetDlg();
        int mon_id = dlg->creatureId;

        if (mon_id >= MON_COMMANDER_FIRST_A
            && mon_id <= MON_COMMANDER_LAST_D
            && itemId >= DLG_CREATURE_INFO_MIN_SKILL_ID
            && itemId < DLG_CREATURE_INFO_MIN_SKILL_ID + NPC_MAX_SKILLS
            && itemId != *(int*)0x68C6B0)
        {
            //setting hint , hacking buffer

            *(int*)0x68C6B0 = itemId;

            H3DlgItem* it = dlg->GetH3DlgItem(itemId);
            strcpy(h3_TextBuffer, it->GetHint());
            c->return_address = 0x5F5319;

            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(Dlg_CreatureInfo_Battle_BeforeCreate)
{
    current_stack = (H3CombatCreature*)(c->eax);
   // H3Messagebox(current_stack->info.soundName);
    return EXEC_DEFAULT;

    // *(int*)(c->ebp + 0x10) += 25;
    // y[5] = *(int*)(*(int*)c->ebp + 0x10);
    int currDlgHeight = *(int*)0x5F3729;
    if (currDlgHeight > 311)
    {
        *(int*)(c->ebp + 0x10) -= (currDlgHeight - 311) / 2;

    }
    return EXEC_DEFAULT;
}
