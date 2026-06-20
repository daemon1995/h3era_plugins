#include "framework.h"

#include "CombatHints.h"
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
PATCH_DECLATOR(cmbspd, CombatSpeed)
// PATCH_DECLATOR(sound, SoundSettings)

void AdditionalConfig::InitialApply()
{
    // if original combat speed patch is not present, apply it
    if (globalPatcher->GetInstance("BattleSpeed") == nullptr)
        cmbspd::CombatSpeed::Get();

    auto &mapScrollSettings = scroll::MapScroller::Get();
    mapScrollSettings.SetEnabled(smoothMapScroll.value);

    auto &soundSettings = sound::SoundSettings::Get();
    soundSettings.SetBackgroundSoundsState(backgroundSound.value);

    auto &combatHints = cmbhints::CombatHints::Get();
    combatHints.settings.isEnabled = showCreatureHealthBar.value;
}

BOOL AdditionalConfig::Save()
{
    AdditionalConfig &instance = Get();
    constexpr size_t length = sizeof(AdditionalConfig) / sizeof(SettingsEntry);
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
    constexpr size_t length = sizeof(AdditionalConfig) / sizeof(SettingsEntry);
    auto array = instance.data();
    for (size_t i = 0; i < length; i++)
    {
        auto &entry = array[i];
        if (Era::ReadStrFromIni(entry.keyName, sectionName, fileName, h3_TextBuffer))
        {
            entry.value = Clamp(0, atoi(h3_TextBuffer), entry.maxValue);
        }
    }
    instance.InitialApply();
    return 1;
}
