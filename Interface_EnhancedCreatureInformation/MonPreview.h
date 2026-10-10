#pragma once
#include "pch.h"

#include <array>
#include <vector>

namespace preview
{
enum BattlePreviewItemId : int
{
    NATIVE_MORALE_LABEL_ID = 2210,
    NATIVE_MORALE_ICON_ID = 2211,
    NATIVE_LUCK_LABEL_ID = 2212,
    NATIVE_LUCK_ICON_ID = 2213,
    NATIVE_SPELL_FIRST_ID = 2215,
    NATIVE_SPELL_LAST_ID = 2218,
    HEALTH_LABEL_ID = 4000,
    HEALTH_VALUE_ID = 4001,
    LOSSES_LABEL_ID = 4002,
    LOSSES_VALUE_ID = 4003,
    STATUS_FIRST_LABEL_ID = 4004,
    STATUS_FIRST_VALUE_ID = 4005,
    EXTENSION_BG_ID = 4090,
    SPELL_FIRST_ITEM_ID = 4100
};

enum class BattleStatusRow : unsigned
{
    Action,
    Retaliations,
    Shots,
    Charges,
    Count
};

constexpr int BATTLE_PREVIEW_SPELL_SLOTS = 12;
constexpr unsigned BATTLE_PREVIEW_STATUS_ROWS = static_cast<unsigned>(BattleStatusRow::Count);
constexpr int NATIVE_PREVIEW_SPELL_ITEMS = NATIVE_SPELL_LAST_ID - NATIVE_SPELL_FIRST_ID + 1;

class MonPreview : public IGamePatch
{
    static MonPreview *instance;

    struct PanelItems
    {
        H3CombatMonsterPanel *panel = nullptr;
        H3DlgPcx *background = nullptr;
        H3DlgText *health = nullptr;
        H3DlgText *losses = nullptr;
        std::array<H3DlgText *, BATTLE_PREVIEW_STATUS_ROWS> statusValues = {};
        std::array<H3DlgPcx16 *, BATTLE_PREVIEW_SPELL_SLOTS> spellPictures = {};
        std::array<H3DlgText *, BATTLE_PREVIEW_SPELL_SLOTS> spellDurations = {};
        std::array<H3DlgItem *, NATIVE_PREVIEW_SPELL_ITEMS> nativeSpellItems = {};
    };

    // One cache reference per frame, plus one reference per attached widget.
    // Recreated panels (including battle replay) reuse the same frames.
    std::vector<H3LoadedPcx16 *> resizedSpellPictures;
    std::vector<PanelItems> activePanels;
    int spellIconWidth = 0;
    int spellIconHeight = 0;

    H3LoadedPcx *extensionBackground = nullptr;

    virtual void CreatePatches() override;

    MonPreview();

    static H3CombatMonsterPanel *__stdcall H3CombatMonsterPanel_Ctor(HiHook *hook, H3CombatMonsterPanel *panel, int x,
                                                                     int y, int width, int height, H3BaseDlg *parent,
                                                                     DWORD type);
    static void __stdcall H3CombatMonsterPanel_Prepare(HiHook *hook, H3CombatMonsterPanel *panel,
                                                       H3CombatCreature *stack, H3Hero *hero);
    static void __stdcall H3CombatMonsterPanel_Dtor(HiHook *hook, H3CombatMonsterPanel *panel);
    static void __stdcall H3DlgBasePanel_Redraw(HiHook *hook, H3DlgBasePanel *panel, BOOL8 redraw,
                                             int firstId, int lastId);

    static void __stdcall OnBattleFinishedOrGameLeave(Era::TEvent *event);

    void EnsureSpellEffectResources();
    void CreateSpellEffectResources();
    void DestroySpellEffectResources();
    void ClearBattleResources();

    void RegisterPanel(H3CombatMonsterPanel *panel);
    void UnregisterPanel(H3CombatMonsterPanel *panel);
    static PanelItems CollectPanelItems(H3CombatMonsterPanel *panel);
    PanelItems *FindPanelItems(H3DlgBasePanel *panel);

    void CreatePanelBackground();
    void BuildExtendedPanel(H3CombatMonsterPanel *panel);
    void UpdateExtendedPanel(H3CombatMonsterPanel *panel, H3CombatCreature *stack, H3Hero *hero);
    void UpdateSpellSlots(const std::array<H3DlgPcx16 *, BATTLE_PREVIEW_SPELL_SLOTS> &pictures,
                          const std::array<H3DlgText *, BATTLE_PREVIEW_SPELL_SLOTS> &durations,
                          H3CombatCreature *stack);

  public:
    static MonPreview &Get();
};

} // namespace preview
