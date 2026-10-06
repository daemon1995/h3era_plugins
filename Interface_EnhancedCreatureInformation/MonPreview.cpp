#include "pch.h"
#include "MonPreview.h"
#include "PluginSettings.h"
#include "CreatureSpellEffects.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

using namespace h3;
using namespace creatureInfo;

namespace preview
{
namespace
{
constexpr int PANEL_HEIGHT_ADD = 86;
constexpr int PANEL_Y_CORRECTION = 13;
constexpr int EXPANDED_PANEL_TYPE = 1;
constexpr int NATIVE_PANEL_HEIGHT = 288;
constexpr int BACKGROUND_SOURCE_Y = 70;
constexpr int HEALTH_LOSSES_Y = 131;
constexpr int MORALE_LUCK_Y = 217;
constexpr int LABEL_WIDTH = 34;
constexpr int VALUE_WIDTH = 60;
constexpr int EMPTY_SPELL_FRAME = 0;
constexpr int DEFAULT_SPELL_ICON_SIZE = 16;
constexpr int DURATION_X_OFFSET = 10;
constexpr int DURATION_Y_OFFSET = 8;
constexpr int DURATION_WIDTH = 24;
constexpr int DURATION_HEIGHT = 12;
constexpr int RETALIATION_DISPLAY_LIMIT = 200;
constexpr int NUMBER_BUFFER_SIZE = 16;
constexpr int COMBAT_SIDE_COUNT = 2;
constexpr DWORD PANEL_CTOR_FUNCTION = 0x46CA00;
constexpr DWORD PANEL_PREPARE_FUNCTION = 0x46D780;
constexpr DWORD PANEL_DTOR_FUNCTION = 0x46D710;
constexpr DWORD PANEL_REDRAW_FUNCTION = 0x5AA800;
constexpr DWORD PANEL_ADD_ITEM_FUNCTION = 0x5AA7B0;

constexpr int STATUS_ROWS = BATTLE_PREVIEW_STATUS_ROWS;
constexpr int STATUS_X = 9;
constexpr int STATUS_Y = 160;
constexpr int STATUS_LINE_HEIGHT = 12;

constexpr int SPELL_SLOT_COUNT = BATTLE_PREVIEW_SPELL_SLOTS;
constexpr int SPELL_COLUMNS = 2;
constexpr int SPELL_X = 5;
constexpr int SPELL_Y = 254;
constexpr int SPELL_X_STEP = 34;
constexpr int SPELL_Y_STEP = 19;
constexpr LPCSTR STATUS_LABEL_KEYS[] = {
    "eci.combat_dialog.stats.0", "eci.combat_dialog.stats.1",
    "eci.combat_dialog.stats.2", "eci.combat_dialog.stats.3"
};
static_assert(sizeof(STATUS_LABEL_KEYS) / sizeof(STATUS_LABEL_KEYS[0]) == STATUS_ROWS,
              "Missing battle status label");

class PanelItemDrawOrder : public H3DlgItem
{
  public:
    static int GetDrawOrder(const H3DlgItem *item) { return item->*(&PanelItemDrawOrder::zOrder); }
};

void AddPanelBackground(H3CombatMonsterPanel *panel, H3DlgPcx *background)
{
    auto &items = panel->GetItems();
    if (items.IsEmpty())
    {
        panel->AddItem(background);
        return;
    }
    // Match the native backdrop's drawing layer. The parent dialog and the
    // panel both draw this copy before the relocated native morale/luck items.
    const int order = PanelItemDrawOrder::GetDrawOrder(items[0]);
    if (!items.Insert(items.begin() + 1, background))
    {
        background->Destroy();
        return;
    }
    background->HideDeactivate();
    THISCALL_3(H3DlgItem *, PANEL_ADD_ITEM_FUNCTION, panel, background, order);
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
    char buffer[NUMBER_BUFFER_SIZE];
    std::snprintf(buffer, sizeof(buffer), "%d", value);
    text->SetText(buffer);
}

int GetCurrentUnitHealth(const H3CombatCreature *stack)
{
    return stack->numberAlive > 0 ? std::max(0, stack->info.hitPoints - stack->healthLost) : 0;
}

int GetCreatureLosses(const H3CombatCreature *stack)
{
    return std::max(0, stack->numberAtStart - stack->numberAlive);
}

void AddStatisticRow(H3CombatMonsterPanel *panel, int y, int labelId, int valueId, LPCSTR labelText)
{
    if (auto *label = H3DlgText::Create(STATUS_X, y, LABEL_WIDTH, STATUS_LINE_HEIGHT, labelText,
                                      NH3Dlg::Text::TINY, eTextColor::WHITE, labelId, eTextAlignment::MIDDLE_LEFT))
        panel->AddItem(label);
    if (auto *value = H3DlgText::Create(STATUS_X, y, VALUE_WIDTH, STATUS_LINE_HEIGHT, h3_NullString,
                                      NH3Dlg::Text::TINY, eTextColor::WHITE, valueId, eTextAlignment::MIDDLE_RIGHT))
        panel->AddItem(value);
}

int ResolvePanelColor(H3CombatCreature *stack, H3Hero *hero)
{
    if (hero && hero->owner >= 0 && hero->owner < limits::PLAYERS)
        return hero->owner;

    if (stack && stack->side >= 0 && stack->side < COMBAT_SIDE_COUNT && P_CombatManager)
    {
        const int player = P_CombatManager->heroOwner[stack->side];
        if (player >= 0 && player < limits::PLAYERS)
            return player;
    }

    if (P_Game)
    {
        const int player = P_Game->GetPlayerID();
        if (player >= 0 && player < limits::PLAYERS)
            return player;
    }
    return -1;
}

} // namespace

MonPreview::PanelItems MonPreview::CollectPanelItems(H3CombatMonsterPanel *panel)
{
    PanelItems result;
    result.panel = panel;
    if (!panel)
        return result;

    for (H3DlgItem *item : panel->GetItems())
    {
        if (!item)
            continue;

        const int id = item->GetID();
        switch (id)
        {
        case HEALTH_VALUE_ID:
            result.health = item->Cast<H3DlgText>();
            continue;
        case LOSSES_VALUE_ID:
            result.losses = item->Cast<H3DlgText>();
            continue;
        case EXTENSION_BG_ID:
            result.background = item->Cast<H3DlgPcx>();
            continue;
        default:
            break;
        }
        if (id >= NATIVE_SPELL_FIRST_ID && id <= NATIVE_SPELL_LAST_ID)
        {
            result.nativeSpellItems[id - NATIVE_SPELL_FIRST_ID] = item;
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

    _pi->WriteHiHook(PANEL_CTOR_FUNCTION, THISCALL_, H3CombatMonsterPanel_Ctor);
    _pi->WriteHiHook(PANEL_PREPARE_FUNCTION, THISCALL_, H3CombatMonsterPanel_Prepare);
    _pi->WriteHiHook(PANEL_DTOR_FUNCTION, THISCALL_, H3CombatMonsterPanel_Dtor);
    _pi->WriteHiHook(PANEL_REDRAW_FUNCTION, THISCALL_, H3DlgBasePanel_Redraw);

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

    H3DefLoader spellsDef(NH3Dlg::Assets::SPELL_SMALL);
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

MonPreview::PanelItems *MonPreview::FindPanelItems(H3DlgBasePanel *panel)
{
    for (auto &items : activePanels)
        if (items.panel == panel)
            return &items;
    return nullptr;
}

void MonPreview::RegisterPanel(H3CombatMonsterPanel *panel)
{
    if (!panel)
        return;
    if (auto *items = FindPanelItems(panel))
        *items = CollectPanelItems(panel);
    else
        activePanels.push_back(CollectPanelItems(panel));
}

void MonPreview::UnregisterPanel(H3CombatMonsterPanel *panel)
{
    for (auto it = activePanels.begin(); it != activePanels.end(); ++it)
        if (it->panel == panel)
        {
            activePanels.erase(it);
            return;
        }
}
void MonPreview::DetachPanelSpellImages(H3CombatMonsterPanel *panel)
{
    auto *items = FindPanelItems(panel);
    if (!items)
        return;
    for (int slot = 0; slot < SPELL_SLOT_COUNT; ++slot)
    {
        if (auto *picture = items->spellPictures[slot])
        {
            picture->SetPcx(nullptr);
            picture->HideDeactivate();
        }
        if (auto *duration = items->spellDurations[slot])
        {
            duration->SetText(h3_NullString);
            duration->HideDeactivate();
        }
    }
}
void MonPreview::DetachAllPanelSpellImages()
{
    for (auto &items : activePanels)
        DetachPanelSpellImages(items.panel);
}

void MonPreview::CreatePanelBackground()
{
    H3PcxLoader original("CCrPop.pcx");
    if (!original.Get() || original->width <= 0 || original->height <= BACKGROUND_SOURCE_Y)
        return;

    const int sourceY = BACKGROUND_SOURCE_Y;
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
    if (type == EXPANDED_PANEL_TYPE && enabled)
    {
        height += PANEL_HEIGHT_ADD;
        y = std::max(0, y - PANEL_HEIGHT_ADD + PANEL_Y_CORRECTION);
    }

    H3CombatMonsterPanel *result = THISCALL_7(H3CombatMonsterPanel *, hook->GetDefaultFunc(), panel,
                                               x, y, width, height, parent, type);
    if (result && type == EXPANDED_PANEL_TYPE && enabled)
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
    {
        if (!item)
            continue;
        switch (item->GetID())
        {
        case NATIVE_MORALE_LABEL_ID:
        case NATIVE_MORALE_ICON_ID:
            item->SetY(panel->GetY() + MORALE_LUCK_Y);
            break;
        case NATIVE_LUCK_LABEL_ID:
        case NATIVE_LUCK_ICON_ID:
            item->SetY(panel->GetY() + MORALE_LUCK_Y + STATUS_LINE_HEIGHT);
            break;
        default:
            if (item->GetID() >= NATIVE_SPELL_FIRST_ID && item->GetID() <= NATIVE_SPELL_LAST_ID)
                item->HideDeactivate();
            break;
        }
    }

    if (extensionBackground)
    {
        H3LoadedPcx *backgroundCopy = H3LoadedPcx::Create(h3_NullString,
                                                          extensionBackground->width,
                                                          extensionBackground->height);
        if (backgroundCopy)
        {
            extensionBackground->DrawToPcx(0, 0, extensionBackground->width, extensionBackground->height,
                                            backgroundCopy, 0, 0, TRUE);
            H3DlgPcx *background = H3DlgPcx::Create(0, BACKGROUND_SOURCE_Y + PANEL_HEIGHT_ADD,
                                                    backgroundCopy->width, backgroundCopy->height,
                                                    EXTENSION_BG_ID, nullptr);
            if (background)
            {
                background->SetPcx(backgroundCopy);
                AddPanelBackground(panel, background);
            }
            else
                backgroundCopy->Dereference();
        }
    }

    // Reuse native morale/luck widgets below the extra statistics.
    AddStatisticRow(panel, HEALTH_LOSSES_Y, HEALTH_LABEL_ID, HEALTH_VALUE_ID, Era::tr("eci.combat_dialog.stats.4"));
    AddStatisticRow(panel, HEALTH_LOSSES_Y + STATUS_LINE_HEIGHT, LOSSES_LABEL_ID, LOSSES_VALUE_ID, Era::tr("eci.combat_dialog.stats.5"));

    for (int row = 0; row < STATUS_ROWS; ++row)
    {
        const int y = STATUS_Y + row * STATUS_LINE_HEIGHT;
        const int labelId = STATUS_FIRST_LABEL_ID + row * 2;
        const int valueId = STATUS_FIRST_VALUE_ID + row * 2;

        AddStatisticRow(panel, y, labelId, valueId, Era::tr(STATUS_LABEL_KEYS[row]));
    }

    EnsureSpellEffectResources();
    const int iconWidth = spellIconWidth > 0 ? spellIconWidth : DEFAULT_SPELL_ICON_SIZE;
    const int iconHeight = spellIconHeight > 0 ? spellIconHeight : DEFAULT_SPELL_ICON_SIZE;

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
            H3LoadedPcx16 *emptyPicture = resizedSpellPictures.empty() ? nullptr : resizedSpellPictures[EMPTY_SPELL_FRAME];
            picture->SetPcx(emptyPicture);
            if (emptyPicture)
                picture->ShowActivate();
            else
                picture->HideDeactivate();
            panel->AddItem(picture);
        }

        // Keep the original overlay geometry: it is intentionally slightly
        // wider than the icon, so xN remains readable over small spell art.
        H3DlgText *duration = H3DlgText::Create(x + DURATION_X_OFFSET, y + DURATION_Y_OFFSET, DURATION_WIDTH, DURATION_HEIGHT,
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
        panel->GetHeight() > NATIVE_PANEL_HEIGHT)
        Get().UpdateExtendedPanel(panel, stack, hero);
}

void __stdcall MonPreview::H3DlgBasePanel_Redraw(HiHook *hook, H3DlgBasePanel *panel, BOOL8 redraw,
                                               int firstId, int lastId)
{
    // Native Show() enables all controls immediately before this draw call.
    // Suppress replaced spell controls here so they cannot reappear over the
    // background now that native morale/luck widgets draw above it.
    if (auto *items = Get().FindPanelItems(panel))
        for (auto *item : items->nativeSpellItems)
            if (item)
                item->HideDeactivate();
    THISCALL_4(void, hook->GetDefaultFunc(), panel, redraw, firstId, lastId);
}

void MonPreview::UpdateExtendedPanel(H3CombatMonsterPanel *panel, H3CombatCreature *stack, H3Hero *hero)
{
    if (!panel || !stack)
        return;

    const auto *items = FindPanelItems(panel);
    if (!items)
        return;

    if (H3DlgPcx *background = items->background)
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

    SetNumber(items->health, GetCurrentUnitHealth(stack));
    SetNumber(items->losses, GetCreatureLosses(stack));
    SetActionStatus(stack, items->statusValues[static_cast<unsigned>(BattleStatusRow::Action)]);

    if (H3DlgText *retaliations = items->statusValues[static_cast<unsigned>(BattleStatusRow::Retaliations)])
    {
        if (stack->retaliations > RETALIATION_DISPLAY_LIMIT)
            retaliations->SetText("99+");
        else
            SetNumber(retaliations, stack->retaliations);
    }

    if (H3DlgText *shots = items->statusValues[static_cast<unsigned>(BattleStatusRow::Shots)])
    {
        if (stack->info.shooter)
            SetNumber(shots, stack->info.numberShots);
        else
            shots->SetText("-");
    }

    SetNumber(items->statusValues[static_cast<unsigned>(BattleStatusRow::Charges)], stack->info.spellCharges);
    UpdateSpellSlots(items->spellPictures, items->spellDurations, stack);
}

void MonPreview::UpdateSpellSlots(const std::array<H3DlgPcx16 *, SPELL_SLOT_COUNT> &pictures,
                                  const std::array<H3DlgText *, SPELL_SLOT_COUNT> &durations,
                                  H3CombatCreature *stack)
{
    std::array<int, SPELL_SLOT_COUNT> spells;
    spells.fill(-1);

    int count = 0;
    for (int spellId = COMBAT_SPELL_COUNT - 1; spellId >= 0 && count < SPELL_SLOT_COUNT; --spellId)
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

        const int frame = spellId + SPELL_DEF_FRAME_OFFSET;
        H3LoadedPcx16 *spellPicture =
            spellId >= 0 && frame < static_cast<int>(resizedSpellPictures.size())
                ? resizedSpellPictures[frame]
                : nullptr;
        const bool hasSpellPicture = spellPicture != nullptr;
        if (spellPicture)
        {
            SetCreatureSpellHints(picture, stack, spellId);
        }
        else
        {
            // Both empty slots and missing frames use the native no-spell backing.
            spellPicture = resizedSpellPictures.empty() ? nullptr : resizedSpellPictures[EMPTY_SPELL_FRAME];
            picture->SetHints(h3_NullString, h3_NullString, TRUE);
        }
        picture->SetPcx(spellPicture);
        if (spellPicture)
            picture->ShowActivate();
        else
            picture->HideDeactivate();

        if (!duration)
            continue;

        if (hasSpellPicture && HasVisibleSpellDuration(spellId))
        {
            char text[SPELL_DURATION_BUFFER_SIZE];
            std::snprintf(text, sizeof(text), "x%d", stack->activeSpellDuration[spellId]);
            duration->SetText(text);
            SetCreatureSpellHints(duration, stack, spellId);
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
