#pragma comment(linker, "/EXPORT:GetOptionValue=_GetOptionValue@4")
#pragma comment(linker, "/EXPORT:SetOptionValue=_SetOptionValue@8")
#pragma comment(linker, "/EXPORT:SaveOptions=_SaveOptions@0")
#include "CombatCreatureHealthBar.h"
#include "CombatSettings.h"
#include "MapScroller.h"
#include "SoundSettings.h"
#include "framework.h"
#include <cstddef>
#include <cstring>

std::unordered_map<std::string, AdditionalConfig::ConfigEntry *> AdditionalConfig::optionsMap;
namespace
{
using ConfigEntry = AdditionalConfig::ConfigEntry;
using EOptionChangeSource = AdditionalConfig::EOptionChangeSource;
sysopts::SaveState saveState;
constexpr LPCSTR HEALTH_SECTION = "Settings.Extra.CombatStackHealthBar";

struct NativeEntry
{
    LPCSTR key;
    size_t offset;
    int minValue;
    int maxValue;
    int &Value() const noexcept
    {
        return *reinterpret_cast<int *>(reinterpret_cast<char *>(&OriginalConfig::Get()) + offset);
    }
    int ConfiguredValue() const noexcept
    {
        return offset == offsetof(OriginalConfig, autoSpells) ? cmbsttngs::CombatSettings::PersistentAutoSpells()
                                                              : Value();
    }
};
#define NATIVE_OPTION(key, field, min, max) {key, offsetof(OriginalConfig, field), min, max}
const NativeEntry nativeEntries[] = {
    NATIVE_OPTION("AdvMap.EnemySpeed", enemySpeed, 2, 5),
    NATIVE_OPTION("AdvMap.PlayerSpeed", playerSpeed, 1, 4),
    NATIVE_OPTION("Sound.MusicVolume", musicVolume, 0, 9),
    NATIVE_OPTION("Sound.EffectsVolume", effectsVolume, 0, 9),
    NATIVE_OPTION("Sound.LastMusicVolume", lastMusicVolume, 0, 9),
    NATIVE_OPTION("Sound.LastEffectsVolume", lastEffectsVolume, 0, 9),
    NATIVE_OPTION("AdvMap.AutoSave", autoSave, 0, 1),
    NATIVE_OPTION("AdvMap.ShowRoute", showRoute, 0, 1),
    NATIVE_OPTION("AdvMap.MoveReminder", moveReminder, 0, 1),
    NATIVE_OPTION("System.VideoSubtitles", videoSubtitles, 0, 1),
    NATIVE_OPTION("System.BuildingOutlines", buildingOutlines, 0, 1),
    NATIVE_OPTION("System.SpellBookAnimation", spellBookAnimation, 0, 1),
    NATIVE_OPTION("AdvMap.ScrollSpeed", mapScrollSpeed, 0, 2),
    NATIVE_OPTION("Combat.AutoCreatures", autoCreatures, 0, 1),
    NATIVE_OPTION("Combat.AutoSpells", autoSpells, 0, 1),
    NATIVE_OPTION("Combat.AutoCatapult", autoCatapult, 0, 1),
    NATIVE_OPTION("Combat.AutoBallista", autoBallista, 0, 1),
    NATIVE_OPTION("Combat.AutoFirstAidTent", autoFirstAidTent, 0, 1),
    NATIVE_OPTION("System.VideoQuality", videoQuality, 0, 1),
    NATIVE_OPTION("Combat.HexGrid", showHexGrid, 0, 1),
    NATIVE_OPTION("Combat.CursorShadow", cursorShadow, 0, 1),
    NATIVE_OPTION("Combat.MovementShadow", movementShadow, 0, 1),
    NATIVE_OPTION("Combat.CreatureInfo", combatViewArmy, 0, 2),
    NATIVE_OPTION("Combat.AnimationSpeed", animationSpeed, 0, 9),
};
#undef NATIVE_OPTION
const NativeEntry *FindNative(LPCSTR key) noexcept
{
    for (const auto &entry : nativeEntries)
        if (!std::strcmp(key, entry.key))
            return &entry;
    return nullptr;
}
BOOL ApplyNative(const NativeEntry &entry, const int requested, const BOOL reportError = FALSE) noexcept
{
    const int value = Clamp(entry.minValue, requested, entry.maxValue);
    if (entry.offset == offsetof(OriginalConfig, musicVolume))
        return sound::SoundSettings::TrySetMusicVolume(value, reportError, TRUE);
    if (entry.offset == offsetof(OriginalConfig, effectsVolume))
        return sound::SoundSettings::TrySetEffectsVolume(value, reportError, TRUE);
    if (entry.offset == offsetof(OriginalConfig, autoSpells))
    {
        cmbsttngs::CombatSettings::SetPersistentAutoSpells(value);
        return TRUE;
    }
    entry.Value() = value;
    OriginalConfig::Get().blackoutComputer = OriginalConfig::Get().enemySpeed == 5;
    return TRUE;
}
void ApplyAlternativeButtonClick(const ConfigEntry &entry, const EOptionChangeSource)
{
    sound::SoundSettings::SetAlternativButtonClickState(entry.value);
}
void ApplyBackgroundSound(const ConfigEntry &entry, const EOptionChangeSource source)
{
    if (source == EOptionChangeSource::InitialLoad)
        sound::SoundSettings::SetBackgroundSoundsState(entry.value);
    else
        sound::SoundSettings::ApplyBackgroundSoundsState(entry.value);
}
void ApplyBattleQueue(const ConfigEntry &entry, const EOptionChangeSource)
{
    if (auto queuePI = globalPatcher->GetInstance("H3.ERA_BattleQueue"))
        entry.value ? queuePI->ApplyAll() : queuePI->UndoAll();
}
void ApplyHealthBar(const ConfigEntry &entry, const EOptionChangeSource)
{
    cmbhints::CombatHints::SetHealthBarEnabled(entry.value);
}
void ApplySmoothMapScroll(const ConfigEntry &entry, const EOptionChangeSource)
{
    scroll::MapScroller::ApplySmoothScrollState(entry.value);
}
BOOL ReadHealth(LPCSTR key, char *buffer)
{
    return Era::ReadStrFromIni(key, HEALTH_SECTION, AdditionalConfig::fileName, buffer);
}
void LoadHealth()
{
    auto &settings = cmbhints::CombatHints::Get().settings;
    char buffer[4096];
    int integer = 0;
    if (ReadHealth("held", buffer) && sysopts::ParseInteger(buffer, integer))
        settings.isHeld = integer != 0;
    if (ReadHealth("keyCode", buffer) && sysopts::ParseInteger(buffer, integer))
    {
        const int scanCode = MapVirtualKeyA(integer, MAPVK_VK_TO_VSC);
        if (settings.validateScanCode(static_cast<eVKey>(scanCode)))
        {
            settings.vKey = integer;
            settings.scanCode = scanCode;
        }
    }
    const char *keys[] = {"fillColor", "lossColor", "colorSaturation", "yLabelShift"};
    for (int i = 0; i < 4; ++i)
        if (ReadHealth(keys[i], buffer))
            sysopts::ParseFloat(buffer, settings.values[i], i == 3 ? -18.0f : 0.0f, i == 3 ? 18.0f : 1.0f);
}
BOOL WriteInteger(LPCSTR key, const int value, LPCSTR section)
{
    return Era::WriteStrToIni(key, std::to_string(value).c_str(), section, AdditionalConfig::fileName);
}
} // namespace

DllExport INT __stdcall GetOptionValue(LPCSTR key)
{
    if (!key)
        return -1;
    const auto it = AdditionalConfig::optionsMap.find(key);
    if (it != AdditionalConfig::optionsMap.end())
        return it->second->value;
    if (const auto native = FindNative(key))
        return native->ConfiguredValue();
    return -1;
}
DllExport INT __stdcall SetOptionValue(LPCSTR key, INT value)
{
    if (!key)
        return -1;
    BOOL changed = FALSE;
    const auto it = AdditionalConfig::optionsMap.find(key);
    if (it != AdditionalConfig::optionsMap.end())
    {
        if (!AdditionalConfig::CanChangeOption(*it->second, EOptionChangeSource::ExternalApi))
            return -2;
        changed = it->second->SetValue(value, EOptionChangeSource::ExternalApi);
    }
    else if (const auto native = FindNative(key))
    {
        const int previous = native->ConfiguredValue();
        if (!AdditionalConfig::SetNativeValue(&native->Value(), value))
            return -2;
        changed = previous != native->ConfiguredValue();
    }
    else
        return -1;
    return AdditionalConfig::SaveIfDirty() ? changed : -3;
}
DllExport BOOL __stdcall SaveOptions()
{
    return AdditionalConfig::Save();
}
BOOL AdditionalConfig::IsInCombat() noexcept
{
    const auto combat = P_CombatManager->Get();
    return combat && combat->dlg;
}
BOOL AdditionalConfig::CanChangeOption(const ConfigEntry &entry, const EOptionChangeSource source) noexcept
{
    return source == EOptionChangeSource::InitialLoad ||
           sysopts::CanChangeCombatOption(IsInCombat(),
                                          &entry == &Get().battleQueue || &entry == &Get().quickCombatType);
}
void AdditionalConfig::MarkDirty() noexcept
{
    saveState.MarkDirty();
}
UINT AdditionalConfig::Revision() noexcept
{
    return saveState.Revision();
}
int AdditionalConfig::ReadNativeValue(const int *valuePtr) noexcept
{
    if (valuePtr == &OriginalConfig::Get().autoSpells)
        return cmbsttngs::CombatSettings::PersistentAutoSpells();
    return valuePtr ? *valuePtr : 0;
}
BOOL AdditionalConfig::SetNativeValue(int *valuePtr, const int value, const BOOL reportError) noexcept
{
    for (const auto &entry : nativeEntries)
        if (&entry.Value() == valuePtr)
        {
            return ApplyNative(entry, value, reportError);
        }
    return FALSE;
}
void AdditionalConfig::BindCallbacks() noexcept
{
    alternativeButtonClick.applyCallback = ApplyAlternativeButtonClick;
    backgroundSound.applyCallback = ApplyBackgroundSound;
    battleQueue.applyCallback = ApplyBattleQueue;
    quickCombatType.applyCallback = cmbsttngs::CombatSettings::ApplyQuickCombatType;
    showCreatureHealthBar.applyCallback = ApplyHealthBar;
    smoothMapScroll.applyCallback = ApplySmoothMapScroll;
}
void AdditionalConfig::InitialApply()
{
    BindCallbacks();
    if (auto combatSpeedOri = globalPatcher->GetInstance("BattleSpeed"))
        combatSpeedOri->UndoAll();
    cmbsttngs::CombatSettings::Get();
    for (auto *entry : Entries())
        entry->Apply(EOptionChangeSource::InitialLoad);
}
BOOL AdditionalConfig::Save()
{
    BOOL success = TRUE;
    for (auto *entry : Get().Entries())
        success = WriteInteger(entry->keyName, entry->value, sectionName) && success;
    const auto &health = cmbhints::CombatHints::Get().settings;
    success = WriteInteger("held", !!health.isHeld, HEALTH_SECTION) && success;
    success = WriteInteger("keyCode", health.vKey, HEALTH_SECTION) && success;
    const char *keys[] = {"fillColor", "lossColor", "colorSaturation", "yLabelShift"};
    for (int i = 0; i < 4; ++i)
        success =
            Era::WriteStrToIni(keys[i], std::to_string(health.values[i]).c_str(), HEALTH_SECTION, fileName) && success;
    success = Era::SaveIni(fileName) && success;
    saveState.Saved(success != FALSE);
    return success;
}
BOOL AdditionalConfig::SaveIfDirty(const BOOL reportError)
{
    if (!saveState.IsDirty() || Save())
        return TRUE;
    if (reportError)
    {
        bool translated = false;
        LPCSTR format = EraJS::read("era.opt.system.saveError", translated);
        if (!translated)
            format = "Unable to save settings to %s. Changes remain active; saving will be retried.";
        H3Messagebox::Show(H3String::Format(format, fileName));
    }
    return FALSE;
}
BOOL AdditionalConfig::Load()
{
    auto &instance = Get();
    char buffer[4096];
    int parsed = 0;
    // The game loads and saves native options; this plugin reads only its own settings.
    for (auto *entry : instance.Entries())
    {
        optionsMap[entry->keyName] = entry;
        entry->value = entry->defaultValue;
        if (Era::ReadStrFromIni(entry->keyName, sectionName, fileName, buffer) && sysopts::ParseInteger(buffer, parsed))
            entry->value = Clamp(0, parsed, entry->maxValue);
    }
    LoadHealth();
    instance.InitialApply();
    saveState.Saved(true);
    return TRUE;
}
