// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "header.h"

Patcher* globalPatcher;
PatcherInstance* _PI;
using namespace h3;


int PrepareAndShowArtifactsDlg(H3Hero*hero, int slot, H3DlgItem* it, std::vector<H3Artifact>& heroArts, int scrollsNum)
{

    int artsNum = heroArts.size();
    int artsNoScrollsNum = artsNum - scrollsNum;
    int itemsInRow = std::max(scrollsNum, artsNoScrollsNum) >7 ? 8 : std::max(scrollsNum, artsNoScrollsNum); // set max items in row


    //define base dlg size 1 * 1;
    int width = 38, height = 38, x = 20, y = 20;

    //Set size depending on artifacts table
    width += itemsInRow * 46;
    height += (artsNoScrollsNum / 8 + ((artsNoScrollsNum % 8 != 0))) * 46;

    if (scrollsNum ) // if there are scrolls in vector
        height += (scrollsNum / 8 + ((scrollsNum % 8) != 0)) * 46; // count them for dlg height

    //set dlg pos close to clicked slot
    int newDlgX = it->GetAbsoluteX() - 22;
    int newDlgY = it->GetAbsoluteY() + 30;
    
    //create new dlg, calss derived from H3Dlg, see HeroArts_CustomDlg.hpp

    int xOffset = o_HD_X - width - newDlgX -5;
    int yOffset = o_HD_Y - height - newDlgY -5;

    // adjust dlg pos to game window borders
    if (xOffset < 0)
        newDlgX -= std::abs(xOffset); 
    if (yOffset < 0)
        newDlgY -= std::abs(yOffset);
    HeroArts_CustomDlg* artToChooseDlg = new HeroArts_CustomDlg(width, height, newDlgX, newDlgY, hero, slot);
    
    artToChooseDlg->FrameRegion(0, 0, width, height, false, hero->owner); // set player color frame (for PvP trade mostly)
   
    artToChooseDlg->displayedArts.clear();

    const char* defName = "artifact.def";
    int i = 0;

    for (; i < artsNoScrollsNum; ++i)
    {
        artToChooseDlg->displayedArts.emplace(i + 1, heroArts[i]); // copy arts into dlg std::set<int id, H3Artifact art>
        artToChooseDlg->CreatePcx(x - 2, y - 2, i + 1, "artslot.pcx"); // place artslot bg
        artToChooseDlg->CreateDef(x, y, i + 1, defName, heroArts[i].id); // place artifact def
        x += 46;

        if (x > width - 46) //if out of width
        {
            x = 20; // go to new line
            y += 46;
        }
    }

    if (globalPatcher->VarFind("HD.def.SpScrArt")!=NULL)
        defName = "def.SpScrArt";

    x = 20; // go to new line for scrolls
    if (artsNoScrollsNum % itemsInRow)
        y += 46;

    for (; i < artsNum; ++i)
    {
        artToChooseDlg->displayedArts.emplace(i + 1, heroArts[i]); // copy arts into dlg std::set<int id, H3Artifact art>
        artToChooseDlg->CreatePcx(x - 2, y - 2, i + 1, "artslot.pcx"); // place artslot bg
        artToChooseDlg->CreateDef(x, y, i + 1, defName, heroArts[i].ScrollSpell()); // place spellScroll  def
        x += 46;

        if (x > width - 46) //if out of width 
        {
            x = 20; // go to new line
            y += 46;
        }
    }


    artToChooseDlg->Start(); //run Dlg
    int newArtId = artToChooseDlg->GetSelectedArtd(); //return selected art id

    delete artToChooseDlg; // destroy dlg

    return newArtId;
}

bool CheckHeroBackPackArtifactsDlg(H3Msg* msg, H3BaseDlg* dlg, H3Hero* hero)
{

    if (msg->subtype == eMsgSubtype::LBUTTON_DOWN
        && msg->flags == eMsgFlag::SHIFT
        || msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_UP)
    {

        
        H3DlgItem* it = dlg->ItemAtPosition(msg); // get clicked itrm

        if (it) // if item 
        {
            int slotId = it->GetID();
            int vSlot = eArtifactSlots::NONE;

            if (hero == P_DialogHero) // if std hero dlg
                vSlot = slotId - 2; // other case set corerct slots for hero dlg
            else if (slotId >= 27 && slotId <= 45 || slotId >= 46 && slotId <= 64) //if Swap Dlg
            {       
                H3SwapManager* swapMgr = H3SwapManager::Get(); // get swapMgr
                swapMgr->heroClicked = slotId < 46 ? 0 : 1;
                hero = swapMgr->hero[swapMgr->heroClicked];// : swapMgr->hero[1]; //get correct hero
                vSlot = slotId < 46 ? slotId - 27 : slotId - 46; // and his slots
            }

            if (hero->owner != P_Game->GetPlayerID())
                return ART_NOT_PLACED;

            if (vSlot >= eArtifactSlots::HEAD && vSlot <= eArtifactSlots::MISC5) // if slot is in range
            {
                if (msg->subtype == eMsgSubtype::LBUTTON_DOWN && msg->flags == eMsgFlag::SHIFT)// only for shift + LMC
                {
                    msg->command = eMsgCommand::WHEEL_BUTTON;
                    msg->subtype = eMsgSubtype::MOUSE_WHEEL_BUTTON_UP;// disable next shift + LMC reaction to prevent other reactions
                }
                //play sound
                P_SoundMgr->ClickSound();

                std::vector<H3Artifact> heroArts(0);
                heroArts.reserve(MAX_BP_ARTIFACTS);
                int scrollsCounter = 0;
                int spell = 0;

                for (size_t i = 0; i < MAX_BP_ARTIFACTS; i++)
                {
                    // if artifact exist and can be placed into slot or replace the current art
                    if (hero->backpackArtifacts[i].id != eArtifact::NONE
                        && hero->CanReplaceArtifact(hero->backpackArtifacts[i].id, vSlot))
                    {
                        heroArts.emplace_back(hero->backpackArtifacts[i]);
                        if (hero->backpackArtifacts[i].id == eArtifact::SPELL_SCROLL)
                            scrollsCounter++;
                    }
                }

                // if there are atrifacts to replace
                if (heroArts.size())
                {

                    std::sort(heroArts.begin(), heroArts.end(), [](H3Artifact& a, H3Artifact& b) // sort artifacts
                        {
                            if (a.id != eArtifact::SPELL_SCROLL && b.id != eArtifact::SPELL_SCROLL)
                                return a.GetCost() > b.GetCost(); // by cost, if not scrolls
                            else if ((a.id == eArtifact::SPELL_SCROLL) ^ (b.id == eArtifact::SPELL_SCROLL))
                                return a.id > b.id; // arts before scrolls
                            else // if scrolls only
                            {
                                H3Spell* spellA = &P_Spell[a.ScrollSpell()];
                                H3Spell* spellB = &P_Spell[b.ScrollSpell()];

                                if (spellA->level != spellB->level)
                                    return spellA->level > spellB->level; // sort by Spell lvl
                                else
                                    return spellA->aiValue > spellB->aiValue; // by ai values, if equal lvls
                            }
                        });

                    if (PrepareAndShowArtifactsDlg(hero, vSlot, it, heroArts, scrollsCounter) != eArtifact::NONE) // if chosen any art
                        return ART_PLACED;
                }
            }
        }
    }

    return ART_NOT_PLACED;
}

_LHF_(Dlg_HeroInfo_BeforeBlockWheel)
{
    H3Msg* msg = (H3Msg*)c->esi;

    if ((msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_UP //if mouse wheel bttn release
        || msg->subtype == eMsgSubtype::LBUTTON_DOWN && msg->flags == eMsgFlag::SHIFT) // or LMC press + shift
        && P_Game->GetPlayerID() == P_ActivePlayer->ownerID) // and for active player only
    {
        
        H3Hero* hero = P_DialogHero;
        H3BaseDlg* dlg = msg->GetDlg();

        bool art = CheckHeroBackPackArtifactsDlg(msg, dlg, hero);
        if (art) // if we chose any art 
        {
            THISCALL_1(void, 0x4E1CC0, dlg); //DlgHeroInfo_FullUpdate
            dlg->Redraw(); // redraw dlg
        }
        
    }

    return EXEC_DEFAULT;

}

int __stdcall DlgSwapHero_Proc(HiHook* h, H3SwapManager* swapMgr, H3Msg* msg)
{
    if (msg->subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_UP && msg->command == eMsgCommand::WHEEL_BUTTON // if mouse wheel bttn release
        || msg->subtype == eMsgSubtype::LBUTTON_DOWN && msg->flags == eMsgFlag::SHIFT && msg->command == eMsgCommand::ITEM_COMMAND) // or LMC press + shift
    {
        H3BaseDlg* dlg = (H3BaseDlg*)swapMgr->dlg;
        bool art = false;
        int heroSide = -1;
        if (swapMgr->twoHumansTrade && dlg->GetH3DlgItem(302)->IsEnabled()) // if Network trade
        {
            heroSide = P_Game->GetPlayerID() == P_ActivePlayer->ownerID ? 0 : 1; // check hero side
            art = CheckHeroBackPackArtifactsDlg(msg, dlg, swapMgr->hero[heroSide]);
        }
        else if (swapMgr->samePlayer)
            art = CheckHeroBackPackArtifactsDlg(msg, dlg, swapMgr->hero[0]);

        if (art)
        {
            swapMgr->UpdateLuckMorale();  // update luck and morale SwapMgr_SetLuckAndMorale((int)o_SwapMgr)

            THISCALL_2(int, 0x5AF4E0, swapMgr, 0);//SwapMgr_RedrawHeroDollBackpack left hero
            THISCALL_2(int, 0x5AF4E0, swapMgr, 1);//SwapMgr_RedrawHeroDollBackpack roght hero

            swapMgr->RefreshDialog(); //THISCALL_1(int, 0x5B1200, swapMgr); // SwapMgr_DlgRedraw1(void *this)

            if (heroSide != -1) // if PvP trade 
            {
                swapMgr->samePlayer = 1; // correct pvp Data
                swapMgr->twoHumansTrade = 1;
                THISCALL_1(void, 0x5AFF80, (int)swapMgr); //sendNetData((int)swapMgr)
            }

            dlg->Redraw(); // redraw to see changes
        }
    }
    
    return THISCALL_2(signed int, h->GetDefaultFunc(), (void*)swapMgr, msg);
}
_LHF_(HooksInit)
{
   
    _PI->WriteLoHook(0x4DD624, Dlg_HeroInfo_BeforeBlockWheel);
    _PI->WriteHiHook(0x5B0100, SPLICE_, EXTENDED_, THISCALL_, DlgSwapHero_Proc);

    return EXEC_DEFAULT;
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

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("Hero_Artifacts_Dlg.daemon.plugin");

            _PI->WriteLoHook(0x4EEAF2,HooksInit);


        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

