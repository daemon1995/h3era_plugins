#include "pch.h"
#include "PluginSettings.h"
#include "PluginSettingsDlg.h"

#include <cstdlib>

namespace creatureInfo
{
namespace
{
constexpr LPCSTR SETTINGS_FILE = "Runtime/PluginSettings/Interface_EnhancedCreatureInformation.ini";
constexpr LPCSTR SYSTEM_OPTIONS_DLL = "ERA_SystemOptionsExtension.era";
constexpr LPCSTR SYSTEM_OPTIONS_BUTTON_TAG = "Interface_EnhancedCreatureInformation.Settings";
constexpr LPCSTR SYSTEM_OPTIONS_BUTTON_NAME = "eci.settings.button_name";
constexpr LPCSTR SYSTEM_OPTIONS_BUTTON_HINT = "eci.settings.button_hint";

using RegisterPluginCallbackButton = BOOL(__stdcall *)(LPCSTR tag, LPCSTR name, LPCSTR description,
                                                       void (*callback)());

PluginSettings settings;

int ReadInt(LPCSTR section, LPCSTR key, int defaultValue) noexcept
{
    char text[64] = {};
    if (!Era::ReadStrFromIni(key, section, SETTINGS_FILE, text))
        return defaultValue;

    char *end = nullptr;
    const long value = std::strtol(text, &end, 10);
    return end == text || *end != '\0' ? defaultValue : static_cast<int>(value);
}

BOOL WriteInt(LPCSTR section, LPCSTR key, int value) noexcept
{
    char text[16];
    wsprintfA(text, "%d", value);
    return Era::WriteStrToIni(key, text, section, SETTINGS_FILE);
}
} // namespace

PluginSettings &GetPluginSettings() noexcept
{
    return settings;
}

void LoadPluginSettings() noexcept
{
    char existingValue[64] = {};
    const bool hasSettings = Era::ReadStrFromIni("BattleDialog", "Display", SETTINGS_FILE, existingValue) != FALSE;

    settings.showBattleDialog = ReadInt("Display", "BattleDialog", 1) != 0;
    settings.showArmyDialog = ReadInt("Display", "ArmyDialog", 1) != 0;
    settings.showRecruitmentDialog = ReadInt("Display", "RecruitmentDialog", 1) != 0;
    settings.showCreatureSkills = ReadInt("Display", "CreatureSkills", 1) != 0;
    settings.showCommanderSkills = ReadInt("Display", "CommanderSkills", 1) != 0;
    settings.showExpandedBattleMonsterPanel = ReadInt("Display", "ExpandedBattleMonsterPanel", 1) != 0;

    settings.descriptionX = ReadInt("Description", "X", settings.descriptionX);
    settings.descriptionY = ReadInt("Description", "Y", settings.descriptionY);
    settings.descriptionWidth = ReadInt("Description", "Width", settings.descriptionWidth);
    settings.descriptionHeight = ReadInt("Description", "Height", settings.descriptionHeight);
    settings.descriptionAlignment = ReadInt("Description", "Alignment", settings.descriptionAlignment);

    if (!hasSettings)
        SavePluginSettings(settings);
}

BOOL SavePluginSettings(const PluginSettings &value) noexcept
{
    BOOL success = TRUE;
    success = WriteInt("Display", "BattleDialog", value.showBattleDialog ? 1 : 0) && success;
    success = WriteInt("Display", "ArmyDialog", value.showArmyDialog ? 1 : 0) && success;
    success = WriteInt("Display", "RecruitmentDialog", value.showRecruitmentDialog ? 1 : 0) && success;
    success = WriteInt("Display", "CreatureSkills", value.showCreatureSkills ? 1 : 0) && success;
    success = WriteInt("Display", "CommanderSkills", value.showCommanderSkills ? 1 : 0) && success;
    success = WriteInt("Display", "ExpandedBattleMonsterPanel", value.showExpandedBattleMonsterPanel ? 1 : 0) && success;

    success = WriteInt("Description", "X", value.descriptionX) && success;
    success = WriteInt("Description", "Y", value.descriptionY) && success;
    success = WriteInt("Description", "Width", value.descriptionWidth) && success;
    success = WriteInt("Description", "Height", value.descriptionHeight) && success;
    success = WriteInt("Description", "Alignment", value.descriptionAlignment) && success;

    const BOOL saved = Era::SaveIni(SETTINGS_FILE);
    return saved && success;
}

void OpenPluginSettingsDialog()
{
    PluginSettingsDlg dialog;
    dialog.Start();
}

void __stdcall RegisterPluginSettingsButton(Era::TEvent *)
{
    LoadPluginSettings();

    HMODULE systemOptions = GetModuleHandleA(SYSTEM_OPTIONS_DLL);
    if (!systemOptions)
        return;

    auto registerButton = reinterpret_cast<RegisterPluginCallbackButton>(
        GetProcAddress(systemOptions, "RegisterPluginCallbackButton"));
    if (registerButton)
        registerButton(SYSTEM_OPTIONS_BUTTON_TAG, SYSTEM_OPTIONS_BUTTON_NAME, SYSTEM_OPTIONS_BUTTON_HINT,
                       OpenPluginSettingsDialog);
}
} // namespace creatureInfo
