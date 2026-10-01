#include "pch.h"
#include "CommanderSkillList.h"
#include "../headers/WoG/NPC.h"
#include <cstddef>

static_assert(sizeof(WoG::NPC) == 296, "Unexpected WoG commander structure");
static_assert(offsetof(WoG::NPC, abilities) == 0x120, "Unexpected commander ability offset");

BOOL CreatureDlgHandler::AddCommanderSkills()
{
    if (!Era::IsCommanderId(dlg->creatureId))
        return FALSE;

    WoG::NPC *npc = nullptr;
    if (stack && stack->side >= 0 && stack->side < 2)
    {
        // The actual side also remains correct when the stack is hypnotized.
        auto *owner = P_CombatManager->hero[stack->side];
        npc = owner ? WoG::NPC::Get(owner->id)
                    : reinterpret_cast<WoG::NPC *>(stack->side ? 0x2861F98 : 0x2861E70);
    }
    else if (!stack && hero)
        npc = WoG::NPC::Get(hero->id);
    if (!npc)
        return FALSE;

    const auto skills = commanderPreview::Collect(npc->abilities.bits);
    const auto layout = commanderPreview::Layout(skills.count, descriptionWidth, 301 - descriptionY);
    if (!layout.height)
        return FALSE;

    H3DefLoader def("dlg_npc3.def");
    if (!def.Get() || !def->groups || !def->groups[0] || def->widthDEF <= 0 || def->heightDEF <= 0 ||
        def->groups[0]->count <= 2 * commanderPreview::MAX_SKILLS - 1)
        return FALSE;
    H3LoadedPcx16 *buffer = H3LoadedPcx16::Create(def->widthDEF, def->heightDEF);
    if (!buffer)
        return FALSE;

    const unsigned char textRows[commanderPreview::MAX_SKILLS] =
        {1, 2, 3, 4, 5, 8, 9, 10, 11, 15, 16, 17, 22, 23, 29};
    int added = 0;
    for (int i = 0; i < skills.count; ++i)
    {
        const int frame = skills.frames[i];
        const int row = textRows[(frame - 1) / 2];
        auto *picture = H3LoadedPcx16::Create(layout.iconSize, layout.iconSize);
        if (!picture)
            break;
        auto *item = H3DlgPcx16::Create(descriptionX + (i % layout.columns) * (layout.iconSize + 4),
                                        descriptionY + (i / layout.columns) * (layout.iconSize + 4),
                                        layout.iconSize, layout.iconSize,
                                        commanderPreview::FIRST_ITEM_ID + i, nullptr);
        if (!item)
        {
            picture->Destroy();
            break;
        }

        buffer->FillRectangle(0, 0, buffer->width, buffer->height, 0, 0, 0);
        def->DrawToPcx16(0, frame, buffer, 0, 0);
        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(
            picture, buffer, buffer->width, buffer->height, 0, 0, layout.iconSize, layout.iconSize);

        const char *hint = CDECL_3(char *, 0x77710B, 28 + row, 1, 0x2860724);
        const char *description = CDECL_3(char *, 0x77710B, 64 + row, 1, 0x2860724);
        H3String popup("{~>dlg_npc3.def:0:");
        popup.Append(frame).Append("}\n\n").Append(description ? description : h3_NullString);
        item->SetHints(hint ? hint : h3_NullString, popup.String(), TRUE);
        // H3DlgPcx16's game destructor dereferences this image. Each item owns
        // its own image; no global pointers survive a closed or nested dialog.
        item->SetPcx(picture);
        dlg->AddItem(item);
        ++added;
    }
    buffer->Destroy();
    if (!added)
        return FALSE;

    commanderPanelHeight = ((added + layout.columns - 1) / layout.columns) * (layout.iconSize + 4);
    // Expand a short configured description area only as far as the buttons.
    const int totalHeight = std::min(301 - descriptionY, std::max(descriptionHeight, commanderPanelHeight + 14));
    descriptionY += commanderPanelHeight;
    descriptionHeight = totalHeight - commanderPanelHeight;
    return TRUE;
}
