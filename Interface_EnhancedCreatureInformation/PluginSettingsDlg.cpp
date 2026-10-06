#include "pch.h"

#include "PluginSettingsDlg.h"
#include "PluginSettings.h"

using namespace h3;

namespace
{
constexpr int DIALOG_WIDTH = 500;
constexpr int DIALOG_HEIGHT = 348;
constexpr int FIRST_CHECKBOX_ID = 100;
enum class SettingRow : int
{
    Battle, Army, Recruitment, CreatureSkills, InactiveSkills, CommanderSkills, BattlePanel, Count
};
constexpr int CHECKBOX_COUNT = static_cast<int>(SettingRow::Count);
constexpr int LABEL_ID_OFFSET = 100;
constexpr int LABEL_HEIGHT = 24;
constexpr int TITLE_X = 24;
constexpr int TITLE_Y = 18;
constexpr int TITLE_HEIGHT = 24;
constexpr DWORD CURRENT_PLAYER_COLOR_ADDRESS = 0x69CCF4;
struct CheckboxSpec
{
    bool creatureInfo::PluginSettings::*field;
    LPCSTR label;
    LPCSTR hint;
};
constexpr CheckboxSpec CHECKBOXES[] = {
    {&creatureInfo::PluginSettings::showBattleDialog, "eci.settings.battle_dialog", "eci.settings.battle_dialog_hint"},
    {&creatureInfo::PluginSettings::showArmyDialog, "eci.settings.army_dialog", "eci.settings.army_dialog_hint"},
    {&creatureInfo::PluginSettings::showRecruitmentDialog, "eci.settings.recruitment_dialog", "eci.settings.recruitment_dialog_hint"},
    {&creatureInfo::PluginSettings::showCreatureSkills, "eci.settings.creature_skills", "eci.settings.creature_skills_hint"},
    {&creatureInfo::PluginSettings::showInactiveCreatureSkills, "eci.settings.inactive_creature_skills", "eci.settings.inactive_creature_skills_hint"},
    {&creatureInfo::PluginSettings::showCommanderSkills, "eci.settings.commander_skills", "eci.settings.commander_skills_hint"},
    {&creatureInfo::PluginSettings::showExpandedBattleMonsterPanel, "eci.settings.battle_panel", "eci.settings.battle_panel_hint"}
};
static_assert(sizeof(CHECKBOXES) / sizeof(CHECKBOXES[0]) == CHECKBOX_COUNT, "Missing checkbox description");
constexpr int CHECKBOX_X = 34;
constexpr int LABEL_X = 70;
constexpr int LABEL_WIDTH = 390;
constexpr int CHECKBOX_ROW_HEIGHT = 30;
constexpr int CHECKBOX_START_Y = 55;

} // namespace

PluginSettingsDlg::PluginSettingsDlg()
    : H3Dlg(DIALOG_WIDTH, DIALOG_HEIGHT, -1, -1, false, true, P_Game ? P_Game->GetPlayerID() : IntAt(CURRENT_PLAYER_COLOR_ADDRESS)),
      draft(creatureInfo::GetPluginSettings())
{
    auto title = H3DlgText::Create(TITLE_X, TITLE_Y, DIALOG_WIDTH - TITLE_X * 2, TITLE_HEIGHT, Era::tr("eci.settings.title"), NH3Dlg::Text::BIG,
                                   eTextColor::WHITE, -1, eTextAlignment::MIDDLE_CENTER);
    if (title)
        AddItem(title);

    for (int row = 0; row < CHECKBOX_COUNT; ++row)
    {
        const auto &spec = CHECKBOXES[row];
        AddCheckbox(FIRST_CHECKBOX_ID + row, CHECKBOX_START_Y + row * CHECKBOX_ROW_HEIGHT,
                    spec.label, spec.hint, draft.*spec.field);
    }
    CreateOKButton();
    CreateCancelButton();
}

void PluginSettingsDlg::AddCheckbox(int id, int y, LPCSTR labelKey, LPCSTR hintKey, bool checked)
{
    auto checkbox = H3DlgDefButton::Create(CHECKBOX_X, y, id, NH3Dlg::Assets::ON_OFF_CHECKBOX, checked ? 1 : 0,
                                           checked ? 1 : 0, false, -1);
    if (checkbox)
    {
        checkbox->SetHint(Era::tr(hintKey));
        AddItem(checkbox);
    }

    auto label = H3DlgText::Create(LABEL_X, y, LABEL_WIDTH, LABEL_HEIGHT, Era::tr(labelKey), NH3Dlg::Text::MEDIUM,
                                   eTextColor::REGULAR, id + LABEL_ID_OFFSET, eTextAlignment::MIDDLE_LEFT);
    if (label)
        AddItem(label);
}

BOOL PluginSettingsDlg::OnLeftClick(INT itemId, H3Msg &msg)
{
    if (itemId >= FIRST_CHECKBOX_ID && itemId < FIRST_CHECKBOX_ID + CHECKBOX_COUNT)
    {
        if (auto checkbox = GetDefButton(itemId))
        {
            const int state = checkbox->GetFrame() == 0 ? 1 : 0;
            checkbox->SetFrame(state);
            checkbox->SetClickFrame(state);
            Redraw();
            return TRUE;
        }
    }
    return H3Dlg::OnLeftClick(itemId, msg);
}

bool PluginSettingsDlg::IsCheckboxChecked(int id, bool fallback)
{
    const auto checkbox = GetDefButton(id);
    return checkbox ? checkbox->GetFrame() != 0 : fallback;
}

VOID PluginSettingsDlg::OnOK()
{
    auto &settings = creatureInfo::GetPluginSettings();
    settings = draft;
    for (int row = 0; row < CHECKBOX_COUNT; ++row)
    {
        const auto field = CHECKBOXES[row].field;
        settings.*field = IsCheckboxChecked(FIRST_CHECKBOX_ID + row, draft.*field);
    }
    if (!creatureInfo::SavePluginSettings(settings))
        H3Messagebox::Show(Era::tr("eci.settings.save_failed"));
}
