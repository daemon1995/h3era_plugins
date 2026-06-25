#pragma comment(linker, "/EXPORT:GetOptionValue=_GetOptionValue@4")
#pragma comment(linker, "/EXPORT:SetOptionValue=_SetOptionValue@8")

#include "framework.h"

#include "CombatCreatureHealthBar.h"
#include "SoundSettings.h"

#define PATCH_DECLATOR(nameSpaceName, className)                                                                       \
    namespace nameSpaceName                                                                                            \
    {                                                                                                                  \
    class className : public IGamePatch                                                                                \
    {                                                                                                                  \
      public:                                                                                                          \
        static className &className::Get();                                                                            \
    };                                                                                                                 \
    }
PATCH_DECLATOR(scroll, MapScroller)
// PATCH_DECLATOR(cmbhints, CombatHints)
PATCH_DECLATOR(cmbsttngs, CombatSettings)
// PATCH_DECLATOR(sound, SoundSettings)
std::unordered_map<std::string, AdditionalConfig::ConfigEntry *> AdditionalConfig::optionsMap;

DllExport INT __stdcall GetOptionValue(LPCSTR key)
{
    auto it = AdditionalConfig::optionsMap.find(key);
    if (it != AdditionalConfig::optionsMap.end())
        return it->second->value;
    return -1;
}
DllExport INT __stdcall SetOptionValue(LPCSTR key, INT value)
{
    auto it = AdditionalConfig::optionsMap.find(key);
    if (it != AdditionalConfig::optionsMap.end())
    {
        return it->second->value = Clamp(0, value, it->second->maxValue);
    }
    return -1;
}

void AdditionalConfig::InitialApply()
{
    // if original combat speed is present, undo it
    if (auto combatSpeedOri = globalPatcher->GetInstance("BattleSpeed"))
        combatSpeedOri->UndoAll();

    cmbsttngs::CombatSettings::Get();

    auto &mapScrollSettings = scroll::MapScroller::Get();
    mapScrollSettings.SetEnabled(smoothMapScroll.value);

    auto &soundSettings = sound::SoundSettings::Get();
    soundSettings.SetBackgroundSoundsState(backgroundSound.value);
    soundSettings.SetAlternativButtonClickState(alternativeButtonClick.value);

    auto &combatHints = cmbhints::CombatHints::Get();
    combatHints.settings.isEnabled = showCreatureHealthBar.value;
    auto queuePI = globalPatcher->GetInstance("H3.ERA_BattleQueue");
    if (queuePI)
    {
        const BOOL enabled = battleQueue.value;
        enabled ? queuePI->ApplyAll() : queuePI->UndoAll();
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
