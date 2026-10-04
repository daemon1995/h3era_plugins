#include "pch.h"
#include "MonPreview.h"
#include "PluginSettings.h"

#include <algorithm>
#include <array>
#include <vector>

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

struct PanelItems
{
    H3DlgPcx *background = nullptr;
    std::array<H3DlgText *, STATUS_ROWS> statusValues = {};
    std::array<H3DlgPcx16 *, SPELL_SLOT_COUNT> spellPictures = {};
    std::array<H3DlgText *, SPELL_SLOT_COUNT> spellDurations = {};
};

PanelItems CollectPanelItems(H3CombatMonsterPanel *panel)
{
    PanelItems result;
    if (!panel)
        return result;

    for (H3DlgItem *item : panel->GetItems())
    {
        if (!item)
            continue;

        const int id = item->GetID();
        if (id == EXTENSION_BG_ID)
        {
            result.background = item->Cast<H3DlgPcx>();
            continue;
        }

        if (id >= STATUS_FIRST_VALUE_ID &&
            id < STATUS_FIRST_VALUE_ID + STATUS_ROWS * 2 &&
            ((id - STATUS_FIRST_VALUE_ID) & 1) == 0)
        {
            result.statusValues[(id - STATUS_FIRST_VALUE_ID) / 2] = item->Cast<H3DlgText>();
            continue;
        }

        if (id >= SPELL_FIRST_ITEM_ID && id < SPELL_FIRST_ITEM_ID + SPELL_SLOT_COUNT * 2)
        {
            const int offset = id - SPELL_FIRST_ITEM_ID;
            const int slot = offset / 2;
            if ((offset & 1) == 0)
                result.spellPictures[slot] = item->Cast<H3DlgPcx16>();
            else
                result.spellDurations[slot] = item->Cast<H3DlgText>();
        }
    }

    return result;
}

void SetActionStatus(H3CombatCreature *stack, H3DlgText *text)
{
    if (!stack || !text)
        return;

    if (stack->IsWaiting())
        text->SetText(Era::tr("eci.combat_dialog.wait"));
    else if (stack->IsDefending())
        text->SetText(Era::tr("eci.combat_dialog.def"));
    else if (stack->IsDone())
        text->SetText(Era::tr("eci.combat_dialog.done"));
    else
        text->SetText(Era::tr("eci.combat_dialog.active"));
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

    CreatePanelBackground();

    _pi->WriteHiHook(0x46CA00, THISCALL_, H3CombatMonsterPanel_Ctor);
    _pi->WriteHiHook(0x46D780, THISCALL_, H3CombatMonsterPanel_Prepare);
    _pi->WriteHiHook(0x46D710, THISCALL_, H3CombatMonsterPanel_Dtor);

    Era::RegisterHandler(OnBeforeBattleUniversal, "OnBeforeBattleUniversal");
    Era::RegisterHandler(OnAfterBattleUniversal, "OnAfterBattleUniversal");

    m_isInited = true;
}

void __stdcall MonPreview::OnBeforeBattleUniversal(Era::TEvent *event)
{
    auto &self = Get();

    // A replay/restart can enter another before-battle phase without a paired
    // after-battle callback. Always rebuild from a clean state.
    self.DetachAllPanelSpellImages();
    self.DestroySpellEffectResources();
    self.CreateSpellEffectResources();
}

void __stdcall MonPreview::OnAfterBattleUniversal(Era::TEvent *event)
{
    auto &self = Get();

    // Slots hold raw pointers. Detach them first so H3DlgPcx16::~H3DlgPcx16
    // cannot Dereference() a shared cache object after we release it here.
    self.DetachAllPanelSpellImages();
    self.DestroySpellEffectResources();
}

void MonPreview::EnsureSpellEffectResources()
{
    if (resizedSpellPictures.empty())
        CreateSpellEffectResources();
}

void MonPreview::CreateSpellEffectResources()
{
    if (!resizedSpellPictures.empty())
        return;

    H3DefLoader spellsDef("spellint.def");
    if (!spellsDef.Get() || !spellsDef->groups || !spellsDef->groups[0] ||
        spellsDef->groups[0]->count <= 0 || spellsDef->widthDEF <= 0 || spellsDef->heightDEF <= 0)
        return;

    const int frameCount = spellsDef->groups[0]->count;
    spellIconWidth = std::max(1, spellsDef->widthDEF / 2);
    spellIconHeight = std::max(1, spellsDef->heightDEF / 2);
    resizedSpellPictures.assign(frameCount, nullptr);

    H3LoadedPcx16 *source = H3LoadedPcx16::Create(spellsDef->widthDEF, spellsDef->heightDEF);
    if (!source)
    {
        resizedSpellPictures.clear();
        spellIconWidth = 0;
        spellIconHeight = 0;
        return;
    }

    for (int frame = 0; frame < frameCount; ++frame)
    {
        H3LoadedPcx16 *picture = H3LoadedPcx16::Create(spellIconWidth, spellIconHeight);
        if (!picture)
            continue;

        source->FillRectangle(0, 0, source->width, source->height, 0, 0, 0);
        picture->FillRectangle(0, 0, picture->width, picture->height, 0, 0, 0);
        spellsDef->DrawToPcx16(0, frame, source, 0, 0);

        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(
            picture, source, source->width, source->height,
            0, 0, picture->width, picture->height);

        resizedSpellPictures[frame] = picture;
    }

    source->Destroy();
}

void MonPreview::DestroySpellEffectResources()
{
    for (H3LoadedPcx16 *picture : resizedSpellPictures)
        if (picture)
            picture->Destroy();

    resizedSpellPictures.clear();
    spellIconWidth = 0;
    spellIconHeight = 0;
}

void MonPreview::RegisterPanel(H3CombatMonsterPanel *panel)
{
    if (!panel)
        return;

    if (std::find(activePanels.begin(), activePanels.end(), panel) == activePanels.end())
        activePanels.push_back(panel);
}

void MonPreview::UnregisterPanel(H3CombatMonsterPanel *panel)
{
    activePanels.erase(std::remove(activePanels.begin(), activePanels.end(), panel), activePanels.end());
}

void MonPreview::DetachPanelSpellImages(H3CombatMonsterPanel *panel)
{
    if (!panel)
        return;

    for (H3DlgItem *item : panel->GetItems())
    {
        if (!item)
            continue;

        const int id = item->GetID();
        if (id < SPELL_FIRST_ITEM_ID || id >= SPELL_FIRST_ITEM_ID + SPELL_SLOT_COUNT * 2)
            continue;

        const int offset = id - SPELL_FIRST_ITEM_ID;
        if ((offset & 1) == 0)
        {
            H3DlgPcx16 *picture = item->Cast<H3DlgPcx16>();
            picture->SetPcx(nullptr);
            picture->HideDeactivate();
        }
        else
        {
            H3DlgText *duration = item->Cast<H3DlgText>();
            duration->SetText(h3_NullString);
            duration->HideDeactivate();
        }
    }
}

void MonPreview::DetachAllPanelSpellImages()
{
    for (H3CombatMonsterPanel *panel : activePanels)
        DetachPanelSpellImages(panel);
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
    const bool enabled = creatureInfo::GetPluginSettings().showExpandedBattleMonsterPanel;
    if (type == 1 && enabled)
    {
        height += PANEL_HEIGHT_ADD;
        y = std::max(0, y - PANEL_HEIGHT_ADD + PANEL_Y_CORRECTION);
    }

    H3CombatMonsterPanel *result = THISCALL_7(H3CombatMonsterPanel *, hook->GetDefaultFunc(), panel,
                                               x, y, width, height, parent, type);
    if (result && type == 1 && enabled)
    {
        auto &self = Get();
        // Normally OnBeforeBattleUniversal has already prepared the shared
        // frames. Keep this lazy fallback for custom battle entry paths.
        self.EnsureSpellEffectResources();
        self.BuildExtendedPanel(result);
        self.RegisterPanel(result);
    }
    return result;
}

void __stdcall MonPreview::H3CombatMonsterPanel_Dtor(HiHook *hook, H3CombatMonsterPanel *panel)
{
    auto &self = Get();
    self.DetachPanelSpellImages(panel);
    self.UnregisterPanel(panel);

    THISCALL_1(void, hook->GetDefaultFunc(), panel);
}

void MonPreview::BuildExtendedPanel(H3CombatMonsterPanel *panel)
{
    if (!panel)
        return;

    for (H3DlgItem *item : panel->GetItems())
        if (item && item->GetID() >= 2215 && item->GetID() <= 2218)
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
        H3String key("eci.combat_dialog.stats.");
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

    EnsureSpellEffectResources();
    const int iconWidth = spellIconWidth > 0 ? spellIconWidth : 16;
    const int iconHeight = spellIconHeight > 0 ? spellIconHeight : 16;

    // Exactly twelve persistent widget slots per panel. The widgets themselves
    // own no image. Prepare() only points them at one of the shared battle
    // cache frames, or nullptr for an empty slot.
    for (int slot = 0; slot < SPELL_SLOT_COUNT; ++slot)
    {
        const int x = SPELL_X + (slot % SPELL_COLUMNS) * SPELL_X_STEP;
        const int y = SPELL_Y + (slot / SPELL_COLUMNS) * SPELL_Y_STEP;
        const int pictureId = SPELL_FIRST_ITEM_ID + slot * 2;
        const int durationId = pictureId + 1;

        H3DlgPcx16 *picture = H3DlgPcx16::Create(x, y, iconWidth, iconHeight, pictureId, nullptr);
        if (picture)
        {
            // Frame 0 of spellint.def is the normal "no spell" backing.
            // Keep every slot visible even when the current creature has no
            // effect assigned to it.
            H3LoadedPcx16 *emptyPicture = resizedSpellPictures.empty() ? nullptr : resizedSpellPictures[0];
            picture->SetPcx(emptyPicture);
            if (emptyPicture)
                picture->ShowActivate();
            else
                picture->HideDeactivate();
            panel->AddItem(picture);
        }

        // Keep the original overlay geometry: it is intentionally slightly
        // wider than the icon, so xN remains readable over small spell art.
        H3DlgText *duration = H3DlgText::Create(x + 10, y + 8, 24, 12,
                                                h3_NullString, NH3Dlg::Text::TINY,
                                                eTextColor::WHITE, durationId,
                                                eTextAlignment::MIDDLE_RIGHT);
        if (duration)
        {
            duration->HideDeactivate();
            panel->AddItem(duration);
        }
    }
}

void __stdcall MonPreview::H3CombatMonsterPanel_Prepare(HiHook *hook, H3CombatMonsterPanel *panel,
                                                         H3CombatCreature *stack, H3Hero *hero)
{
    THISCALL_3(void, hook->GetDefaultFunc(), panel, stack, hero);
    if (panel && stack && creatureInfo::GetPluginSettings().showExpandedBattleMonsterPanel &&
        panel->GetHeight() > 288)
        Get().UpdateExtendedPanel(panel, stack, hero);
}

void MonPreview::UpdateExtendedPanel(H3CombatMonsterPanel *panel, H3CombatCreature *stack, H3Hero *hero)
{
    if (!panel || !stack)
        return;

    const PanelItems items = CollectPanelItems(panel);

    if (H3DlgPcx *background = items.background)
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

    SetActionStatus(stack, items.statusValues[0]);

    if (H3DlgText *retaliations = items.statusValues[1])
    {
        if (stack->retaliations > 200)
            retaliations->SetText("99+");
        else
            SetNumber(retaliations, stack->retaliations);
    }

    if (H3DlgText *shots = items.statusValues[2])
    {
        if (stack->info.flags & 4)
            SetNumber(shots, stack->info.numberShots);
        else
            shots->SetText("-");
    }

    SetNumber(items.statusValues[3], stack->info.spellCharges);
    UpdateSpellSlots(items.spellPictures, items.spellDurations, stack);
}

void MonPreview::UpdateSpellSlots(const std::array<H3DlgPcx16 *, SPELL_SLOT_COUNT> &pictures,
                                  const std::array<H3DlgText *, SPELL_SLOT_COUNT> &durations,
                                  H3CombatCreature *stack)
{
    std::array<int, SPELL_SLOT_COUNT> spells;
    spells.fill(-1);

    int count = 0;
    const int durationCount = sizeof(stack->activeSpellDuration) / sizeof(stack->activeSpellDuration[0]);
    for (int spellId = durationCount - 1; spellId >= 0 && count < SPELL_SLOT_COUNT; --spellId)
        if (stack->activeSpellDuration[spellId] > 0)
            spells[count++] = spellId;

    for (int slot = 0; slot < SPELL_SLOT_COUNT; ++slot)
    {
        H3DlgPcx16 *picture = pictures[slot];
        H3DlgText *duration = durations[slot];
        const int spellId = spells[slot];

        if (!picture)
        {
            if (duration)
            {
                duration->SetText(h3_NullString);
                duration->HideDeactivate();
            }
            continue;
        }

        if (spellId < 0)
        {
            // Empty slots must still draw spellint.def frame 0. This is the
            // regular backing used by the original panel, not an absence of
            // a widget/image.
            H3LoadedPcx16 *emptyPicture = resizedSpellPictures.empty() ? nullptr : resizedSpellPictures[0];
            picture->SetPcx(emptyPicture);
            picture->SetHints(h3_NullString, h3_NullString, TRUE);
            if (emptyPicture)
                picture->ShowActivate();
            else
                picture->HideDeactivate();

            if (duration)
            {
                duration->SetText(h3_NullString);
                duration->HideDeactivate();
            }
            continue;
        }

        const int frame = spellId + 1;
        H3LoadedPcx16 *spellPicture =
            frame >= 0 && frame < static_cast<int>(resizedSpellPictures.size())
                ? resizedSpellPictures[frame]
                : nullptr;

        if (!spellPicture)
        {
            // A missing/corrupt spell frame falls back to frame 0 instead of
            // drawing a black rectangle or retaining the previous creature's
            // image.
            H3LoadedPcx16 *emptyPicture = resizedSpellPictures.empty() ? nullptr : resizedSpellPictures[0];
            picture->SetPcx(emptyPicture);
            picture->SetHints(h3_NullString, h3_NullString, TRUE);
            if (emptyPicture)
                picture->ShowActivate();
            else
                picture->HideDeactivate();

            if (duration)
            {
                duration->SetText(h3_NullString);
                duration->HideDeactivate();
            }
            continue;
        }

        picture->SetPcx(spellPicture);
        SetSpellHints(picture, stack, spellId);
        picture->ShowActivate();

        if (!duration)
            continue;

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

} // namespace preview
