#include "header.h"
using namespace h3;
H3CombatCreature* creature_dlg_stack = nullptr;

_LHF_(Dlg_CreatureInfo_RmcProc)
{
    H3Msg* msg = (H3Msg*)(c->esi);
    int item_id = msg->itemId;

   // H3Messagebox(std::to_string(creature_dlg_stack->activeSpellNumber).c_str());
    if ((item_id > 220 && item_id < 224
        || item_id >= 3000 && item_id < 3003)
        && msg->subtype == eMsgSubtype::RBUTTON_DOWN
        && creature_dlg_stack->activeSpellNumber)
    {
        int arr_size = sizeof(creature_dlg_stack->activeSpellDuration) / sizeof(INT32);
        int columns = 5 > creature_dlg_stack->activeSpellNumber ? creature_dlg_stack->activeSpellNumber : 5;
        int rows = creature_dlg_stack->activeSpellNumber / columns + (bool)(creature_dlg_stack->activeSpellNumber % columns);

        int d_w = H3LoadedDef::Load("spellint.def")->widthDEF;
        int d_h = H3LoadedDef::Load("spellint.def")->heightDEF;
        int width = (d_w + 5) * columns + 35;
        int height = (d_h + 5) * rows + 35;

        H3Dlg* dlg = new H3Dlg(width, height);


        int x = 20, y = 20;
        int dur = 0, counter = 0;
        for (INT8 i = 0; i < arr_size; i++)
        {
            dur = creature_dlg_stack->activeSpellDuration[i];
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

_LHF_(Dlg_CreatureInfo_Battle_BeforeCreate)
{
   // H3Messagebox("������ ������ ��������� ��� ���� ���������");
   // creature_dlg_stack = *(H3CombatCreature**)(c->ebp + 0x8);
    creature_dlg_stack = (H3CombatCreature*)c->edi;
  //  H3Messagebox(std::to_string(creature_dlg_stack->activeSpellNumber).c_str());
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