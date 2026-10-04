#pragma once
#include "pch.h"

namespace preview
{
class MonPreview : public IGamePatch
{
    static MonPreview *instance;

    H3Vector<H3LoadedPcx16 *> resizedSpellPictures;
    H3LoadedPcx *extensionBackground = nullptr;

    virtual void CreatePatches() override;

    MonPreview();

    static H3CombatMonsterPanel *__stdcall H3CombatMonsterPanel_Ctor(HiHook *hook, H3CombatMonsterPanel *panel, int x,
                                                                     int y, int width, int height, H3BaseDlg *parent,
                                                                     DWORD type);
    static void __stdcall H3CombatMonsterPanel_Prepare(HiHook *hook, H3CombatMonsterPanel *panel,
                                                        H3CombatCreature *stack, H3Hero *hero);

    void CreateResizedSpellEffectPictures();
    void CreatePanelBackground();
    void BuildExtendedPanel(H3CombatMonsterPanel *panel);
    void UpdateExtendedPanel(H3CombatMonsterPanel *panel, H3CombatCreature *stack, H3Hero *hero);
    void UpdateSpellSlots(H3CombatMonsterPanel *panel, H3CombatCreature *stack);

  public:
    static MonPreview &Get();
};

} // namespace preview
