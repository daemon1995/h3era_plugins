// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include "..\oldHeroesHeaders\headers\era.h"
#include "..\oldHeroesHeaders\headers\HoMM3.h"
/*#include "..\heroes\headers\patcher_x86.hpp"
#include "..\heroes\headers\HoMM3_ids.h"
#include "..\heroes\headers\HoMM3_Base.h"
#include "..\heroes\headers\HoMM3_Res.h"
#include "..\heroes\headers\HoMM3_GUI.h"*/


#include <string>
#include <iostream>
// получение разрешения игры

#define o_HD_X (*(_int_*)0x401448)
#define o_HD_Y (*(_int_*)0x40144F)

using namespace Era;

const int ADV_MAP = 37;
const int CTRL_LMB = 4;
const int LMB_PUSH = 12;

Patcher* globalPatcher;
PatcherInstance* _GEM;

namespace dllText
{
    const char* PLUGIN_NAME = "NewDlgItems.era";
    const char* PLUGIN_AUTHOR = "daemon_n";
    const char* PLUGIN_DATA = "16.01.2022";
}
char* GetEraJSON(const char* json_string_name) {
    return tr(json_string_name);
}


//#define SPBID 3430
POINT k_newSacrificeWindowArtifactSlotPositions[19] = {
{0x1AE, 0x11}, {0x1f3,0x11},{0x169,0x11},{0x137,0x11}, {0x225,0x11},
{0x169,0x13F}, {0x137,0x43}, {0x225,0x43}, {0x1F3,0x13F}, {0x137,0x77},
{0x137,0xA9}, {0x137,0xDB}, {0x137,0x10D}, {0x225,0xA9}, {0x225,0xDB},
{0x225,0x10D}, {0x225,0x77}, {0x225,0x13F}, {0x137,0x13F}
};

void nhd_AddNewDefsIntoDlg(_Dlg_* dlg, string jsonSubStr)
{
   
    int bkgId = std::atoi(GetEraJSON((jsonSubStr + ".bg_item_id").c_str()));
    int xPos = std::atoi(GetEraJSON((jsonSubStr+ ".x").c_str()));
    int yPos = std::atoi(GetEraJSON((jsonSubStr + ".y").c_str()));

    dlg->AddItemToOwnArrayList(_DlgStaticDef_::Create(xPos, yPos, 44, 44, bkgId, (char*)"nhd_bkg.def", 1, FALSE, FALSE)); //add new items to list
    dlg->AddItemToOwnArrayList(_DlgStaticDef_::Create(xPos + 49, yPos, ++bkgId, (char*)"nhd_look.def", 1, FALSE, FALSE));
    dlg->AddItemToOwnArrayList(_DlgStaticDef_::Create(xPos + 2, yPos, 34, 225, ++bkgId, (char*)"nhd_cov.def", 1, FALSE, FALSE));
}

int __stdcall OnAfter_HeroDlg_Create(LoHook* hook, HookContext* c)

{
    _Dlg_* dlg = (_Dlg_*)c->edi;

   // CALL_1(_Dlg_*, __thiscall, hook->GetDefaultFunc(), dlg);
    _DlgItem_* it, *itBg;

    int xOffset = std::atoi(GetEraJSON("nhd.hero_screen.x"));
    int yOffset = std::atoi(GetEraJSON("nhd.hero_screen.y"));

    for (int i = 0; i < 19; i++)
    {
        int artSlotId = i + 2;
        int artSlotPressedId = i + 21;
        it = dlg->GetItem(artSlotId);
        itBg = dlg->GetItem(artSlotPressedId);

        if (it && itBg)
        {
            int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + xOffset;
            int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + yOffset;

            it->x = xPos;
            it->y = yPos;
            itBg->x = xPos;
            itBg->y = yPos;
        }
    }

    for (int i = 0; i < 5; i++)
    {
        int artSlotId = i + 40;
        if (dlg->GetItem(artSlotId))
        { 
        dlg->GetItem(artSlotId)->x = i * std::atoi(GetEraJSON("nhd.hero_screen.backpack.interval")) + std::atoi(GetEraJSON("nhd.hero_screen.backpack.x"));
        dlg->GetItem(artSlotId)->y = std::atoi(GetEraJSON("nhd.hero_screen.backpack.y"));
        } 

    }
  
    // стрелки рюкзака
	if (dlg->GetItem(77))// стрелка влево
    {
        dlg->GetItem(77)->x = std::atoi(GetEraJSON("nhd.hero_screen.left_arrow.x"));
        dlg->GetItem(77)->y = std::atoi(GetEraJSON("nhd.hero_screen.left_arrow.y"));
    }   
    if (dlg->GetItem(78))// стрелка вправо
    {
        dlg->GetItem(78)->x = std::atoi(GetEraJSON("nhd.hero_screen.right_arrow.x"));
        dlg->GetItem(78)->y = std::atoi(GetEraJSON("nhd.hero_screen.right_arrow.y"));
    }   

    // кнопка командира
	if (dlg->GetItem(4445)) { dlg->GetItem(4445)->x = std::atoi(GetEraJSON("nhd.hero_screen.commander.x")); dlg->GetItem(4445)->y = std::atoi(GetEraJSON("nhd.hero_screen.commander.y")); }

    return EXEC_DEFAULT;

}

void NewSacrifaceDlgWidgetsPosition(int xOffset, int yOffset)
{
    for (int i = 0; i < 19; i++) // Fill Table with values from json
    {

        int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + xOffset;
        int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + yOffset;

        k_newSacrificeWindowArtifactSlotPositions[i] = { xPos, yPos };
    }

    _GEM->WriteDword(0x560761 + 0x3, (int)&k_newSacrificeWindowArtifactSlotPositions[0].y); //replace artifacts.def original pos

    _GEM->WriteDword(0x5609B6 + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.left_arrow.y"))); //replace left arrow original y pos
    _GEM->WriteByte(0x5609BB + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.left_arrow.x")));  //replace left arrow original x pos

    _GEM->WriteDword(0x560A39 + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.right_arrow.y"))); //replace right_arrow arrow original x pos
    _GEM->WriteDword(0x560A3E + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.right_arrow.x"))); //replace right_arrow arrow original y pos

    _GEM->WriteDword(0x5608FB + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.backpack.y"))); //replace bacpack artifacts.def original y pos
    _GEM->WriteDword(0x5608B6 + 3, std::atoi(GetEraJSON("nhd.altar_of_sacr.backpack.x"))); //replace bacpack artifacts.def original start x pos
    _GEM->WriteByte(0x560921 + 2, std::atoi(GetEraJSON("nhd.altar_of_sacr.backpack.interval"))); //replace bacpack artifacts.def x interval
    return;
}

int __stdcall OnBeforeSacrDlgArtifactPlace(LoHook* hook, HookContext* c)
{
    _Dlg_* dlg = *(_Dlg_**)(c->ebp - 0x10); //get Dlg

    nhd_AddNewDefsIntoDlg(dlg, "nhd.altar_of_sacr");

    int bkgId = std::atoi(GetEraJSON("nhd.altar_of_sacr.bg_item_id"));
    int xPos = std::atoi(GetEraJSON("nhd.altar_of_sacr.x"));
    int yPos = std::atoi(GetEraJSON("nhd.altar_of_sacr.y"));
    NewSacrifaceDlgWidgetsPosition(xPos, yPos);

    return EXEC_DEFAULT;
}

int __stdcall OnBeforeMarketDlgArtifactPlace(LoHook* h, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->edi;
    nhd_AddNewDefsIntoDlg(dlg, "nhd.artifact_merchant");

    int bkgId = std::atoi(GetEraJSON("nhd.artifact_merchant.bg_item_id"));
    int xPos = std::atoi(GetEraJSON("nhd.artifact_merchant.x"));
    int yPos = std::atoi(GetEraJSON("nhd.artifact_merchant.y"));

    return EXEC_DEFAULT;
}

int __stdcall OnAfterMarketDlgArtifactPlace(LoHook* h, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->ecx;
    _DlgItem_* it;
    int xOffset = std::atoi(GetEraJSON("nhd.artifact_merchant.x"));
    int yOffset = std::atoi(GetEraJSON("nhd.artifact_merchant.y"));


    for (int i = 0; i < 19; i++)
    {
        int artSlotId = i + 84;
        int artSlotPressedId = i + 107;

        if (i == 18) // if misc 5 slot = only for HD mod
        {
            artSlotId = 1126;
            artSlotPressedId = 1127;

            int misc_fiveBg = 2126;
            it = dlg->GetItem(misc_fiveBg);
            if (it)
            {
                it->Hide();
            }

        }

        it = dlg->GetItem(artSlotId);
        if (it)
        {
            int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + xOffset;
            int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + yOffset;

            it->x = xPos;
            it->y = yPos;
        }

        it = dlg->GetItem(artSlotPressedId);

        if (it)
        {
            int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + xOffset;
            int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + yOffset;

            it->x = xPos -2;
            it->y = yPos -2;

        }

    }

    for (int i = 0; i < 5; i++)
    {
        int artSlotId = i + 102;
        int artSlotPressedId = i + 125;

        it = dlg->GetItem(artSlotId);
        if (it)
        {

            it->x = i * std::atoi(GetEraJSON("nhd.artifact_merchant.backpack.interval")) + std::atoi(GetEraJSON("nhd.artifact_merchant.backpack.x"));// -2 + 4;
            it->y = std::atoi(GetEraJSON("nhd.artifact_merchant.backpack.y")); //- 2 + 33;
        }

        it = dlg->GetItem(artSlotPressedId);
        if (it)
        {
            it->x = i * std::atoi(GetEraJSON("nhd.artifact_merchant.backpack.interval")) + std::atoi(GetEraJSON("nhd.artifact_merchant.backpack.x")) -2;
            it->y = std::atoi(GetEraJSON("nhd.artifact_merchant.backpack.y")) - 2;// + 33;
        }

    }

    if (dlg->GetItem(130)) //left arrow Pos
    {     
        dlg->GetItem(130)->x = std::atoi(GetEraJSON("nhd.artifact_merchant.left_arrow.x"));
        dlg->GetItem(130)->y = std::atoi(GetEraJSON("nhd.artifact_merchant.left_arrow.y"));
    }

    if (dlg->GetItem(131)) //right_arrow  Pos
    {         
        dlg->GetItem(131)->x = std::atoi(GetEraJSON("nhd.artifact_merchant.right_arrow.x"));
        dlg->GetItem(131)->y = std::atoi(GetEraJSON("nhd.artifact_merchant.right_arrow.y"));
    }

    if (dlg->GetItem(14)) //text with "hero's name's artifacts"
    {
        dlg->GetItem(14)->Hide();
        
    }


    return EXEC_DEFAULT;
}

int __stdcall OnAdventureDlgCreate2(LoHook* h, HookContext* c)
{   
    _Dlg_* dlg = *(_Dlg_**)c->edi;
    _DlgItem_* it;
   // dlg->AddItem(_DlgStaticDef_::Create(200, 200, 5455, SpellInt_DEF, 0, 0, 18));
  //  dlg->AddItem(_DlgTextButton_::Create(22, 22, 14222, (char*)"m_automf.def", (char*)"23", (char*)"bigfont.fnt", 16, 22, 0x18, 162, 2));
   // ExecErmCmd("IF:L^2 22^");
    return EXEC_DEFAULT;
}
int __stdcall OnBeforeHeroDlgArtifactPlace(LoHook* h, HookContext* c)

{
    _Dlg_* dlg = *(_Dlg_**)(c->ebp -0x14);
    nhd_AddNewDefsIntoDlg(dlg, "nhd.hero_screen");

    return EXEC_DEFAULT;
}

void HeroSwapDlgAddNewItems(_Dlg_* dlg, string jsonSubStr,  int side)
{

    int bkgId = std::atoi(GetEraJSON((jsonSubStr + ".bg_item_id").c_str()));
    int xOffset = std::atoi(GetEraJSON((jsonSubStr + ".x").c_str()));
    int yOffset = std::atoi(GetEraJSON((jsonSubStr + ".y").c_str()));
    int idOffset = 20000;
    if (Era::TPluginExists((char)*"hd_wog.dll"))
    {
        
        if (o_HD_Y < 664)

        {             
            yOffset -= 14;
        }
    }
    else
    {
        yOffset -= 25;
    }

    dlg->AddItem(_DlgStaticDef_::Create(xOffset, yOffset, bkgId, (char*)"nhd_bkg.def", 1, FALSE, FALSE)); //add new items to list
    dlg->AddItem(_DlgStaticDef_::Create(xOffset + 49, yOffset, ++bkgId, (char*)"nhd_look.def", 1, FALSE, FALSE));
    dlg->AddItem(_DlgStaticDef_::Create(xOffset + 2, yOffset, ++bkgId, (char*)"nhd_cov.def", 1, FALSE, FALSE));


    for (int i = 0; i < 19; i++)
    {

        int artSlotId = side * 19 + i + 27 + idOffset;
        int artSlotBgId = side * 19 + i + 150 + idOffset;

        int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + xOffset;
        int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + yOffset;

        dlg->AddItem(_DlgStaticDef_::Create(xPos, yPos, 44, 44, artSlotBgId, (char*)"artifact.def", 4, FALSE, 16)); //add new items to list
        dlg->AddItem(_DlgStaticDef_::Create(xPos, yPos, 44, 44, artSlotId, (char*)"artifact.def", 4, FALSE, 16)); //add new items to list
        dlg->GetItem(artSlotId - idOffset)->Hide();
        dlg->GetItem(artSlotBgId - idOffset)->Hide();

        dlg->GetItem(artSlotId)->id -= idOffset;
        dlg->GetItem(artSlotBgId)->id -= idOffset;

    }

    for (int i = 0; i < 5; i++)
    {
        int artSlotId = side * 5 + i + 89 + idOffset;

        int xPos = i * std::atoi(GetEraJSON((jsonSubStr + ".backpack.interval").c_str())) + std::atoi(GetEraJSON((jsonSubStr + ".backpack.x").c_str()));// +xOffset;
        int yPos = std::atoi(GetEraJSON((jsonSubStr + ".backpack.y").c_str()));// +yOffset;
        if (Era::TPluginExists((char)*"hd_wog.dll"))
        {
            if (o_HD_Y < 664)
            {
                yPos -= 14;

            }
        }
        else
        {
            yPos -= 25;

        }
        dlg->AddItem(_DlgStaticDef_::Create(xPos, yPos, 44, 44, artSlotId, (char*)"artifact.def", 4, FALSE, 16)); //add new items to list
        //dlg->GetItem(artSlotId - idOffset)->Hide();

        dlg->GetItem(artSlotId)->id -= idOffset;
    }


    int arrowId = 99 + side;
    if (dlg->GetItem(arrowId))
    {
       //    dlg
            int xPos = std::atoi(GetEraJSON((jsonSubStr + ".left_arrow.x").c_str()));
            int yPos = std::atoi(GetEraJSON((jsonSubStr + ".left_arrow.y").c_str()));
            if (Era::TPluginExists((char)*"hd_wog.dll"))
            {
                if (o_HD_Y < 664)
                {
                    yPos -= 14;

                }
            }
            else
            {
                yPos -= 25;

            }
            dlg->AddItem(_DlgButton_::Create(xPos, yPos, 22, 46, arrowId + idOffset, (char*)"hsbtns3.def", 0, 1, 0, 0, 2)); //add new items to list
            dlg->GetItem(arrowId)->Hide();
            dlg->GetItem(arrowId + idOffset)->id = arrowId;
    }
    arrowId = 101 + side;

    if (dlg->GetItem(arrowId))
    {
        //    dlg
        int xPos = std::atoi(GetEraJSON((jsonSubStr + ".right_arrow.x").c_str()));
        int yPos = std::atoi(GetEraJSON((jsonSubStr + ".right_arrow.y").c_str()));
        if (Era::TPluginExists((char)*"hd_wog.dll"))
        {
            if (o_HD_Y < 664)
            {
                yPos -= 14;

            }
        }
        else
        {
            yPos -= 25;

        }
        //dlg->AddItem(_DlgButton_::Create()
        dlg->AddItem(_DlgButton_::Create(xPos, yPos, 22, 46, arrowId + idOffset, (char*)"hsbtns5.def", 0, 1, 0, 0, 2)); //add new items to list
        dlg->GetItem(arrowId)->Hide();
        dlg->GetItem(arrowId + idOffset)->id = arrowId;
    }

}

int __stdcall OnBeforeHeroSwapDlgShow(LoHook* h, HookContext* c)

{
    _Dlg_* dlg = (_Dlg_*)c->eax;

    HeroSwapDlgAddNewItems(dlg, "nhd.hero_left", 0);
    HeroSwapDlgAddNewItems(dlg, "nhd.hero_right", 1);

  
    return EXEC_DEFAULT;
}

//OnAdventureDlgCreate
void HooksInit()
{

    
    _GEM->WriteLoHook(0x4DEF7A, OnBeforeHeroDlgArtifactPlace);
    _GEM->WriteLoHook(0x4E15BF, OnAfter_HeroDlg_Create);
   
    _GEM->WriteLoHook(0x5E5D64, OnBeforeMarketDlgArtifactPlace);
    _GEM->WriteLoHook(0x5EC280, OnAfterMarketDlgArtifactPlace);

    _GEM->WriteLoHook(0x56073A, OnBeforeSacrDlgArtifactPlace);

    _GEM->WriteLoHook(0x5AEAF0, OnBeforeHeroSwapDlgShow);

    
    
    return;

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    static _bool_ plugin_On = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        //if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;
            ConnectEra();
            //RegisterHandler(OnBattleReplay, "OnBattleReplay");

            globalPatcher = GetPatcher();
            _GEM = globalPatcher->CreateInstance("NewHeroDlg.daemon.plugin");
            HooksInit();


        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

