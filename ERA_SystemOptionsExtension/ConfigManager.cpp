#pragma comment(linker, "/EXPORT:GetOptionValue=_GetOptionValue@4")
#pragma comment(linker, "/EXPORT:SetOptionValue=_SetOptionValue@8")

#include "framework.h"

#include "CombatCreatureHealthBar.h"
#include "CombatSettings.h"
#include "MapScroller.h"
#include "SoundSettings.h"

std::unordered_map<std::string, AdditionalConfig::ConfigEntry *> AdditionalConfig::optionsMap;

namespace
{
using ConfigEntry = AdditionalConfig::ConfigEntry;
using EOptionChangeSource = AdditionalConfig::EOptionChangeSource;

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

    constexpr size_t length = sizeof(AdditionalConfig) / sizeof(ConfigEntry);
    auto array = data();
    for (size_t i = 0; i < length; i++)
    {
        array[i].Apply(EOptionChangeSource::InitialLoad);
    }
}

BOOL AdditionalConfig::Save()
{
    AdditionalConfig &instance = Get();
    constexpr size_t length = sizeof(AdditionalConfig) / sizeof(ConfigEntry);
    auto array = instance.data();
    for (size_t i = 0; i < length; i++)
    {
        auto &entry = array[i];
        libc::sprintf(h3_TextBuffer, "%d", entry.value);
        Era::WriteStrToIni(entry.keyName, h3_TextBuffer, sectionName, fileName);
    }
    return 1;
}
BOOL AdditionalConfig::Load()
{
    AdditionalConfig &instance = Get();
    constexpr size_t length = sizeof(AdditionalConfig) / sizeof(ConfigEntry);
    auto array = instance.data();
    for (size_t i = 0; i < length; i++)
    {
        auto &entry = array[i];
        optionsMap[entry.keyName] = &entry;
        if (Era::ReadStrFromIni(entry.keyName, sectionName, fileName, h3_TextBuffer))
        {
            entry.value = Clamp(0, atoi(h3_TextBuffer), entry.maxValue);
        }
    }
    instance.InitialApply();
    return 1;
}
