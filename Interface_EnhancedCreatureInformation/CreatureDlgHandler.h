#pragma once
#include "pch.h"

#include <string>
#include <vector>

class CreatureDlgHandler
{
    struct CreatureSkill
    {
        int frame = -1;
        std::string text;
        std::string hint;
        H3LoadedPcx16 *pcx16 = nullptr;
    };

    struct ScrolledItem
    {
        H3DlgItem *item = nullptr;
        int baseY = 0;
    };

    H3CreatureInfoDlg *dlg = nullptr;
    H3CombatCreature *stack = nullptr;
    H3Army *army = nullptr;
    int armySlotIndex = -1;
    bool wogStackExperience = false;
    const H3Hero *hero = nullptr;
    int playerColor = -1;

    int descriptionX = 24;
    int descriptionY = 189;
    int descriptionWidth = 250;
    int descriptionHeight = 55;
    int commanderPanelHeight = 0;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int maxScrollTick = 0;

    // Pixel offset for every logical row. A skill-icon row and a text row
    // both consume exactly one scrollbar tick.
    std::vector<int> scrollRowOffsets;
    std::vector<CreatureSkill> creatureSkills;
    std::vector<ScrolledItem> scrolledItems;

    void ApplyDialogAppearance();
    void HideDefaultBattleSpellItems();
    H3DlgText *FindOriginalDescription() const;
    std::string GetDescriptionText(H3DlgText *description) const;
    BOOL BuildDescriptionArea();
    BOOL BuildExperienceSkillsPanel(H3DlgText *description, const std::string &descriptionText);
    void ReleaseUnownedSkillImages();

  public:
    CreatureDlgHandler(H3CreatureInfoDlg *dlg, H3CombatCreature *stack = nullptr, H3Army *army = nullptr,
                       int armySlotIndex = -1, const H3Hero *hero = nullptr, int playerColor = -1);
    ~CreatureDlgHandler();

    void ScrollTo(int scrollTick);
    BOOL AlignItems();
    BOOL AddExperienceButton();
    BOOL AddSpellEfects();
    BOOL AddCommanderSkills();
    BOOL CreateCreatureSkillsList();
};
