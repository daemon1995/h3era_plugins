#include "pch.h"
#include "MonPreview.h"

#include <algorithm>
#include <array>

using namespace h3;

namespace preview
{
namespace
{
constexpr int PANEL_HEIGHT_ADD = 86;
constexpr int PANEL_Y_CORRECTION = 13;
constexpr int EXTENSION_BG_ID = 4090;

constexpr int STATUS_FIRST_LABEL_ID = 4004;
constexpr int STATUS_FIRST_VALUE_ID = 4005;
constexpr int STATUS_ROWS = 4;
constexpr int STATUS_X = 9;
constexpr int STATUS_Y = 160;
constexpr int STATUS_LINE_HEIGHT = 12;

constexpr int SPELL_FIRST_ITEM_ID = 4100;
constexpr int SPELL_SLOT_COUNT = 12;
constexpr int SPELL_COLUMNS = 2;
constexpr int SPELL_X = 5;
constexpr int SPELL_Y = 254;
constexpr int SPELL_X_STEP = 34;
constexpr int SPELL_Y_STEP = 19;

bool HasVisibleDuration(int spellId)
{
    return spellId != eSpell::BERSERK && spellId != eSpell::DISRUPTING_RAY && spellId != eSpell::BIND;
}

void SetActionStatus(H3CombatCreature *stack, H3DlgText *text)
{
    if (!stack || !text)
        return;

    if (stack->IsWaiting())
        text->SetText(Era::tr("gem_plugin.combat_dlg.wait"));
    else if (stack->IsDefending())
        text->SetText(Era::tr("gem_plugin.combat_dlg.def"));
    else if (stack->IsDone())
        text->SetText(Era::tr("gem_plugin.combat_dlg.done"));
    else
        text->SetText(Era::tr("gem_plugin.combat_dlg.active"));
}

void SetNumber(H3DlgText *text, int value)
{
    if (!text)
        return;
    H3String str;
    str.Append(value);
    text->SetText(str.String());
}

int ResolvePanelColor(H3CombatCreature *stack, H3Hero *hero)
{
    if (hero && hero->owner >= 0 && hero->owner < 8)
        return hero->owner;

    if (stack && stack->side >= 0 && stack->side < 2 && P_CombatManager)
    {
        const int player = P_CombatManager->heroOwner[stack->side];
        if (player >= 0 && player < 8)
            return player;
    }

    if (P_Game)
    {
        const int player = P_Game->GetPlayerID();
        if (player >= 0 && player < 8)
            return player;
    }
    return -1;
}

void SetSpellHints(H3DlgItem *item, H3CombatCreature *stack, int spellId)
{
    if (!item || !stack || spellId < 0)
        return;

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
    item->SetHints(h3_TextBuffer, spellDesc.String(), TRUE);
}

} // namespace

MonPreview *MonPreview::instance = nullptr;

MonPreview &MonPreview::Get()
{
    if (!instance)
        instance = new MonPreview();
    return *instance;
}

MonPreview::MonPreview() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.ExtendedMonPreviewPanel.daemon_n"))
{
    CreatePatches();
}

void MonPreview::CreatePatches()
{
    if (m_isInited)
        return;

    // Resources are prepared before hooks become reachable.
    CreateResizedSpellEffectPictures();
    CreatePanelBackground();

    _pi->WriteHiHook(0x46CA00, THISCALL_, H3CombatMonsterPanel_Ctor);
    _pi->WriteHiHook(0x46D780, THISCALL_, H3CombatMonsterPanel_Prepare);
    m_isInited = true;
}

void MonPreview::CreateResizedSpellEffectPictures()
{
    H3DefLoader spellsDef("spellint.def");
    if (!spellsDef.Get() || !spellsDef->groups || !spellsDef->groups[0] || spellsDef->widthDEF <= 0 ||
        spellsDef->heightDEF <= 0)
        return;

    const int width = spellsDef->widthDEF;
    const int height = spellsDef->heightDEF;
    const int smallWidth = std::max(1, width / 2);
    const int smallHeight = std::max(1, height / 2);
    const int framesNum = spellsDef->groups[0]->count;
    resizedSpellPictures.Resize(framesNum);

    H3LoadedPcx16 *source = H3LoadedPcx16::Create(width, height);
    if (!source)
        return;

    for (int frame = 0; frame < framesNum; ++frame)
    {
        H3LoadedPcx16 *resized = H3LoadedPcx16::Create(smallWidth, smallHeight);
        if (!resized)
            continue;

        source->FillRectangle(0, 0, source->width, source->height, 0, 0, 0);
        spellsDef->DrawToPcx16(0, frame, source, 0, 0);
        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(
            resized, source, width, height, 0, 0, smallWidth, smallHeight);
        resizedSpellPictures[frame] = resized;
    }

    source->Destroy();
}

void MonPreview::CreatePanelBackground()
{
    H3PcxLoader original("CCrPop.pcx");
    if (!original.Get() || original->width <= 0 || original->height <= 70)
        return;

    const int sourceY = 70;
    extensionBackground = H3LoadedPcx::Create(h3_NullString, original->width, original->height - sourceY);
    if (extensionBackground)
        original->DrawToPcx(0, sourceY, original->width, original->height - sourceY,
                            extensionBackground, 0, 0, TRUE);
}

H3CombatMonsterPanel *__stdcall MonPreview::H3CombatMonsterPanel_Ctor(HiHook *hook, H3CombatMonsterPanel *panel, int x,
                                                                      int y, int width, int height, H3BaseDlg *parent,
                                                                      DWORD type)
{
    if (type == 1)
    {
        height += PANEL_HEIGHT_ADD;
        y = std::max(0, y - PANEL_HEIGHT_ADD + PANEL_Y_CORRECTION);
    }

    H3CombatMonsterPanel *result = THISCALL_7(H3CombatMonsterPanel *, hook->GetDefaultFunc(), panel,
                                               x, y, width, height, parent, type);
    if (result && type == 1)
        Get().BuildExtendedPanel(result);
    return result;
}

void MonPreview::BuildExtendedPanel(H3CombatMonsterPanel *panel)
{
    if (!panel)
        return;

    // The original panel has only three 48x36 spell slots. They sit below y=169
    // and would be covered by the extension background, so disable them instead
    // of leaving invisible mouse-active controls behind it.
    for (int id = 2215; id <= 2218; ++id)
        if (auto *item = panel->GetH3DlgItem(id))
            item->HideDeactivate();

    if (extensionBackground)
    {
        H3LoadedPcx *backgroundCopy = H3LoadedPcx::Create(h3_NullString,
                                                          extensionBackground->width,
                                                          extensionBackground->height);
        if (backgroundCopy)
        {
            extensionBackground->DrawToPcx(0, 0, extensionBackground->width, extensionBackground->height,
                                            backgroundCopy, 0, 0, TRUE);
            H3DlgPcx *background = H3DlgPcx::Create(0, 70 + PANEL_HEIGHT_ADD,
                                                    backgroundCopy->width, backgroundCopy->height,
                                                    EXTENSION_BG_ID, nullptr);
            if (background)
            {
                background->SetPcx(backgroundCopy);
                panel->AddItem(background);
            }
            else
                backgroundCopy->Dereference();
        }
    }

    for (int row = 0; row < STATUS_ROWS; ++row)
    {
        H3String key("gem_plugin.combat_dlg.");
        key.Append(row);
        const int y = STATUS_Y + row * STATUS_LINE_HEIGHT;
        const int labelId = STATUS_FIRST_LABEL_ID + row * 2;
        const int valueId = STATUS_FIRST_VALUE_ID + row * 2;

        if (H3DlgText *label = H3DlgText::Create(STATUS_X, y, 34, STATUS_LINE_HEIGHT,
                                                 Era::tr(key.String()), NH3Dlg::Text::TINY, 1,
                                                 labelId, eTextAlignment::MIDDLE_LEFT))
            panel->AddItem(label);

        if (H3DlgText *value = H3DlgText::Create(STATUS_X, y, 60, STATUS_LINE_HEIGHT,
                                                 h3_NullString, NH3Dlg::Text::TINY, 1,
                                                 valueId, eTextAlignment::MIDDLE_RIGHT))
            panel->AddItem(value);
    }

    if (!resizedSpellPictures.Size() || !resizedSpellPictures[0])
        return;

    const int iconWidth = resizedSpellPictures[0]->width;
    const int iconHeight = resizedSpellPictures[0]->height;
    for (int slot = 0; slot < SPELL_SLOT_COUNT; ++slot)
    {
        const int x = SPELL_X + (slot % SPELL_COLUMNS) * SPELL_X_STEP;
        const int y = SPELL_Y + (slot / SPELL_COLUMNS) * SPELL_Y_STEP;
        const int pictureId = SPELL_FIRST_ITEM_ID + slot * 2;
        const int durationId = pictureId + 1;

        H3LoadedPcx16 *slotPcx = H3LoadedPcx16::Create(iconWidth, iconHeight);
        H3DlgPcx16 *picture = slotPcx ? H3DlgPcx16::Create(x, y, iconWidth, iconHeight, pictureId, nullptr) : nullptr;
        if (picture)
        {
            picture->SetPcx(slotPcx);
            panel->AddItem(picture);
            picture->HideDeactivate();
        }
        else if (slotPcx)
            slotPcx->Destroy();

        H3DlgText *duration = H3DlgText::Create(x + 10, y + std::max(0, iconHeight - 10), 24, 10,
                                                h3_NullString, NH3Dlg::Text::TINY, 1, durationId,
                                                eTextAlignment::BOTTOM_RIGHT);
        if (duration)
        {
            panel->AddItem(duration);
            duration->HideDeactivate();
        }
    }
}

void __stdcall MonPreview::H3CombatMonsterPanel_Prepare(HiHook *hook, H3CombatMonsterPanel *panel,
                                                         H3CombatCreature *stack, H3Hero *hero)
{
    THISCALL_3(void, hook->GetDefaultFunc(), panel, stack, hero);
    if (panel && stack && panel->GetHeight() > 288)
        Get().UpdateExtendedPanel(panel, stack, hero);
}

void MonPreview::UpdateExtendedPanel(H3CombatMonsterPanel *panel, H3CombatCreature *stack, H3Hero *hero)
{
    if (!panel || !stack)
        return;

    if (H3DlgPcx *background = panel->GetPcx(EXTENSION_BG_ID))
    {
        H3LoadedPcx *target = background->GetPcx();
        if (extensionBackground && target)
        {
            extensionBackground->DrawToPcx(0, 0, extensionBackground->width, extensionBackground->height,
                                            target, 0, 0, TRUE);
            const int player = ResolvePanelColor(stack, hero);
            if (player >= 0)
                background->AdjustColor(player);
        }
    }

    SetActionStatus(stack, panel->GetText(4005));

    if (H3DlgText *retaliations = panel->GetText(4007))
    {
        if (stack->retaliations > 200)
            retaliations->SetText("99+");
        else
            SetNumber(retaliations, stack->retaliations);
    }

    if (H3DlgText *shots = panel->GetText(4009))
    {
        if (stack->info.flags & 4)
            SetNumber(shots, stack->info.numberShots);
        else
            shots->SetText("-");
    }

    SetNumber(panel->GetText(4011), stack->info.spellCharges);
    UpdateSpellSlots(panel, stack);
}

void MonPreview::UpdateSpellSlots(H3CombatMonsterPanel *panel, H3CombatCreature *stack)
{
    std::array<int, SPELL_SLOT_COUNT> spells;
    spells.fill(-1);

    int count = 0;
    const int durationCount = sizeof(stack->activeSpellDuration) / sizeof(stack->activeSpellDuration[0]);
    for (int spellId = durationCount - 1; spellId >= 0 && count < SPELL_SLOT_COUNT; --spellId)
        if (stack->activeSpellDuration[spellId])
            spells[count++] = spellId;

    for (int slot = 0; slot < SPELL_SLOT_COUNT; ++slot)
    {
        H3DlgPcx16 *picture = panel->GetPcx16(SPELL_FIRST_ITEM_ID + slot * 2);
        H3DlgText *duration = panel->GetText(SPELL_FIRST_ITEM_ID + slot * 2 + 1);
        const int spellId = spells[slot];

        if (spellId < 0 || !picture)
        {
            if (picture)
                picture->HideDeactivate();
            if (duration)
                duration->HideDeactivate();
            continue;
        }

        const int frame = spellId + 1;
        if (frame < 0 || frame >= static_cast<int>(resizedSpellPictures.Size()) || !resizedSpellPictures[frame])
        {
            picture->HideDeactivate();
            if (duration)
                duration->HideDeactivate();
            continue;
        }

        if (H3LoadedPcx16 *slotPcx = picture->GetPcx())
            slotPcx->CopyRegion(resizedSpellPictures[frame], 0, 0);
        SetSpellHints(picture, stack, spellId);
        picture->ShowActivate();

        if (duration)
        {
            if (HasVisibleDuration(spellId))
            {
                H3String text("x");
                text.Append(stack->activeSpellDuration[spellId]);
                duration->SetText(text.String());
                SetSpellHints(duration, stack, spellId);
                duration->ShowActivate();
            }
            else
            {
                duration->SetText(h3_NullString);
                duration->HideDeactivate();
            }
        }
    }
}

} // namespace preview
