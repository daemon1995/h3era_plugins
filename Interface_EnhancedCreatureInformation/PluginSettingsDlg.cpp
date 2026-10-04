#include "pch.h"
#include "PluginSettings.h"
#include "PluginSettingsDlg.h"

using namespace h3;

namespace
{
constexpr int DIALOG_WIDTH = 500;
constexpr int DIALOG_HEIGHT = 318;
constexpr int FIRST_CHECKBOX_ID = 100;
constexpr int CHECKBOX_X = 34;
constexpr int LABEL_X = 70;
constexpr int LABEL_WIDTH = 390;
constexpr int CHECKBOX_ROW_HEIGHT = 30;
constexpr int CHECKBOX_START_Y = 55;

} // namespace

PluginSettingsDlg::PluginSettingsDlg()
    : H3Dlg(DIALOG_WIDTH, DIALOG_HEIGHT, -1, -1, false, true,
            P_Game ? P_Game->GetPlayerID() : IntAt(0x69CCF4)),
      draft(creatureInfo::GetPluginSettings())
{
    auto title = H3DlgText::Create(24, 18, DIALOG_WIDTH - 48, 24, Era::tr("eci.settings.title"),
                                   NH3Dlg::Text::BIG, eTextColor::WHITE, -1,
                                   eTextAlignment::MIDDLE_CENTER);
    if (title)
        AddItem(title);

    AddCheckbox(FIRST_CHECKBOX_ID + 0, CHECKBOX_START_Y, "eci.settings.battle_dialog",
                "eci.settings.battle_dialog_hint", draft.showBattleDialog);
    AddCheckbox(FIRST_CHECKBOX_ID + 1, CHECKBOX_START_Y + CHECKBOX_ROW_HEIGHT, "eci.settings.army_dialog",
                "eci.settings.army_dialog_hint", draft.showArmyDialog);
    AddCheckbox(FIRST_CHECKBOX_ID + 2, CHECKBOX_START_Y + CHECKBOX_ROW_HEIGHT * 2,
                "eci.settings.recruitment_dialog", "eci.settings.recruitment_dialog_hint",
                draft.showRecruitmentDialog);
    AddCheckbox(FIRST_CHECKBOX_ID + 3, CHECKBOX_START_Y + CHECKBOX_ROW_HEIGHT * 3,
                "eci.settings.creature_skills", "eci.settings.creature_skills_hint", draft.showCreatureSkills);
    AddCheckbox(FIRST_CHECKBOX_ID + 4, CHECKBOX_START_Y + CHECKBOX_ROW_HEIGHT * 4,
                "eci.settings.commander_skills", "eci.settings.commander_skills_hint", draft.showCommanderSkills);
    AddCheckbox(FIRST_CHECKBOX_ID + 5, CHECKBOX_START_Y + CHECKBOX_ROW_HEIGHT * 5,
                "eci.settings.battle_panel", "eci.settings.battle_panel_hint",
                draft.showExpandedBattleMonsterPanel);

    CreateOKButton();//(DIALOG_WIDTH - 74, DIALOG_HEIGHT - 47);
    CreateCancelButton();//(24, DIALOG_HEIGHT - 47);
}

void PluginSettingsDlg::AddCheckbox(int id, int y, LPCSTR labelKey, LPCSTR hintKey, bool checked)
{
    auto checkbox = H3DlgDefButton::Create(CHECKBOX_X, y, id, NH3Dlg::Assets::ON_OFF_CHECKBOX,
                                           checked ? 1 : 0, checked ? 1 : 0, false, -1);
    if (checkbox)
    {
        checkbox->SetHint(Era::tr(hintKey));
        AddItem(checkbox);
    }

    auto label = H3DlgText::Create(LABEL_X, y, LABEL_WIDTH, 24, Era::tr(labelKey),
                                   NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, id + 100,
                                   eTextAlignment::MIDDLE_LEFT);
    if (label)
        AddItem(label);
}

BOOL PluginSettingsDlg::OnLeftClick(INT itemId, H3Msg &msg)
{
    if (itemId >= FIRST_CHECKBOX_ID && itemId < FIRST_CHECKBOX_ID + 6)
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

VOID PluginSettingsDlg::OnOK()
{
    auto &settings = creatureInfo::GetPluginSettings();
    settings = draft;
    const auto isChecked = [this](int id, bool fallback) {
        const auto checkbox = GetDefButton(id);
        return checkbox ? checkbox->GetFrame() != 0 : fallback;
    };
    settings.showBattleDialog = isChecked(FIRST_CHECKBOX_ID + 0, draft.showBattleDialog);
    settings.showArmyDialog = isChecked(FIRST_CHECKBOX_ID + 1, draft.showArmyDialog);
    settings.showRecruitmentDialog = isChecked(FIRST_CHECKBOX_ID + 2, draft.showRecruitmentDialog);
    settings.showCreatureSkills = isChecked(FIRST_CHECKBOX_ID + 3, draft.showCreatureSkills);
    settings.showCommanderSkills = isChecked(FIRST_CHECKBOX_ID + 4, draft.showCommanderSkills);
    settings.showExpandedBattleMonsterPanel = isChecked(FIRST_CHECKBOX_ID + 5, draft.showExpandedBattleMonsterPanel);

    if (!creatureInfo::SavePluginSettings(settings))
        H3Messagebox::Show(Era::tr("eci.settings.save_failed"));
}
