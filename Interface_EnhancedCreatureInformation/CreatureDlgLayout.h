#pragma once
#include "pch.h"
#include "PluginSettings.h"

namespace creatureInfo
{
enum CreatureDialogItemId : int
{
    DLG_DESCRIPTION_ID = -1,
    DLG_DESCRIPTION_FALLBACK_ID = 1,
    DLG_BACKGROUND_ID = 200,
    DLG_NAME_ID = 203,
    DLG_MORALE_ID = 219,
    DLG_LUCK_ID = 220,
    DLG_NATIVE_SPELL_FIRST_ID = 221,
    DLG_NATIVE_SPELL_LAST_ID = 223,
    DLG_HINT_ID = 224,
    DLG_OK_FRAME_ID = 225,
    DLG_UPGRADE_ID = 300,
    DLG_CAST_ID = 301,
    DLG_SPELL_FIRST_ID = 1000,
    DLG_SPELL_DURATION_FIRST_ID = 1100,
    DLG_NATIVE_SPELL_TEXT_FIRST_ID = 3000,
    DLG_NATIVE_SPELL_TEXT_LAST_ID = 3002,
    DLG_MORALE_TEXT_ID = 3006,
    DLG_LUCK_TEXT_ID = 3007,
    DLG_SPELLS_BTTN_ID = 4443,
    WOG_CREATURE_EXP_BUTTON_ID = 4444,
    CREATURE_EXP_SKILL_FIRST_ID = 4600,
    CREATURE_EXP_TEXT_FIRST_ID = 4700,
    CREATURE_EXP_SCROLLBAR_ID = 4800,
    DLG_OK_ID = 30722,
    DLG_DISMISS_ID = 30723
};

constexpr int NATIVE_DLG_WIDTH = 298;
constexpr int NATIVE_DLG_HEIGHT = 311;

constexpr int DLG_WIDTH = 350;
constexpr int DLG_HEIGHT = 387;
// H3Dlg::DrawShadow draws an 8 px strip to the right and below the dialog.
constexpr int DLG_SHADOW_SIZE = 8;
constexpr int CONTENT_BOTTOM = 301;

constexpr int BUTTON_WIDTH = 46;
constexpr int BUTTON_HEIGHT = 32;
constexpr int BUTTON_Y = DLG_HEIGHT - BUTTON_HEIGHT - 48;
constexpr int OK_BUTTON_X = DLG_WIDTH - BUTTON_WIDTH - 18;
constexpr int BUTTON_FRAME_BORDER = 1;
constexpr int BUTTON_FRAME_WIDTH = BUTTON_WIDTH + BUTTON_FRAME_BORDER * 2;
constexpr int BUTTON_FRAME_HEIGHT = BUTTON_HEIGHT + BUTTON_FRAME_BORDER * 2;
constexpr int DISMISS_BUTTON_X = 127;
constexpr int UPGRADE_BUTTON_X = 233;
constexpr int CAST_BUTTON_X = 126;
constexpr int EXPERIENCE_BUTTON_X = 180;
constexpr int HINT_MARGIN = 7;
constexpr int MORALE_X = 24;
constexpr int LUCK_X = 78;
constexpr int MORALE_LUCK_BOTTOM_MARGIN = 46;
constexpr int BATTLE_DIALOG_POSITION_OFFSET = 30;
constexpr int ADVENTURE_DIALOG_BOTTOM_MARGIN = 145;
constexpr int SPELL_COLUMN_X = 283;
constexpr int SPELL_COLUMN_Y = 47;
constexpr int SPELL_ROW_HEIGHT = 42;
constexpr int SPELL_VISIBLE_ROWS = 6;
constexpr int SPELL_ROWS_WITH_BUTTON = SPELL_VISIBLE_ROWS - 1;
constexpr int SPELL_DURATION_WIDTH = 24;
constexpr int SPELL_DURATION_HEIGHT = 12;
constexpr int SPELL_POPUP_MARGIN = 20;
constexpr int SPELL_POPUP_GAP = 5;
constexpr int SPELL_POPUP_SIZE_PADDING = 35;
constexpr int SPELL_POPUP_DURATION_Y = 25;
constexpr int SPELL_POPUP_DURATION_HEIGHT = 14;
constexpr int SPELL_POPUP_RIGHT_MARGIN = 200;
constexpr int SPELL_POPUP_BOTTOM_MARGIN = 48;

constexpr int DESCRIPTION_DEFAULT_X = 24;
constexpr int DESCRIPTION_DEFAULT_Y = 189;
constexpr int DESCRIPTION_DEFAULT_WIDTH = 250;
constexpr int DESCRIPTION_DEFAULT_HEIGHT = 55;
constexpr int DESCRIPTION_MIN_X = 8;
constexpr int DESCRIPTION_MAX_X = 262;
constexpr int DESCRIPTION_MIN_Y = 185;
constexpr int DESCRIPTION_MAX_Y = 287;
constexpr int DESCRIPTION_MIN_WIDTH = 16;
constexpr int DESCRIPTION_MIN_HEIGHT = 14;
constexpr int DESCRIPTION_RIGHT = 278;
constexpr int MAX_SKILL_WIDGETS = 32;
constexpr DWORD STACK_EXPERIENCE_ENABLED_ADDRESS = 0x02772730;

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
constexpr const char *BUTTON_FRAME_PCX = "box46x32.pcx";
constexpr const char *EXPERIENCE_BUTTON_DEF = "CrExpBut.def";

inline bool WogStackExperienceEnabled()
{
    return *reinterpret_cast<bool *>(STACK_EXPERIENCE_ENABLED_ADDRESS);
}

inline void ReadDescriptionRect(int &x, int &y, int &width, int &height)
{
    const auto &settings = GetPluginSettings();
    x = settings.descriptionX;
    y = settings.descriptionY;
    width = settings.descriptionWidth;
    height = settings.descriptionHeight;

    x = Clamp(DESCRIPTION_MIN_X, x ? x : DESCRIPTION_DEFAULT_X, DESCRIPTION_MAX_X);
    y = Clamp(DESCRIPTION_MIN_Y, y ? y : DESCRIPTION_DEFAULT_Y, DESCRIPTION_MAX_Y);
    width = Clamp(DESCRIPTION_MIN_WIDTH, width ? width : DESCRIPTION_DEFAULT_WIDTH, DESCRIPTION_RIGHT - x);
    height = Clamp(DESCRIPTION_MIN_HEIGHT, height ? height : DESCRIPTION_DEFAULT_HEIGHT, CONTENT_BOTTOM - y);
}

inline bool IsExperienceSkillItem(int itemId)
{
    return itemId >= CREATURE_EXP_SKILL_FIRST_ID &&
           itemId < CREATURE_EXP_SKILL_FIRST_ID + MAX_SKILL_WIDGETS;
}

inline bool GetDialogButtonX(int itemId, int &x)
{
    switch (itemId)
    {
    case DLG_OK_ID: x = OK_BUTTON_X; return true;
    case DLG_DISMISS_ID: x = DISMISS_BUTTON_X; return true;
    case DLG_UPGRADE_ID: x = UPGRADE_BUTTON_X; return true;
    case DLG_CAST_ID: x = CAST_BUTTON_X; return true;
    default: return false;
    }
}

inline void SetDialogButtonBounds(H3DlgItem *item, int x)
{
    if (!item)
        return;
    item->SetX(x);
    item->SetY(BUTTON_Y);
    item->SetWidth(BUTTON_WIDTH);
    item->SetHeight(BUTTON_HEIGHT);
}

} // namespace creatureInfo
