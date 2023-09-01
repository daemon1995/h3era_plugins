// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "pch.h"
#include "..\..\headers\era.h"
#include "..\..\headers\HoMM3\HoMM3.h"
/*#include "..\oldHeroesHeaders\headers\patcher_x86.hpp"
#include "..\oldHeroesHeaders\headers\HoMM3_ids.h"
#include "..\oldHeroesHeaders\headers\HoMM3_Base.h"
#include "..\oldHeroesHeaders\headers\HoMM3_Res.h"
#include "..\oldHeroesHeaders\headers\HoMM3_GUI.h"*/


#include <string>
#include <iostream>
// получение разрешения игры
#define DllImport  extern "C" __declspec( dllimport )
#define DllExport  extern "C" __declspec( dllexport )
//#define HERO_BG_DEF (char*)"nhd_bkg.def"
//#define HERO_LOOK_DEF (char*)"nhd_look.def"
//#define HERO_SLOTS_DEF (char*)"nhd_cov.def"
//#define HERO_BACKPACK_DEF (char*)"nhd_bpck.def"
#define NHD_EMPTY_PCX "nhd_look.pcx"
#define NHD_NPC_DEF "nhd_NPC.def"
//#define NHD_NPC_NIM_DEF "nhd_NPCn.def"


#define DLG_CMD_SET_DEF 9
#define DLG_CMD_SET_PCX 11
#define BG_PCX_HEIGHT 407
#define BG_PCX_WIDTH 287

#define WoG_NoNPC (*(_int_*)0x277192C) // опция командиров (0-вкл, 1-выкл)
#define NPC_BttnID 4445

namespace dllText
{
    const char* PLUGIN_NAME = "NewDlgItems.era";
    const char* PLUGIN_AUTHOR = "daemon_n";
    const char* PLUGIN_DATA = "16.01.2022";
    const char* SAVEGAME_SECTION = "NewHeroDlg.daemon.plugin";
}
using namespace Era;
using namespace dllText;

struct NewHeroLook {
    _int8_ faction;
    _int8_ raceIndex; //relative from faction start Id
    _int8_ bg; //BackGround Frame id
    //_int8_ bp[8];
};
_int8_ playerBackPackSettings[8] = { -1 }; //bacpPack fram id;
    bool playerHasNewInterFaceMod[8] = { 0 };
    const _int16_ HEROES_MAX_AMOUNT = 156;
NewHeroLook heroDoll[HEROES_MAX_AMOUNT];

//int myArr[HID_XERON + 1];
Patcher* globalPatcher;
PatcherInstance* _PI;
char* GetEraJSON(const char* json_string_name) {
    return tr(json_string_name);
}
int GetHeroId(_Hero_* heroPtr)
{
    unsigned char* bytePtr = reinterpret_cast<unsigned char*>(heroPtr);
    bytePtr += 26;
    int HeroId = *(int*)(bytePtr);
    _DefFrame_;
    return HeroId;
}

//#define SPBID 3430
POINT k_newSacrificeWindowArtifactSlotPositions[19] = {
{0x1AE, 0x11}, {0x1f3,0x11},{0x169,0x11},{0x137,0x11}, {0x225,0x11},
{0x169,0x13F}, {0x137,0x43}, {0x225,0x43}, {0x1F3,0x13F}, {0x137,0x77},
{0x137,0xA9}, {0x137,0xDB}, {0x137,0x10D}, {0x225,0xA9}, {0x225,0xDB},
{0x225,0x10D}, {0x225,0x77}, {0x225,0x13F}, {0x137,0x13F}
};
string pcxNames[4] = { "" };


void CorrectPcxNames(int heroId)
{
    pcxNames[0] = "nhBG_" + std::to_string(heroDoll[heroId].bg) + ".pcx";
    pcxNames[1] = "nhv" + std::to_string(heroDoll[heroId].faction) + "_" + std::to_string(heroDoll[heroId].raceIndex) + ".pcx";
    pcxNames[2] = "nhArt_" + std::to_string(playerBackPackSettings[o_GameMgr->GetMeID()]) + ".pcx";
    pcxNames[3] = "nhBP_" + std::to_string(playerBackPackSettings[o_GameMgr->GetMeID()]) + ".pcx";
    for (int i = 0; i < 4; i++)
    {
        if (!PcxPngExists(pcxNames[i].c_str())) pcxNames[i] = NHD_EMPTY_PCX;
    }
}
void HeroSwapDlgAddNewItems(_Dlg_* dlg, string jsonSubStr, int side, int heroId)
{
    int yOffset = 0;
    CorrectPcxNames(heroId);
    if (Era::TPluginExists((char)*"hd_wog.dll"))
    {
        if (o_HD_Y < 664) { yOffset = 14; }
    }
    else { yOffset = 25; }
    
    int bkgWidth = std::atoi(GetEraJSON((jsonSubStr + ".backgound.width").c_str()));
    int bkgHeight = std::atoi(GetEraJSON((jsonSubStr + ".backgound.height").c_str()));
    if (bkgWidth < 1) { bkgWidth = BG_PCX_WIDTH; }
    if (bkgHeight < 1) { bkgHeight = BG_PCX_HEIGHT; }

    int bkgX = std::atoi(GetEraJSON((jsonSubStr + ".backgound.x").c_str()));
    int bkgY = std::atoi(GetEraJSON((jsonSubStr + ".backgound.y").c_str()));

    int bkgId = std::atoi(GetEraJSON((jsonSubStr + ".bg_item_id").c_str()));

    dlg->AddItem(_DlgStaticPcx16_::Create(bkgX, bkgY - yOffset, bkgWidth, bkgHeight, bkgId, (char*)pcxNames[0].c_str(), 2048));


    int mainXPos = std::atoi(GetEraJSON((jsonSubStr + ".x").c_str()));
    int mainYPos = std::atoi(GetEraJSON((jsonSubStr + ".y").c_str())) - yOffset;

    int width = std::atoi(GetEraJSON((jsonSubStr + ".width").c_str()));
    int height = std::atoi(GetEraJSON((jsonSubStr + ".height").c_str()));

    if (width < 1) { width = BG_PCX_WIDTH; }
    if (height < 1) { height = BG_PCX_HEIGHT; }
    
    dlg->AddItem(_DlgStaticPcx16_::Create(mainXPos, mainYPos, width, height, ++bkgId, (char*)pcxNames[1].c_str(), 2048));


    dlg->AddItem(_DlgStaticPcx16_::Create(mainXPos +1, mainYPos, width, BG_PCX_HEIGHT -57, ++bkgId, (char*)pcxNames[2].c_str(), 2048));

    int backPackY = std::atoi(GetEraJSON((jsonSubStr + ".backpack_picture_altitude.y").c_str())) -yOffset;
    
    dlg->AddItem(_DlgStaticPcx16_::Create(mainXPos + 1, backPackY, width, 56, ++bkgId, (char*)pcxNames[3].c_str(), 2048));


    int idOffset = 20000;

    for (int i = 0; i < 19; i++)
    {

        int artSlotId = side * 19 + i + 27 + idOffset;
        int artSlotBgId = side * 19 + i + 150 + idOffset;

        int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + mainXPos;
        int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + mainYPos;
        _DlgItem_* it1 = _DlgButton_::Create(xPos, yPos, artSlotBgId, (char*)"artifact.def", 0, 0, 0, 0);
      //  dlg->AddItem()
        ///it1.
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
        int yPos = std::atoi(GetEraJSON((jsonSubStr + ".backpack.y").c_str())) - yOffset;// +yOffset;

        dlg->AddItem(_DlgStaticDef_::Create(xPos, yPos, 44, 44, artSlotId, (char*)"artifact.def", 4, FALSE, 16)); //add new items to list
        dlg->GetItem(artSlotId - idOffset)->Hide();

        dlg->GetItem(artSlotId)->id -= idOffset;
    }


    int arrowId = 99 + side;
    if (dlg->GetItem(arrowId))
    {
        int xPos = std::atoi(GetEraJSON((jsonSubStr + ".left_arrow.x").c_str()));
        int yPos = std::atoi(GetEraJSON((jsonSubStr + ".left_arrow.y").c_str())) - yOffset;

        dlg->AddItem(_DlgButton_::Create(xPos, yPos, 22, 46, arrowId + idOffset, (char*)"hsbtns3.def", 0, 1, 0, 0, 2)); //add new items to list
        dlg->GetItem(arrowId)->Hide();
        dlg->GetItem(arrowId + idOffset)->id = arrowId;
    }
    arrowId = 101 + side;

    if (dlg->GetItem(arrowId))
    {
        //    dlg
        int xPos = std::atoi(GetEraJSON((jsonSubStr + ".right_arrow.x").c_str()));
        int yPos = std::atoi(GetEraJSON((jsonSubStr + ".right_arrow.y").c_str())) - yOffset;

        //dlg->AddItem(_DlgButton_::Create()
        dlg->AddItem(_DlgButton_::Create(xPos, yPos, 22, 46, arrowId + idOffset, (char*)"hsbtns5.def", 0, 1, 0, 0, 2)); //add new items to list
        dlg->GetItem(arrowId)->Hide();
        dlg->GetItem(arrowId + idOffset)->id = arrowId;
    }

}


void nhd_AddNewDefsIntoDlg(_Dlg_* dlg, string jsonSubStr, int heroId)
{

    int width = std::atoi(GetEraJSON((jsonSubStr + ".width").c_str()));
    int height = std::atoi(GetEraJSON((jsonSubStr + ".height").c_str()));

    if (width < 1)      width = BG_PCX_WIDTH;   
    if (height < 1)     height = BG_PCX_HEIGHT;
 
    int bkgId = std::atoi(GetEraJSON((jsonSubStr + ".bg_item_id").c_str()));
    int xPos = std::atoi(GetEraJSON((jsonSubStr + ".x").c_str()));
    int yPos = std::atoi(GetEraJSON((jsonSubStr + ".y").c_str()));
    CorrectPcxNames(heroId);

    dlg->AddItemToOwnArrayList(_DlgStaticPcx16_::Create(xPos, yPos, width, height, bkgId, (char*)pcxNames[0].c_str(), 2048));
    dlg->AddItemToOwnArrayList(_DlgStaticPcx16_::Create(xPos, yPos, width, height, ++bkgId, (char*)pcxNames[1].c_str(), 2048));

    dlg->AddItemToOwnArrayList(_DlgStaticPcx16_::Create(xPos + 1, yPos, width, BG_PCX_HEIGHT - 57, ++bkgId, (char*)pcxNames[2].c_str(), 2048));

    int backPackY = std::atoi(GetEraJSON((jsonSubStr + ".backpack_picture_altitude.y").c_str()));
    dlg->AddItemToOwnArrayList(_DlgStaticPcx16_::Create(xPos + 1, backPackY, width, 56, ++bkgId, (char*)pcxNames[3].c_str(), 2048));

}

int __stdcall OnBeforeHeroDlgArtifactPlace(LoHook* h, HookContext* c)
{
    _Dlg_* dlg = *(_Dlg_**)(c->ebp - 0x14);

    nhd_AddNewDefsIntoDlg(dlg, "nhd.hero_screen", GetHeroId(o_HeroDlg_Hero));
    int xPos, yPos;

    int btnId = std::atoi(GetEraJSON("nhd.hero_screen.manager.id"));

    if (!WoG_NoNPC)
    {
        xPos = std::atoi(GetEraJSON("nhd.hero_screen.manager.x"));
        yPos = std::atoi(GetEraJSON("nhd.hero_screen.manager.y"));
    }
    else
    {
        xPos = std::atoi(GetEraJSON("nhd.hero_screen.manager.xNoNpc"));
        yPos = std::atoi(GetEraJSON("nhd.hero_screen.manager.yNoNpc"));
    }
    _int8_ nymOffset = playerHasNewInterFaceMod[o_GameMgr->GetMeID()] *2;
    //if
    _DlgButton_* manager_button = _DlgButton_::Create(xPos, yPos, btnId, (char*)"BttnMAN.def", 0 + nymOffset, 1 + nymOffset, 0, NULL);
    manager_button->full_tip_text =GetEraJSON("nhd.button.manager");
    manager_button->short_tip_text = GetEraJSON("nhd.button.manager");

    dlg->AddItemToOwnArrayList(manager_button);

    //  y[1] = btnId;
     // y[2] = xPos;
     // ExecErmCmd("IF:M^%y1 %y2^");

    return EXEC_DEFAULT;
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
        it = dlg->GetItem(artSlotId);
        if (it)
        { 
            it->x = i * std::atoi(GetEraJSON("nhd.hero_screen.backpack.interval")) + std::atoi(GetEraJSON("nhd.hero_screen.backpack.x"));
            it->y = std::atoi(GetEraJSON("nhd.hero_screen.backpack.y"));
        } 

    }
  
    // стрелки рюкзака
    it = dlg->GetItem(77);

	if (it)// стрелка влево
    {
        it->x = std::atoi(GetEraJSON("nhd.hero_screen.left_arrow.x"));
        it->y = std::atoi(GetEraJSON("nhd.hero_screen.left_arrow.y"));
    }   
    it = dlg->GetItem(78);

    if (it)// стрелка вправо
    {
        it->x = std::atoi(GetEraJSON("nhd.hero_screen.right_arrow.x"));
        it->y = std::atoi(GetEraJSON("nhd.hero_screen.right_arrow.y"));
    }   

    // кнопка командира
   
    
    if (!WoG_NoNPC )
    {
        it = dlg->GetItem(NPC_BttnID);
        if (it)
        {
            it->x = std::atoi(GetEraJSON("nhd.hero_screen.commander.x"));
            it->y = std::atoi(GetEraJSON("nhd.hero_screen.commander.y"));

        } 

    }

    return EXEC_DEFAULT;

}
//int heroId;

void __stdcall OnPreHeroScreen(TEvent* e)
{
    int bkgId = std::atoi(GetEraJSON("nhd.hero_screen.bg_item_id"));
    int heroId = GetHeroId(o_HeroDlg_Hero);

    _Dlg_* dlg = o_CurrentDlg;
    _DlgItem_* it = dlg->GetItem(bkgId);

    CorrectPcxNames(heroId);
    
    for (int i = 0; i < 4; i++)
    {
        it = dlg->GetItem(bkgId++);
        if (it) it->SendCommand(DLG_CMD_SET_PCX, int(pcxNames[i].c_str()));
       // it->SendCommand(11, int("dfgdfg.pcx"));
    }

    it = dlg->GetItem(NPC_BttnID);

    if (it)
    {
        // it->full_tip_text((char)"233");
        it->SendCommand(DLG_CMD_SET_DEF, (int)NHD_NPC_DEF);
        it->x = std::atoi(GetEraJSON("nhd.hero_screen.commander.x"));
        it->y = std::atoi(GetEraJSON("nhd.hero_screen.commander.y"));
        it->height = 46;
        it->width = 40;

        _int8_ nymOffset = playerHasNewInterFaceMod[o_GameMgr->GetMeID()] * 2;

        reinterpret_cast<_DlgStaticDef_*>(it)->def_frame_index = 0 + nymOffset;
        reinterpret_cast<_DlgStaticDef_*>(it)->press_def_frame_index = 1 + nymOffset;
    }

    it = dlg->GetItem(std::atoi(GetEraJSON("nhd.hero_screen.manager.id")));
    if (it)
    {
        //reinterpret_cast<_DlgStaticDef_*>(it)->short_tip_text = GetEraJSON("nhd.button.manager");
      //  reinterpret_cast<_DlgButton_*>(it)->full_tip_text = GetEraJSON("nhd.button.manager");
        it->short_tip_text = GetEraJSON("nhd.button.manager");
    }
    
    
}

int __stdcall Y_HeroDlg_Proc(HiHook* hook, _CustomDlg_* dlg, _EventMsg_* msg)
{
    if (msg->type == MT_MOUSEOVER) {
        _DlgItem_* it = dlg->FindItem(msg->x_abs, msg->y_abs);

        if (it && it->id == std::atoi(GetEraJSON("nhd.hero_screen.manager.id")))
        {
            _DlgMsg_ m; CALL_2(void, __thiscall, 0x5FF3A0, dlg, m.Set(512, 3, 115, 0, 0, 0, (int)GetEraJSON("nhd.button.manager"), 0));
            dlg->Redraw(TRUE, 114, 115);
            return 0;
        }
   

    }

   // _DlgMsg_ m;
    return CALL_2(int, __thiscall, hook->GetDefaultFunc(), dlg, msg);
    
  // dlg->Redraw(TRUE, 114, 115);
   // return EXEC_DEFAULT;
}


void NewSacrifaceDlgWidgetsPosition(int xOffset, int yOffset)
{
    for (int i = 0; i < 19; i++) // Fill Table with values from json
    {

        int xPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".x").c_str())) + xOffset;
        int yPos = std::atoi(GetEraJSON(("nhd.artifacts_slots.slot_" + std::to_string(i) + ".y").c_str())) + yOffset;

        k_newSacrificeWindowArtifactSlotPositions[i] = { xPos, yPos };
    }

    _PI->WriteDword(0x560761 + 0x3, (int)&k_newSacrificeWindowArtifactSlotPositions[0].y); //replace artifacts.def original pos

    _PI->WriteDword(0x5609B6 + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.left_arrow.y"))); //replace left arrow original y pos
    _PI->WriteByte(0x5609BB + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.left_arrow.x")));  //replace left arrow original x pos

    _PI->WriteDword(0x560A39 + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.right_arrow.y"))); //replace right_arrow arrow original x pos
    _PI->WriteDword(0x560A3E + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.right_arrow.x"))); //replace right_arrow arrow original y pos

    _PI->WriteDword(0x5608FB + 1, std::atoi(GetEraJSON("nhd.altar_of_sacr.backpack.y"))); //replace bacpack artifacts.def original y pos
    _PI->WriteDword(0x5608B6 + 3, std::atoi(GetEraJSON("nhd.altar_of_sacr.backpack.x"))); //replace bacpack artifacts.def original start x pos
    _PI->WriteByte(0x560921 + 2, std::atoi(GetEraJSON("nhd.altar_of_sacr.backpack.interval"))); //replace bacpack artifacts.def x interval
    return;
}

int __stdcall OnBeforeSacrDlgArtifactPlace(LoHook* hook, HookContext* c)
{
    _Dlg_* dlg = *(_Dlg_**)(c->ebp - 0x10); //get Dlg
    _Player_* playerPtr = o_GameMgr->GetPlayer(o_GameMgrMgr_GetMeID());

    nhd_AddNewDefsIntoDlg(dlg, "nhd.altar_of_sacr", playerPtr->selected_hero_id);
    
    int bkgId = std::atoi(GetEraJSON("nhd.altar_of_sacr.bg_item_id"));
    int xPos = std::atoi(GetEraJSON("nhd.altar_of_sacr.x"));
    int yPos = std::atoi(GetEraJSON("nhd.altar_of_sacr.y"));
    NewSacrifaceDlgWidgetsPosition(xPos, yPos);

    return EXEC_DEFAULT;
}

int __stdcall OnBeforeMarketDlgArtifactPlace(LoHook* h, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->edi;
    nhd_AddNewDefsIntoDlg(dlg, "nhd.artifact_merchant", GetHeroId(o_Market_Hero));

    return EXEC_DEFAULT;
}





int __stdcall OnAfterMarketDlgArtifactPlace(LoHook* h, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->ecx;
    _DlgItem_* it;
    int xOffset = std::atoi(GetEraJSON("nhd.artifact_merchant.x"));
    int yOffset = std::atoi(GetEraJSON("nhd.artifact_merchant.y"));
    _Hero_* hero = o_Market_Hero;
    int lookId = std::atoi(GetEraJSON("nhd.artifact_merchant.bg_item_id")) + 1;

    _DlgItem_* def = dlg->GetItem(lookId);
    reinterpret_cast<_DlgStaticDef_*>(def)->def_frame_index = GetHeroId(hero);

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

int __stdcall OnBeforeHeroSwapDlgShow(LoHook* h, HookContext* c)

{
    _Dlg_* dlg = (_Dlg_*)c->eax;

    _Hero_* leftHero = o_SwapMgr->hero[0];
    _Hero_* rightHero = o_SwapMgr->hero[1];

    HeroSwapDlgAddNewItems(dlg, "nhd.hero_left", 0, GetHeroId(leftHero));
    HeroSwapDlgAddNewItems(dlg, "nhd.hero_right", 1, GetHeroId(rightHero));
  
    return EXEC_DEFAULT;
}
int __stdcall OnHeroLvlUpDlgShow(LoHook* h, HookContext* c)
{
    _Dlg_* dlg = (_Dlg_*)c->edi;
    _DlgItem_* it = dlg->GetItem(2007);
    if (it)
    {
       // nhd_AddNewDefsIntoDlg(dlg, "nhd.hero_screen", 12);
        int bkgId = std::atoi(GetEraJSON("nhd.hero_screen.bg_item_id"));
        int xPos = std::atoi(GetEraJSON("nhd.hero_screen.x"));
        int yPos = std::atoi(GetEraJSON("nhd.hero_screen.y"));
        int backPackY = std::atoi(GetEraJSON("nhd.hero_screen.backpack_picture_altitude.y"));
        int width = std::atoi(GetEraJSON("nhd.hero_screen.width"));
        int height = std::atoi(GetEraJSON( "nhd.hero_screen.height"));

        if (width < 1)
        {
            width = BG_PCX_WIDTH;
        }
        if (height < 1)
        {
            height = BG_PCX_HEIGHT;
        }



        string heroBgStr = "nhBG_" + std::to_string(heroDoll[17].bg) + ".pcx";
        dlg->AddItem(_DlgStaticPcx16_::Create(22, 55, width, height/2, bkgId, (char*)heroBgStr.c_str(), 2048));

        string heroSilStr = "nhv" + std::to_string(heroDoll[17].faction) + "_" + std::to_string(heroDoll[17].raceIndex) + ".pcx";
        dlg->AddItem(_DlgStaticPcx16_::Create(22, -50, width, height/2, ++bkgId, (char*)heroSilStr.c_str(), 2048));



        ExecErmCmd("IF:L^^");
        it->x += 22;
        //dlg->SetSize(400, 400);
        //dlg
    }
    return EXEC_DEFAULT;

}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DllExport int HeroVisualSetSettings( int heroId, int newFaction,  int newIndex, int newBg, int newBp)
{
    if (heroId < 0 || heroId > HEROES_MAX_AMOUNT - 1)
    {
        Era::y[1] = HEROES_MAX_AMOUNT - 1;
        ExecErmCmd("IF:M^Hero ID is out of range 0 - %y1^");
        return 0;
    }
        if (newFaction >= 0)     heroDoll[heroId].faction = newFaction; 
        if (newIndex >= 0)       heroDoll[heroId].raceIndex = newIndex; 
    
        if (newBg >= 0)          heroDoll[heroId].bg = newBg; 
        if (newBp >= 0)          playerBackPackSettings[o_GameMgr->GetMeID()] = newBp; 

    return 0;
}

DllExport void HeroVisualGetSettings( int heroId, int* newFaction, int *newIndex, int* newBg, int* newBp)
{
    if (heroId < 0 || heroId > HEROES_MAX_AMOUNT - 1)
    {
        Era::y[1] = HEROES_MAX_AMOUNT - 1;
        ExecErmCmd("IF:M^Hero ID is out of range 0 - %y1^");
        return;

    }
       * newFaction = heroDoll[heroId].faction;
       * newIndex = heroDoll[heroId].raceIndex;
       * newBg = heroDoll[heroId].bg;
       * newBp = playerBackPackSettings[o_GameMgr->GetMeID()];

    return;
}

DllExport int SetNymDependecies(int playerId, bool newInterMod)
{
    playerHasNewInterFaceMod[playerId] = newInterMod;

    if (playerBackPackSettings[playerId] == -1)
    {
      //  y[1] = playerId;
      //  y[2] = newInterMod;
      //  ExecErmCmd("IF:L^%y1 %y2^");
        if (newInterMod)
        {
            playerBackPackSettings[playerId] = 10;

        }
        else
        {
            playerBackPackSettings[playerId] = 4;

        }
    }

    return 0;
}
void  __stdcall OnAfterErmInstructions(TEvent* event)
{
    int index = 0;
    int campaignInex = 0;
    for (int i = 0; i < HEROES_MAX_AMOUNT; i++)
    {
        if (o_HeroInfo[i - 1].classHero / 2 != o_HeroInfo[i].classHero / 2 && i > 0) index = 0;


        heroDoll[i].raceIndex = index++;
        heroDoll[i].bg = o_HeroInfo[i].classHero / 2;
        heroDoll[i].faction = heroDoll[i].bg;

        if (o_HeroInfo[i].isCampaing || i == HID_SIR_MULLICH)
        {
            heroDoll[i].raceIndex = campaignInex++;
            --index;
            heroDoll[i].faction = 42;
        }

    }


}

void __stdcall OnSavegameRead(TEvent* Event) {
    int dataSize = 0;
    ReadSavegameSection(sizeof(int), &dataSize, SAVEGAME_SECTION);

    if (dataSize > 0) {
        ReadSavegameSection(sizeof(heroDoll), &heroDoll, SAVEGAME_SECTION);
        ReadSavegameSection(sizeof(playerBackPackSettings), &playerBackPackSettings, SAVEGAME_SECTION);

    }
}

void __stdcall OnSavegameWrite(TEvent* Event) {
    int dataSize = sizeof(heroDoll) +sizeof(playerBackPackSettings);
    WriteSavegameSection(sizeof(dataSize), &dataSize, SAVEGAME_SECTION);
    WriteSavegameSection(sizeof(heroDoll), &heroDoll, SAVEGAME_SECTION);
    WriteSavegameSection(sizeof(playerBackPackSettings), &playerBackPackSettings, SAVEGAME_SECTION);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void HooksInit()
{
  //  _PI->WriteLoHook(0x4F9D79, OnHeroLvlUpDlgShow);




     _PI->WriteLoHook(0x5AEAF0, OnBeforeHeroSwapDlgShow);
   // _PI->WriteLoHook(0x5AEB81, OnBeforeHeroSwapDlgShow);
        
  //  _PI->WriteLoHook(0x4E1A70, OnBefore_HeroDlg_Create);

    _PI->WriteLoHook(0x4DEF7A, OnBeforeHeroDlgArtifactPlace);

    _PI->WriteLoHook(0x4E15BF, OnAfter_HeroDlg_Create);
    RegisterHandler(OnPreHeroScreen, "OnPreHeroScreen");
   // _PI->WriteHiHook(0x4DD540, SPLICE_, EXTENDED_, THISCALL_, Y_HeroDlg_Proc);



    _PI->WriteLoHook(0x5E5D64, OnBeforeMarketDlgArtifactPlace);
    _PI->WriteLoHook(0x5EC280, OnAfterMarketDlgArtifactPlace);

    _PI->WriteLoHook(0x56073A, OnBeforeSacrDlgArtifactPlace);


    RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
    RegisterHandler(OnSavegameRead, "OnSavegameRead");
    RegisterHandler(OnSavegameWrite, "OnSavegameWrite");


    for (int i = 0; i < 8; i++)
    {
        playerBackPackSettings[i] = -1;
    }
  //  _PI->WriteLoHook(0x4BFB30, StartNewGame);

   // Era::TEventHandler(OnAfterErmIsntructions, "OnAfterErmInstructions");
   // Era::TEventHandler(OnAfterErmIsntructions(), "OnAfterErmInstructions");

    
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
            _PI = globalPatcher->CreateInstance((char*)SAVEGAME_SECTION);
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

