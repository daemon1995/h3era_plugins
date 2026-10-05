#pragma once
#include "pch.h"

#include <array>
#include <vector>

namespace preview
{
class MonPreview : public IGamePatch
{
    static MonPreview *instance;

    // Shared read-only cache for the current battle. H3DlgPcx16::SetPcx()
    // does not add a reference, therefore every panel slot must be detached
    // before a panel is destroyed or before this cache is released.
    std::vector<H3LoadedPcx16 *> resizedSpellPictures;
    std::vector<H3CombatMonsterPanel *> activePanels;
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

    static void __stdcall OnBeforeBattleUniversal(Era::TEvent *event);
    static void __stdcall OnAfterBattleUniversal(Era::TEvent *event);

    void EnsureSpellEffectResources();
    void CreateSpellEffectResources();
    void DestroySpellEffectResources();

    void RegisterPanel(H3CombatMonsterPanel *panel);
    void UnregisterPanel(H3CombatMonsterPanel *panel);
    void DetachPanelSpellImages(H3CombatMonsterPanel *panel);
    void DetachAllPanelSpellImages();

    void CreatePanelBackground();
    void BuildExtendedPanel(H3CombatMonsterPanel *panel);
    void UpdateExtendedPanel(H3CombatMonsterPanel *panel, H3CombatCreature *stack, H3Hero *hero);
    void UpdateSpellSlots(const std::array<H3DlgPcx16 *, 12> &pictures, const std::array<H3DlgText *, 12> &durations,
                          H3CombatCreature *stack);

  public:
    static MonPreview &Get();
};

} // namespace preview
