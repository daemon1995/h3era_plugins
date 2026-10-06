#pragma once

#include <windows.h>

namespace creatureInfo
{
struct PluginSettings
{
    bool showBattleDialog = true;
    bool showArmyDialog = true;
    bool showRecruitmentDialog = true;
    bool showCreatureSkills = true;
    bool showInactiveCreatureSkills = true;
    bool showCommanderSkills = true;
    bool showExpandedBattleMonsterPanel = true;

    int descriptionX = 24;
    int descriptionY = 185;
    int descriptionWidth = 250;
    int descriptionHeight = 110;
    int descriptionAlignment = 0;
};

PluginSettings &GetPluginSettings() noexcept;
void LoadPluginSettings() noexcept;
BOOL SavePluginSettings(const PluginSettings &settings) noexcept;
void OpenPluginSettingsDialog();
void __stdcall RegisterPluginSettingsButton();
} // namespace creatureInfo
