//#include "header.h"
#include "battle_CreatureDlg_SpellsPreview.h"


using namespace h3;



bool ShowStackAvtiveSpells(H3CombatCreature* stack, bool isRMC)
{


    int arr_size = sizeof(stack->activeSpellDuration) / sizeof(INT32);
    int columns = 5 > stack->activeSpellNumber ? stack->activeSpellNumber : 5;
    int rows = stack->activeSpellNumber / columns + (bool)(stack->activeSpellNumber % columns);

    int d_w = H3LoadedDef::Load("spellint.def")->widthDEF;
    int d_h = H3LoadedDef::Load("spellint.def")->heightDEF;
    int width = (d_w + 5) * columns + 35;
    int height = (d_h + 5) * rows + 35;

    H3Dlg* dlg = new SpellsDlg(width, height);


    int x = 20, y = 20;
    int dur = 0, counter = 0;
    for (INT8 i = 0; i < arr_size; i++)
    {
        dur = stack->activeSpellDuration[i];
        if (dur)
        {

            H3DlgDef* def = H3DlgDef::Create(x, y, "spellint.def", i + 1);
            dlg->AddItem(def);

            H3String str = "x";
            str.Append(dur);
            H3DlgText* text = H3DlgText::Create(x, y + 25, d_w, 14, str.String(), h3::NH3Dlg::Text::TINY, 1, 0, eTextAlignment::MIDDLE_RIGHT);
            if (i != NH3Spells::eSpell::BERSERK && i != NH3Spells::eSpell::DISRUPTING_RAY && i != NH3Spells::eSpell::BIND )
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

    if (isRMC)
    {
        dlg->RMB_Show();
    }
    else
    {
      //  dlg->CreateOKButton();
       
        dlg->Start();
    }

    delete dlg;
    return false;
}

_LHF_(Dlg_CreatureInfo_RmcProc)
{
    H3Msg* msg = (H3Msg*)(c->esi);
    int item_id = msg->itemId;

    if ((item_id > 220 && item_id < 224
        || item_id >= 3000 && item_id < 3003)
        && msg->subtype == eMsgSubtype::RBUTTON_DOWN
        && creature_dlg_stack->activeSpellNumber)
    {
        ShowStackAvtiveSpells(creature_dlg_stack, true);

        msg->itemId = -1;
    }

    return EXEC_DEFAULT;
}

_LHF_(Dlg_CreatureInfo_Battle_BeforeCreate)
{
    creature_dlg_stack = (H3CombatCreature*)c->edi;
    return EXEC_DEFAULT;
}


void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance* pi)
{
    pi->WriteLoHook(0x5F4C5D, Dlg_CreatureInfo_RmcProc);
    pi->WriteLoHook(0x5F3741, Dlg_CreatureInfo_Battle_BeforeCreate);
}
