#pragma once
#include "pch.h"
#include "PluginSettings.h"

namespace creatureInfo
{
constexpr int DLG_SPELLS_BTTN_ID = 4443;
constexpr int WOG_CREATURE_EXP_BUTTON_ID = 4444;
constexpr int CREATURE_EXP_SKILL_FIRST_ID = 4600;
constexpr int CREATURE_EXP_TEXT_FIRST_ID = 4700;
constexpr int CREATURE_EXP_SCROLLBAR_ID = 4800;

constexpr int DLG_WIDTH = 350;
constexpr int DLG_HEIGHT = 387;
// H3Dlg::DrawShadow draws an 8 px strip to the right and below the dialog.
constexpr int DLG_SHADOW_SIZE = 8;
constexpr int CONTENT_BOTTOM = 301;

constexpr int BUTTON_WIDTH = 46;
constexpr int BUTTON_HEIGHT = 32;
constexpr int BUTTON_Y = DLG_HEIGHT - BUTTON_HEIGHT - 48;
constexpr int OK_BUTTON_X = DLG_WIDTH - BUTTON_WIDTH - 18;

constexpr int EXP_SKILL_ICON_SIZE = 24;
constexpr int EXP_SKILL_GAP = 4;
constexpr int EXP_SCROLLBAR_WIDTH = 16;
constexpr int EXP_SCROLLBAR_GAP = 3;

constexpr const char *OK_BUTTON_DEF = "iOkay2.def";
constexpr const char *CAST_BUTTON_DEF = "iMagic.def";
constexpr const char *SPELL_LIST_BUTTON_DEF = "iBaff.def";
constexpr const char *DIALOG_BACKGROUND_PCX = "gemCrIBg.pcx";
constexpr const char *DIALOG_HINT_BAR_PCX = "gemCrVar.pcx";
constexpr const char *CREATURE_EXP_DEF = "DlgCrExp.def";

inline bool WogStackExperienceEnabled()
{
    return *reinterpret_cast<bool *>(0x02772730);
}

inline void ReadDescriptionRect(int &x, int &y, int &width, int &height)
{
    const auto &settings = GetPluginSettings();
    x = settings.descriptionX;
    y = settings.descriptionY;
    width = settings.descriptionWidth;
    height = settings.descriptionHeight;

    x = Clamp(8, x ? x : 24, 262);
    y = Clamp(185, y ? y : 189, 287);
    width = Clamp(16, width ? width : 250, 278 - x);
    height = Clamp(14, height ? height : 55, CONTENT_BOTTOM - y);
}

inline bool IsExperienceSkillItem(int itemId)
{
    return itemId >= CREATURE_EXP_SKILL_FIRST_ID &&
           itemId < CREATURE_EXP_SKILL_FIRST_ID + 32;
}

} // namespace creatureInfo
