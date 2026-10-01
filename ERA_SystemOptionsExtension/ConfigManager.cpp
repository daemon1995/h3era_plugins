#pragma comment(linker, "/EXPORT:GetOptionValue=_GetOptionValue@4")
#pragma comment(linker, "/EXPORT:SetOptionValue=_SetOptionValue@8")

#include "framework.h"

#include "CombatCreatureHealthBar.h"
#include "CombatSettings.h"
#include "MapScroller.h"
#include "SoundSettings.h"
#include <cerrno>
#include <cctype>
#include <cstdlib>

std::unordered_map<std::string, AdditionalConfig::ConfigEntry *> AdditionalConfig::optionsMap;

namespace
{
using ConfigEntry = AdditionalConfig::ConfigEntry;
using EOptionChangeSource = AdditionalConfig::EOptionChangeSource;

BOOL ParseInteger(const char *text, int &value) noexcept
{
    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || errno == ERANGE)
        return FALSE;
    while (std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    if (*end)
        return FALSE;
    value = static_cast<int>(parsed);
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
    {
        entry.value ? queuePI->ApplyAll() : queuePI->UndoAll();
    }
}

void ApplyHealthBar(const ConfigEntry &entry, const EOptionChangeSource)
{
    cmbhints::CombatHints::SetHealthBarEnabled(entry.value);
}

void ApplySmoothMapScroll(const ConfigEntry &entry, const EOptionChangeSource)
{
    scroll::MapScroller::ApplySmoothScrollState(entry.value);
}
} // namespace

DllExport INT __stdcall GetOptionValue(LPCSTR key)
{
    if (!key)
        return -1;

    auto it = AdditionalConfig::optionsMap.find(key);
    if (it != AdditionalConfig::optionsMap.end())
        return it->second->value;
    return -1;
}
DllExport INT __stdcall SetOptionValue(LPCSTR key, INT value)
{
    if (!key)
        return -1;

    auto it = AdditionalConfig::optionsMap.find(key);
    if (it != AdditionalConfig::optionsMap.end())
    {
        return it->second->SetValue(value, AdditionalConfig::EOptionChangeSource::ExternalApi);
    }
    return -1;
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

    // if original combat speed is present, undo it
    if (auto combatSpeedOri = globalPatcher->GetInstance("BattleSpeed"))
        combatSpeedOri->UndoAll();

    cmbsttngs::CombatSettings::Get();

    for (auto *entry : Entries())
        entry->Apply(EOptionChangeSource::InitialLoad);
}

BOOL AdditionalConfig::Save()
{
    AdditionalConfig &instance = Get();
    BOOL success = TRUE;
    for (auto *entry : instance.Entries())
    {
        libc::sprintf(h3_TextBuffer, "%d", entry->value);
        if (!Era::WriteStrToIni(entry->keyName, h3_TextBuffer, sectionName, fileName))
            success = FALSE;
    }
    // WriteStrToIni updates ERA's cache; saving it is a separate operation.
    return Era::SaveIni(fileName) && success;
}
BOOL AdditionalConfig::Load()
{
    AdditionalConfig &instance = Get();
    for (auto *entry : instance.Entries())
    {
        optionsMap[entry->keyName] = entry;
        entry->value = entry->defaultValue;
        int parsed = 0;
        if (Era::ReadStrFromIni(entry->keyName, sectionName, fileName, h3_TextBuffer) && ParseInteger(h3_TextBuffer, parsed))
            entry->value = Clamp(0, parsed, entry->maxValue);
    }
    instance.InitialApply();
    return 1;
}
