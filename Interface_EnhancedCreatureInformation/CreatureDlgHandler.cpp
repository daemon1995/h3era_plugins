#include "pch.h"
#include "CreatureDlgHandler.h"
#include "CreatureDlgHooks.h"
#include "CreatureDlgLayout.h"
#include "PluginSettings.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

using namespace h3;
using namespace creatureInfo;


namespace
{
struct _DlgCreatureExpoInfo_
{
    char *Caption;
    char *Info;
    char *Picture;
    char *PictureHint;
    char **TxtProperties;
    char **IcoProperties;
    char **HintProperties;
    char *ColCaptions;
    char *ColHint;
    char **RowCaptions;
    char **RowCaptionHints;
    char **Rows;
    char **RowHints;
    int IcoPropertiesCount;
    int ShowSpecButton;
    char *SpecButtonHint;
    int CurPropColLeft;
    int CurPropColRight;
    char *ArtIcon;
    char *ArtHint;
    int ArtOutput;
    int Flags;
    int ArtCopy;
};

static_assert(offsetof(_DlgCreatureExpoInfo_, TxtProperties) == 0x10, "Unexpected CrExp text array offset");
static_assert(offsetof(_DlgCreatureExpoInfo_, IcoProperties) == 0x14, "Unexpected CrExp icon array offset");
static_assert(offsetof(_DlgCreatureExpoInfo_, HintProperties) == 0x18, "Unexpected CrExp hint array offset");
static_assert(offsetof(_DlgCreatureExpoInfo_, IcoPropertiesCount) == 0x34, "Unexpected CrExp icon count offset");

struct CrExpBonLine
{
    unsigned __int32 Act : 1;
    unsigned __int32 _unused : 31;
    char Type;
    char Mod;
    char Lvls[11];
};

static_assert(offsetof(CrExpBonLine, Lvls) == 6, "Unexpected CrExp bonus level offset");

// WoG stores CrExpo records in 16-byte slots. For a purchase-preview there is
// no persistent CrExpo record, but CrExpBon_Dlg_PrepareInfo still expects one
// when it builds the icon/text buffers. This local record is never registered
// in CrExpoSet and therefore cannot alter game state.
struct PreviewCrExpo
{
    int experience = 0;
    int count = 1;
    DWORD flags = 0;
    DWORD artifact = 0;
};

constexpr int CR_EXP_BONUS_LINES = 20;
constexpr DWORD CR_EXP_BONUS_TABLE = 0x0847D98;

CrExpBonLine *GetCrExpBonusLine(int index)
{
    return reinterpret_cast<CrExpBonLine *>(CR_EXP_BONUS_TABLE + index * 17);
}

DWORD MakePreviewCrExpoFlags(int creatureId)
{
    // CrExpo bit layout: Act:1, Type:4, MType:8, then artifact bits.
    // Preview records are not inserted into CrExpoSet, but native WoG dialog
    // code still expects an active record with a valid non-zero storage type.
    // CE_HERO (1) is harmless here because no lookup by location is performed.
    constexpr DWORD CE_HERO = 1;
    return 1u | (CE_HERO << 1) |
           ((static_cast<DWORD>(static_cast<unsigned char>(creatureId)) << 5) & 0x1FE0u);
}
} // namespace

void CreatureDlgHandler::HideDefaultBattleSpellItems()
{
    if (!dlg)
        return;

    for (int id = 221; id <= 223; ++id)
        if (auto *item = dlg->GetH3DlgItem(id))
            item->HideDeactivate();
}

CreatureDlgHandler::CreatureDlgHandler(H3CreatureInfoDlg *dlg, H3CombatCreature *stack, H3Army *army,
                                       int armySlotIndex, const H3Hero *hero, int playerColor)
    : dlg(dlg), stack(stack), army(army), armySlotIndex(armySlotIndex),
      wogStackExperience(WogStackExperienceEnabled()), hero(hero), playerColor(playerColor)
{
    if (!dlg)
        return;

    ApplyDialogAppearance();
    ReadDescriptionRect(descriptionX, descriptionY, descriptionWidth, descriptionHeight);
    if (GetPluginSettings().showCommanderSkills)
        AddCommanderSkills();
    AlignItems();
    if (wogStackExperience && dlg->GetDefButton(30722))
        AddExperienceButton();
    if (stack)
    {
        HideDefaultBattleSpellItems();
        AddSpellEfects();
    }
}

CreatureDlgHandler::~CreatureDlgHandler()
{
    ReleaseUnownedSkillImages();
}

void CreatureDlgHandler::ApplyDialogAppearance()
{
    if (!dlg)
        return;

    // Normalize the final object after the native constructor. The nested
    // constructor hooks already make the native build use the enlarged size,
    // while this protects us against hook chaining by other plugins.
    dlg->SetWidth(DLG_WIDTH);
    dlg->SetHeight(DLG_HEIGHT);

    if (auto *background = dlg->GetPcx(200))
    {
        background->SetWidth(DLG_WIDTH);
        background->SetHeight(DLG_HEIGHT);
        background->SetPcx(DIALOG_BACKGROUND_PCX);

        // H3API::AdjustColor is the wrapper for SoD's
        // DlgStaticPcx8_Colorize (0x4501D0). SetPcx above reloads the PCX,
        // therefore the native Colorize done inside the original constructor
        // must be applied once more to the final resource.
        if (playerColor >= 0 && playerColor < 8)
            background->AdjustColor(playerColor);
    }
}

void CreatureDlgHandler::ScrollTo(int scrollTick)
{
    if (!dlg)
        return;

    scrollTick = Clamp(0, scrollTick, maxScrollTick);

    int scrollOffset = 0;
    if (!scrollRowOffsets.empty())
    {
        const int rowIndex = std::min(scrollTick, static_cast<int>(scrollRowOffsets.size()) - 1);
        scrollOffset = scrollRowOffsets[rowIndex];
    }

    const int viewportBottom = viewportY + viewportHeight;
    for (auto &entry : scrolledItems)
    {
        if (!entry.item)
            continue;

        const int y = entry.baseY - scrollOffset;
        entry.item->SetY(y);
        if (y >= viewportY && y + entry.item->GetHeight() <= viewportBottom)
            entry.item->ShowActivate();
        else
            entry.item->HideDeactivate();
    }

    dlg->Redraw(viewportX, viewportY, viewportWidth, viewportHeight);
}

H3DlgText *CreatureDlgHandler::FindOriginalDescription() const
{
    H3DlgText *description = dlg ? dlg->GetText(-1) : nullptr;
    if (!description && dlg)
        description = dlg->GetText(1);
    return description;
}

std::string CreatureDlgHandler::GetDescriptionText(H3DlgText *description) const
{
    const char *text = description ? description->GetH3String().String() : nullptr;
    if ((!text || !*text) && dlg && dlg->creatureId >= 0 && H3CreatureInformation::Get()[dlg->creatureId].description)
        text = H3CreatureInformation::Get()[dlg->creatureId].description;

    std::string result(text ? text : h3_NullString);
    if (commanderPanelHeight)
    {
        const auto first = result.find('[');
        const auto last = first == std::string::npos ? first : result.find(']', first + 1);
        if (last != std::string::npos)
            result.erase(first, last - first + 1);
    }
    return result;
}

BOOL CreatureDlgHandler::AlignItems()
{
    if (!dlg)
        return FALSE;

    if (auto *hint = dlg->GetTextPcx(224))
    {
        hint->SetX(7);
        hint->SetY(DLG_HEIGHT - hint->GetHeight() - 7);
        hint->SetWidth(DLG_WIDTH - 14);
    }
    if (auto *name = dlg->GetText(203))
        name->SetX((DLG_WIDTH - name->GetWidth()) / 2);

    if (auto *morale = dlg->GetDef(219))
    {
        morale->SetX(24);
        morale->SetY(DLG_HEIGHT - morale->GetHeight() - 46);
        if (auto *text = dlg->GetText(3006))
            text->SetY(morale->GetY() + morale->GetHeight() - text->GetHeight());
    }
    if (auto *luck = dlg->GetDef(220))
    {
        luck->SetX(78);
        luck->SetY(DLG_HEIGHT - luck->GetHeight() - 46);
        if (auto *text = dlg->GetText(3007))
            text->SetY(luck->GetY() + luck->GetHeight() - text->GetHeight());
    }
    if (auto *ok = dlg->GetDefButton(30722))
    {
        ok->SetX(OK_BUTTON_X);
        ok->SetY(BUTTON_Y);
        ok->SetWidth(BUTTON_WIDTH);
        ok->SetHeight(BUTTON_HEIGHT);
    }
    if (auto *dismiss = dlg->GetDefButton(30723))
    {
        dismiss->SetX(127);
        dismiss->SetY(BUTTON_Y);
        dismiss->SetWidth(BUTTON_WIDTH);
        dismiss->SetHeight(BUTTON_HEIGHT);
    }
    if (auto *upgrade = dlg->GetDefButton(300))
    {
        upgrade->SetX(233);
        upgrade->SetY(BUTTON_Y);
        upgrade->SetWidth(BUTTON_WIDTH);
        upgrade->SetHeight(BUTTON_HEIGHT);
    }
    if (auto *creatureCast = dlg->GetCustomButton(301))
    {
        creatureCast->SetX(126);
        creatureCast->SetY(BUTTON_Y);
        creatureCast->SetWidth(BUTTON_WIDTH);
        creatureCast->SetHeight(BUTTON_HEIGHT);
    }

    return BuildDescriptionArea();
}

BOOL CreatureDlgHandler::BuildDescriptionArea()
{
    H3DlgText *description = FindOriginalDescription();
    const std::string descriptionText = GetDescriptionText(description);
    if (description)
        description->HideDeactivate();

    // Commanders already have their own reduced icon block. Ordinary creatures
    // use current active skills for stack/army dialogs and the full 0..10 rank
    // preview for a purchase dialog that has no persistent CrExpo record.
    if (GetPluginSettings().showCreatureSkills && !Era::IsCommanderId(dlg->creatureId) &&
        CreateCreatureSkillsList() && !creatureSkills.empty())
    {
        if (BuildExperienceSkillsPanel(description, descriptionText))
            return TRUE;
    }

    ReleaseUnownedSkillImages();
    const int availableHeight = std::max(descriptionHeight, CONTENT_BOTTOM - descriptionY);
    const char *fontName = description && description->GetFont() ? description->GetFont()->GetName() : NH3Dlg::Text::TINY;
    const int color = description ? description->color : 4;
    H3DlgScrollableText *scrollable = H3DlgScrollableText::Create(descriptionText.c_str(), descriptionX, descriptionY,
                                                                  descriptionWidth, availableHeight, fontName, color,
                                                                  false);
    if (scrollable)
    {
        dlg->AddItem(scrollable);
        return TRUE;
    }

    if (description)
        description->ShowActivate();
    return FALSE;
}

BOOL CreatureDlgHandler::BuildExperienceSkillsPanel(H3DlgText *description, const std::string &descriptionText)
{
    scrolledItems.clear();
    viewportX = descriptionX;
    viewportY = descriptionY;
    viewportWidth = descriptionWidth;
    viewportHeight = std::max(descriptionHeight, CONTENT_BOTTOM - descriptionY);
    maxScrollTick = 0;
    scrollRowOffsets.clear();

    const char *fontName = description && description->GetFont() ? description->GetFont()->GetName() : NH3Dlg::Text::TINY;
    const int textColor = description ? description->color : 4;
    const int textAlign = description ? description->alignment : GetPluginSettings().descriptionAlignment;
    H3Font *font = H3Font::Load(fontName);
    if (!font)
    {
        ReleaseUnownedSkillImages();
        return FALSE;
    }

    int contentWidth = descriptionWidth;
    int columns = std::max(1, (contentWidth + EXP_SKILL_GAP) / (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP));
    columns = std::min(columns, static_cast<int>(creatureSkills.size()));
    int rows = (static_cast<int>(creatureSkills.size()) + columns - 1) / columns;
    int skillBlockHeight = rows * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP);
    const int lineHeight = std::max(10, static_cast<int>(font->height) + 2);

    H3Vector<H3String> lines;
    font->SplitTextIntoLines(descriptionText.c_str(), contentWidth, lines);
    int totalHeight = skillBlockHeight + (lines.Size() ? 2 : 0) + static_cast<int>(lines.Size()) * lineHeight;
    bool needScrollbar = totalHeight > viewportHeight;

    if (needScrollbar)
    {
        contentWidth = std::max(32, descriptionWidth - EXP_SCROLLBAR_WIDTH - EXP_SCROLLBAR_GAP);
        columns = std::max(1, (contentWidth + EXP_SKILL_GAP) / (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP));
        columns = std::min(columns, static_cast<int>(creatureSkills.size()));
        rows = (static_cast<int>(creatureSkills.size()) + columns - 1) / columns;
        skillBlockHeight = rows * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP);
        lines.RemoveAll();
        font->SplitTextIntoLines(descriptionText.c_str(), contentWidth, lines);
        totalHeight = skillBlockHeight + (lines.Size() ? 2 : 0) + static_cast<int>(lines.Size()) * lineHeight;
    }

    // Build logical rows after the final content width is known.
    // One icon row == one tick; one wrapped text line == one tick.
    scrollRowOffsets.reserve(static_cast<size_t>(rows) + lines.Size());
    for (int row = 0; row < rows; ++row)
        scrollRowOffsets.push_back(row * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP));

    const int textStartOffset = skillBlockHeight + (lines.Size() ? 2 : 0);
    for (UINT32 lineIndex = 0; lineIndex < lines.Size(); ++lineIndex)
        scrollRowOffsets.push_back(textStartOffset + static_cast<int>(lineIndex) * lineHeight);

    if (needScrollbar && totalHeight > viewportHeight && !scrollRowOffsets.empty())
    {
        const int overflowPixels = totalHeight - viewportHeight;

        // Find how many complete logical rows have to be skipped before the
        // remaining content fits in the viewport. This row index is max tick.
        maxScrollTick = static_cast<int>(scrollRowOffsets.size()) - 1;
        for (size_t rowIndex = 1; rowIndex < scrollRowOffsets.size(); ++rowIndex)
        {
            if (scrollRowOffsets[rowIndex] >= overflowPixels)
            {
                maxScrollTick = static_cast<int>(rowIndex);
                break;
            }
        }
    }

    int addedSkills = 0;
    for (size_t i = 0; i < creatureSkills.size(); ++i)
    {
        CreatureSkill &skill = creatureSkills[i];
        if (!skill.pcx16)
            continue;

        const int baseY = descriptionY + static_cast<int>(i / columns) * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP);
        const int x = descriptionX + static_cast<int>(i % columns) * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP);
        H3DlgPcx16 *item = H3DlgPcx16::Create(x, baseY, EXP_SKILL_ICON_SIZE, EXP_SKILL_ICON_SIZE,
                                              CREATURE_EXP_SKILL_FIRST_ID + static_cast<int>(i), nullptr);
        if (!item)
            continue;
        item->SetPcx(skill.pcx16);
        skill.pcx16 = nullptr; // ownership is now held by the native dialog item

		libc::sprintf(h3_TextBuffer, "{~>%s:0:%d block}\n\n%s", CREATURE_EXP_DEF, skill.frame, skill.text.c_str());

		std::string popup(h3_TextBuffer);
        item->SetHints(skill.hint.c_str(), h3_TextBuffer, TRUE);
        dlg->AddItem(item);
        scrolledItems.push_back({item, baseY});
        ++addedSkills;
    }

    if (!addedSkills)
    {
        font->Dereference();
        ReleaseUnownedSkillImages();
        scrolledItems.clear();
        return FALSE;
    }

    const int textStartY = descriptionY + skillBlockHeight + (lines.Size() ? 2 : 0);
    for (UINT32 i = 0; i < lines.Size(); ++i)
    {
        H3DlgText *line = H3DlgText::Create(descriptionX, textStartY + static_cast<int>(i) * lineHeight,
                                            contentWidth, lineHeight, lines[i].String(), fontName, textColor,
                                            CREATURE_EXP_TEXT_FIRST_ID + static_cast<int>(i), textAlign);
        if (!line)
            continue;
        dlg->AddItem(line);
        scrolledItems.push_back({line, line->GetY()});
    }

    font->Dereference();
    ReleaseUnownedSkillImages();

    if (needScrollbar && maxScrollTick > 0)
    {
        H3DlgScrollbar *scrollbar = H3DlgScrollbar::Create(
            descriptionX + descriptionWidth - EXP_SCROLLBAR_WIDTH, descriptionY, EXP_SCROLLBAR_WIDTH,
            viewportHeight, CREATURE_EXP_SCROLLBAR_ID, maxScrollTick + 1, CreatureSkillsScrollbarProc,
            false, 1, true);
        if (scrollbar)
        {
            // stepSize=1 means one scrollbar tick == one logical content row.
            dlg->AddItem(scrollbar);
        }
        else
        {
            maxScrollTick = 0;
            scrollRowOffsets.clear();
        }
    }

    ScrollTo(0);
    return TRUE;
}

void CreatureDlgHandler::ReleaseUnownedSkillImages()
{
    for (auto &skill : creatureSkills)
    {
        if (skill.pcx16)
        {
            skill.pcx16->Destroy();
            skill.pcx16 = nullptr;
        }
    }
    creatureSkills.clear();
}

BOOL CreatureDlgHandler::AddExperienceButton()
{
    const bool isNpc = Era::IsCommanderId(dlg->creatureId);
    if (isNpc && !stack)
        return FALSE;

    constexpr int x = 180;
    const int y = BUTTON_Y;
    H3DlgPcx *frame = H3DlgPcx::Create(x - 1, y - 1, -1, "box46x32.pcx");
    if (frame)
        dlg->AddItem(frame);

    H3DlgDefButton *button = H3DlgDefButton::Create(x, y, WOG_CREATURE_EXP_BUTTON_ID, "CrExpBut.def", 0, 1, false,
                                                    eVKey::H3VK_E);
    if (!button)
        return FALSE;
    H3String hint = isNpc ? Era::tr("eci.combat_dialog.creature_info.npc_hint")
                          : Era::tr("eci.combat_dialog.creature_info.stack_exp_hint");
    button->SetHints(hint.String(), h3_NullString, true);
    dlg->AddItem(button);
    return TRUE;
}

BOOL CreatureDlgHandler::AddSpellEfects()
{
    if (!stack)
        return FALSE;

    std::vector<INT32> activeSpells;
    const int arrSize = sizeof(stack->activeSpellDuration) / sizeof(INT32);
    for (int i = 0; i < arrSize; ++i)
        if (stack->activeSpellDuration[i])
            activeSpells.push_back(i);

    const bool needToExpand = activeSpells.size() > 6;
    const int spellsToShow = needToExpand ? 5 : static_cast<int>(activeSpells.size());
    for (INT32 i = 0; i < spellsToShow; ++i)
    {
        const int yPos = 42 * i + 47;
        const int spellId = activeSpells[i];
        H3DlgDef *spellDef = H3DlgDef::Create(283, yPos, 1000 + i,  NH3Dlg::Assets::SPELL_SMALL, spellId + 1);
        if (!spellDef)
            continue;

        H3String spellName = H3Spell::Get()[spellId].name;
        H3String spellDesc = H3Spell::Get()[spellId].description[0];
        if (spellDesc == h3_NullString)
            spellDesc = spellName;
        switch (spellId)
        {
        case eSpell::BIND:
            libc::sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(681), spellName.String(),
                          H3GeneralText::Get()->GetText(682));
            break;
        case eSpell::BERSERK:
            libc::sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(681), spellName.String(),
                          H3GeneralText::Get()->GetText(683));
            break;
        case eSpell::DISRUPTING_RAY:
            libc::sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(681), spellName.String(),
                          H3GeneralText::Get()->GetText(684));
            break;
        default:
            libc::sprintf(h3_TextBuffer, H3GeneralText::Get()->GetText(612), spellName.String(),
                          stack->activeSpellDuration[spellId]);
            break;
        }
        spellDef->SetHints(h3_TextBuffer, spellDesc.String(), true);
        dlg->AddItem(spellDef);

        if (stack->activeSpellDuration[spellId] && spellId != eSpell::BERSERK && spellId != eSpell::DISRUPTING_RAY &&
            spellId != eSpell::BIND)
        {
            H3String duration("x");
            duration.Append(stack->activeSpellDuration[spellId]);
            H3DlgText *durationText = H3DlgText::Create(spellDef->GetX() + spellDef->GetWidth() - 24,
                                                        spellDef->GetY() + spellDef->GetHeight() - 12, 24, 12,
                                                        duration.String(), NH3Dlg::Text::TINY, 1, 0,
                                                        eTextAlignment::BOTTOM_RIGHT);
            if (durationText)
            {
                durationText->SetHints(h3_TextBuffer, spellDesc.String(), true);
                dlg->AddItem(durationText);
            }
        }

        if (i == 4 && needToExpand)
        {
            H3DlgDefButton *button = H3DlgDefButton::Create(283, yPos + 42, DLG_SPELLS_BTTN_ID, SPELL_LIST_BUTTON_DEF, 0, 1,
                                                            false, eVKey::H3VK_S);
            if (button)
            {
                button->SetHints(Era::tr("eci.combat_dialog.creature_info.spell_list_hint"), h3_NullString, true);
                dlg->AddItem(button);
            }
        }
    }
    return TRUE;
}


BOOL CreatureDlgHandler::CreateCreatureSkillsList()
{
    ReleaseUnownedSkillImages();
    if (!dlg || !wogStackExperience)
        return FALSE;

    const int creatureId = dlg->creatureId;
    const bool hasArmyExperienceSource = army && armySlotIndex >= 0 && armySlotIndex <= 6;
    const bool hasExperienceSource = stack || hasArmyExperienceSource;
    const bool isPreviewWithoutCrExpo = !hasExperienceSource;

    DWORD crExpo = 0;
    int experience = 0;

    if (stack)
    {
        const int expoCreatureType = CDECL_1(int, 0x716C8E, stack);
        CDECL_2(void, 0x728120, expoCreatureType, creatureId);
        crExpo = CDECL_1(DWORD, 0x728110, expoCreatureType);
    }
    else
    {
        // Select the Creature Experience bonus table for this creature. This is
        // sufficient for army and purchase dialogs; only the former has a
        // persistent CrExpo record.
        CDECL_2(void, 0x728120, -1, creatureId);

        if (hasArmyExperienceSource)
        {
            int expType = 0;
            DWORD expData = 0;
            CDECL_4(void, 0x71A1B7, army, armySlotIndex, &expType, &expData);
            if (expType >= 1 && expType <= 5)
                crExpo = CDECL_2(DWORD, 0x718617, expType, expData);
        }
    }

    if (hasExperienceSource)
    {
        // A concrete battle/army stack keeps the strict old behavior: no
        // CrExpo or zero experience means no skill icon panel.
        if (!crExpo)
            return FALSE;

        experience = IntAt(crExpo);
        if (experience <= 0)
            return FALSE;
    }

    int creatureCount = dlg->numberCreatures;
    if (creatureCount <= 0 && hasArmyExperienceSource)
        creatureCount = army->count[armySlotIndex];
    creatureCount = std::max(1, creatureCount);

    H3LoadedDef *skillsDef = H3LoadedDef::Load(CREATURE_EXP_DEF);
    if (!skillsDef || !skillsDef->groups || !skillsDef->groups[0] || skillsDef->groups[0]->count <= 0)
    {
        if (skillsDef)
            skillsDef->Dereference();
        return FALSE;
    }

    H3LoadedPcx16 *source = H3LoadedPcx16::Create(skillsDef->widthDEF, skillsDef->heightDEF);
    if (!source)
    {
        skillsDef->Dereference();
        return FALSE;
    }

    struct PreparedSkill
    {
        int slot = -1;
        int frame = -1;
        std::string text;
        std::string hint;
    };

    auto readPreparedSkills = [&](int preparedExperience, DWORD preparedCrExpo) {
        std::vector<PreparedSkill> result;

        CDECL_5(void, 0x71EF2B, creatureId, creatureCount, preparedExperience, preparedCrExpo, hero ? 1 : 0);
        auto *info = reinterpret_cast<_DlgCreatureExpoInfo_ *>(0x845880);
        const int count = Clamp(0, info->IcoPropertiesCount, 6);
        if (!count || !info->IcoProperties)
            return result;

        result.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            const int frame = reinterpret_cast<int>(info->IcoProperties[i]);
            if (frame < 0 || frame >= skillsDef->groups[0]->count)
                continue;

            const char *propertyText = info->TxtProperties ? info->TxtProperties[i] : nullptr;
            const char *propertyHint = info->HintProperties ? info->HintProperties[i] : nullptr;

            PreparedSkill skill;
            skill.slot = i;
            skill.frame = frame;
            skill.text = propertyText && *propertyText ? propertyText : (propertyHint ? propertyHint : h3_NullString);
            skill.hint = propertyHint && *propertyHint ? propertyHint : skill.text;
            result.push_back(skill);
        }
        return result;
    };

    std::vector<PreparedSkill> preparedSkills;

    if (!isPreviewWithoutCrExpo)
    {
        // Concrete stack: native WoG preparation already chooses the correct
        // frame for every currently displayed property.
        preparedSkills = readPreparedSkills(experience, crExpo);
    }
    else
    {
        // Purchase dialog: emulate a valid zero-experience CrExpo in local
        // memory. The record is read-only from the game's point of view and is
        // never inserted into CrExpoSet.
        PreviewCrExpo previewCrExpo;
        previewCrExpo.experience = 0;
        previewCrExpo.count = creatureCount;
        previewCrExpo.flags = MakePreviewCrExpoFlags(creatureId);
        const DWORD previewCrExpoPtr = reinterpret_cast<DWORD>(&previewCrExpo);

        // CrExpBonLine already contains the configured values for all 11 ranks.
        // Temporarily project each rank into Lvls[0], ask the native WoG UI
        // preparation code to describe that rank, and take the union by icon
        // frame. This avoids hard-coding rank experience thresholds and also
        // supports custom Crexpbon.txt configurations where abilities disappear
        // again at later ranks.
        char originalRank0[CR_EXP_BONUS_LINES] = {};
        for (int lineIndex = 0; lineIndex < CR_EXP_BONUS_LINES; ++lineIndex)
            originalRank0[lineIndex] = GetCrExpBonusLine(lineIndex)->Lvls[0];

        for (int rank = 0; rank <= 10; ++rank)
        {
            for (int lineIndex = 0; lineIndex < CR_EXP_BONUS_LINES; ++lineIndex)
            {
                CrExpBonLine *line = GetCrExpBonusLine(lineIndex);
                line->Lvls[0] = line->Lvls[rank];
            }

            const auto rankSkills = readPreparedSkills(0, previewCrExpoPtr);
            for (const auto &skill : rankSkills)
            {
                // IcoProperties has six logical property slots. The native
                // code may use different DEF frames for inactive/active states
                // of the same slot. Union by slot, not by frame, otherwise the
                // same skill is added twice and spills into a second row.
                //
                // Ranks are processed from 0 upward, so when rank 0 already
                // supplies an inactive frame we keep exactly that native frame.
                const auto found = std::find_if(preparedSkills.begin(), preparedSkills.end(),
                                                [&](const PreparedSkill &other) {
                                                    return other.slot == skill.slot;
                                                });
                if (found == preparedSkills.end())
                    preparedSkills.push_back(skill);
            }
        }

        // Always restore the shared WoG bonus table before creating any widgets.
        for (int lineIndex = 0; lineIndex < CR_EXP_BONUS_LINES; ++lineIndex)
            GetCrExpBonusLine(lineIndex)->Lvls[0] = originalRank0[lineIndex];
    }

    creatureSkills.reserve(preparedSkills.size());
    for (const auto &prepared : preparedSkills)
    {
        H3LoadedPcx16 *picture = H3LoadedPcx16::Create(EXP_SKILL_ICON_SIZE, EXP_SKILL_ICON_SIZE);
        if (!picture)
            continue;

        source->FillRectangle(0, 0, source->width, source->height, 0, 0, 0);
        skillsDef->DrawToPcx16(0, prepared.frame, source, 0, 0);
        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(
            picture, source, source->width, source->height, 0, 0, EXP_SKILL_ICON_SIZE, EXP_SKILL_ICON_SIZE);

        // Do not synthesize an inactive appearance here. DlgCrExp.def already
        // contains the frame selected by native WoG preparation for this state.
        CreatureSkill skill;
        skill.frame = prepared.frame;
        skill.text = prepared.text;
        skill.hint = prepared.hint;
        skill.pcx16 = picture;
        creatureSkills.push_back(skill);
    }

    source->Destroy();
    skillsDef->Dereference();
    return !creatureSkills.empty();
}

EXTERN_C __declspec(dllexport) void AddExternalCreatureSkill(const char *name, const char *description,
                                                             const char *pcx16Name)
{
    // Reserved ABI entry point. Creature Experience skills are currently read
    // from WoG's prepared _DlgCreatureExpoInfo_ structure.
}
