#include "HeroArts_CustomDlg.h"

using namespace h3;

HeroArts_CustomDlg::~HeroArts_CustomDlg()
{
}

BOOL HeroArts_CustomDlg::DialogProc(H3Msg& msg)
{
    if (msg.subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_DOWN && msg.command == eMsgCommand::WHEEL_BUTTON
        || msg.command == eMsgCommand::LCLICK_OUTSIDE
        || msg.command == eMsgCommand::RCLICK_OUTSIDE)
        this->Stop();
    else if (msg.subtype == eMsgSubtype::LBUTTON_DOWN || msg.subtype == eMsgSubtype::RBUTTON_DOWN)
    {
        int indexInSet= msg.GetDlg()->ItemAtPosition(msg)->GetID();
        if (indexInSet>0)
        {
            H3Artifact* art = &displayedArts[indexInSet];
            switch (msg.subtype)
            {
            case eMsgSubtype::RBUTTON_DOWN:
                ShowArtifactDescription(art);
                break;
            case eMsgSubtype::LBUTTON_DOWN:
                SwitchHeroArtifact(art, indexInSet -1);
                this->Stop(); //stop dlg
                break;
            default:
                break;
            }
        }
        else
            this->Stop();
    }

    return 0;
}



void HeroArts_CustomDlg::ShowArtifactDescription(const H3Artifact* art)
{
        H3PictureCategories pic;

        if (art->GetId() != eArtifact::SPELL_SCROLL)
            pic = H3PictureCategories::Artifact(art->GetId());
        else
            pic = H3PictureCategories::Spell(art->ScrollSpell());
            
        H3String artDescription;
        // call "BuildUpArtDescription"
        THISCALL_2(H3String*, 0x4DB650, art, &artDescription);

        //show msg
        H3Messagebox::RMB(artDescription.String(), pic);
}

void SortHeroBackPackArtfacts(H3Hero* hero)
{
    std::vector<H3Artifact> bpArtVec(0);
    bpArtVec.reserve(MAX_BP_ARTIFACTS);
    for (int i = 0; i < MAX_BP_ARTIFACTS; i++)
    {
        if (hero->backpackArtifacts[i].id != eArtifact::NONE)
            bpArtVec.emplace_back(hero->backpackArtifacts[i]);
    }
    int vecSize = bpArtVec.size();

    for (int i = 0; i < vecSize; i++)
        hero->backpackArtifacts[i] = bpArtVec[i];
    for (int i = vecSize; i < MAX_BP_ARTIFACTS; i++)
        hero->backpackArtifacts[i].Clear();

    return;
}
void HeroArts_CustomDlg::SwitchHeroArtifact(H3Artifact* art, int itId)
{

    H3Artifact artAtSlot(hero->bodyArtifacts[slot]);
   // H3Artifact artA
    bool isReplaced = !artAtSlot.Empty();
    if (isReplaced)
        hero->RemoveArtifact(slot);// unequip old art from doll

    int i = 0;
    for (; i < MAX_BP_ARTIFACTS; i++) // loop BackPack
        if (hero->backpackArtifacts[i] == *art)
            break;


    hero->GiveArtifact(*art, slot); //Equip Selected Artifact
    hero->RemoveBackpackArtifact(i);// remove art from BackPack

    if (isReplaced) // if we replaced artifact
        hero->GiveBackpackArtifact(artAtSlot, i); // place it into backpack at the same place
    else // if placed into empty slot, then we must sort arts
        SortHeroBackPackArtfacts(hero);

    this->selectedArt = art->id;
    P_SoundMgr->ClickSound();


    return;
}

