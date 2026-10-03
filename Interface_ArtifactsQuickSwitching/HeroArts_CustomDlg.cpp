#include "HeroArts_CustomDlg.h"
#include "..\headers\EraPluginsAPI\ArtifactDescriptionApi.h"

BOOL HeroArts_CustomDlg::DialogProc(H3Msg &msg)
{
    if ((msg.subtype == eMsgSubtype::MOUSE_WHEEL_BUTTON_DOWN && msg.command == eMsgCommand::WHEEL_BUTTON) ||
        msg.command == eMsgCommand::LCLICK_OUTSIDE || msg.command == eMsgCommand::RCLICK_OUTSIDE)
        this->Stop();
    else if (msg.command == eMsgCommand::MOUSE_BUTTON &&
             (msg.subtype == eMsgSubtype::LBUTTON_DOWN || msg.subtype == eMsgSubtype::RBUTTON_DOWN))
    {
        H3DlgItem *item = ItemAtPosition(msg);
        if (!item)
        {
            this->Stop();
            return 0;
        }

        const auto displayedArt = displayedArts.find(item->GetID());
        if (displayedArt != displayedArts.end())
        {
            const H3Artifact &art = displayedArt->second;
            switch (msg.subtype)
            {
            case eMsgSubtype::RBUTTON_DOWN:
                ShowArtifactDescription(&art);
                break;
            case eMsgSubtype::LBUTTON_DOWN:
                if (SwitchHeroArtifact(art))
                    this->Stop();
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

void HeroArts_CustomDlg::ShowArtifactDescription(const H3Artifact *art)
{
    const auto api = globalPatcher->VarGetValue<const ArtifactDescriptionApi::Api *>(ArtifactDescriptionApi::VARIABLE,
                                                                                  nullptr);
    if (api && api->magic == ArtifactDescriptionApi::MAGIC && api->size >= sizeof(*api) &&
        api->version == ArtifactDescriptionApi::VERSION && api->show)
    {
        const ArtifactDescriptionApi::Request request{sizeof(ArtifactDescriptionApi::Request),
                                                     ArtifactDescriptionApi::VERSION, hero, art, slot};
        if (api->show(&request))
            return;
    }
    H3PictureCategories pic;

    if (art->GetId() != eArtifact::SPELL_SCROLL)
        pic = H3PictureCategories::Artifact(art->GetId());
    else
        pic = H3PictureCategories::Spell(art->ScrollSpell());

    H3String artDescription;
    // call "BuildUpArtDescription"
    THISCALL_2(H3String *, 0x4DB650, art, &artDescription);

    // show msg
    H3Messagebox::RMB(artDescription.String(), pic);
}

bool HeroArts_CustomDlg::SwitchHeroArtifact(const H3Artifact &art)
{
    if (!hero || art.Empty() || slot < eArtifactSlots::HEAD || slot > eArtifactSlots::MISC5 ||
        hero->owner != P_Game->GetPlayerID())
        return false;

    int backpackIndex = 0;
    for (; backpackIndex < MAX_BP_ARTIFACTS; ++backpackIndex)
        if (hero->backpackArtifacts[backpackIndex] == art)
            break;

    if (backpackIndex == MAX_BP_ARTIFACTS || !hero->CanReplaceArtifact(art.id, slot))
        return false;

    // CanReplaceArtifact temporarily changes equipment; verify the source again after its hooks run.
    if (hero->backpackArtifacts[backpackIndex] != art)
        return false;

    const H3Artifact selectedArtifact(art);
    const H3Artifact artAtSlot(hero->bodyArtifacts[slot]);
    const bool isReplaced = !artAtSlot.Empty();
    if (isReplaced)
        hero->RemoveArtifact(slot);

    // These native functions return BOOL8; the H3API wrappers discard that result.
    if (!THISCALL_3(BOOL8, 0x4E2C70, hero, &selectedArtifact, slot))
    {
        if (isReplaced)
            hero->GiveArtifact(artAtSlot, slot);
        return false;
    }

    hero->RemoveBackpackArtifact(backpackIndex); // also shifts the remaining backpack entries

    if (isReplaced && !THISCALL_3(BOOL8, 0x4E3200, hero, &artAtSlot, backpackIndex))
    {
        hero->RemoveArtifact(slot);
        hero->GiveArtifact(artAtSlot, slot);
        hero->GiveBackpackArtifact(selectedArtifact, backpackIndex);
        return false;
    }

    selectedArt = selectedArtifact.id;
    P_SoundMgr->ClickSound();
    return true;
}
