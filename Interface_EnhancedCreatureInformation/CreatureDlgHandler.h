#pragma once
#include "pch.h"
#include "CreatureDlgLayout.h"

#include <string>
#include <vector>

class CreatureDlgHandler
{
    struct SkillIcon
    {
        const char *defName = nullptr;
        int frame = -1;
        const char *text = nullptr;
        const char *hint = nullptr;
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

    int descriptionX = creatureInfo::DESCRIPTION_DEFAULT_X;
    int descriptionY = creatureInfo::DESCRIPTION_DEFAULT_Y;
    int descriptionWidth = creatureInfo::DESCRIPTION_DEFAULT_WIDTH;
    int descriptionHeight = creatureInfo::DESCRIPTION_DEFAULT_HEIGHT;
    int commanderPanelHeight = 0;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int maxScrollTick = 0;
    int currentScrollTick = 0;

    // Pixel offset for every logical row. A skill-icon row and a text row
    // both consume exactly one scrollbar tick.
    std::vector<int> scrollRowOffsets;
    std::vector<SkillIcon> creatureSkills;
    std::vector<ScrolledItem> scrolledItems;
    std::vector<H3DlgPcx16 *> skillItems;
    std::vector<H3DlgText *> descriptionLineItems;
    H3DlgScrollbar *skillsScrollbar = nullptr;
    std::vector<char> skillPopupBuffer;

    void ApplyDialogAppearance();
    void HideDefaultBattleSpellItems();
    H3DlgText *FindOriginalDescription() const;
    std::string GetDescriptionText(H3DlgText *description) const;
    BOOL BuildDescriptionArea();
    BOOL BuildExperienceSkillsPanel(H3DlgText *description, const std::string &descriptionText);
    void ReleaseUnownedSkillImages();
    void ResetExperienceSkillsPanel();

  public:
    CreatureDlgHandler(H3CreatureInfoDlg *dlg, H3CombatCreature *stack = nullptr, H3Army *army = nullptr,
                       int armySlotIndex = -1, const H3Hero *hero = nullptr, int playerColor = -1);
    ~CreatureDlgHandler();

    void ScrollTo(int scrollTick);
    void RefreshCreatureArtifact();
    BOOL AlignItems();
    BOOL AddExperienceButton();
    BOOL AddSpellEffects();
    BOOL AddCommanderSkills();
    BOOL CreateCreatureSkillsList();
};
