// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
using namespace h3;
namespace dllText
{
constexpr const char *PLUGIN_AUTHOR = "daemon_n";
constexpr const char *PLUGIN_VERSION = "1.0";
constexpr const char *PLUGIN_DATA = __DATE__;
constexpr const char *INSTANCE_NAME = "EraPlugin.Gameplay_SlayerSpellRework.daemon_n";
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

float slayerSpecialtyPower = 0.05f;

int __stdcall BattleStack_CalculateDamageBonus(HiHook *h, H3CombatCreature *_this, signed int damage, char shoot,
                                               H3CombatCreature *enemy, int IsRealDamage, signed int StepsTaken)
{
    int result = THISCALL_6(int, h->GetDefaultFunc(), _this, damage, shoot, enemy, IsRealDamage, StepsTaken);
    if (_this->activeSpellDuration[eSpell::SLAYER])
    {
        if (float slayerPower = P_Spell[eSpell::SLAYER].baseValue[_this->slayerType])
        {
            if (auto hero = _this->GetOwner())
            {
                const auto &speciality = P_HeroSpecialty[hero->id];
                if (speciality.type == eHeroSpecialty::SPELL && speciality.GetSpell() == eSpell::SLAYER)
                {
                    slayerPower += ceil(slayerPower * static_cast<float>(hero->level / (_this->info.level + 1)) *
                                        slayerSpecialtyPower);
                }
            }
            // apply slayer bonus
            result += static_cast<int>(static_cast<float>(damage * (slayerPower / 100)));
        }
    }
    return result;
}

void __stdcall OnAfterWog(Era::TEvent *event)
{
    _PI->WriteDword(0x0442178, 0xA4E9);
    _PI->WriteByte(0x0442178 + 5, 0x90);

    _PI->WriteHiHook(0x0443040, THISCALL_, BattleStack_CalculateDamageBonus);
    //_PI->WriteHiHook(0x0443040, THISCALL_, BattleStack_GetSpeed);

    slayerSpecialtyPower = Clamp(1, EraJS::readInt("shimmy.spells.55.speciality"), 100) / static_cast<float>(100);

	_PI->WriteByte(0x059F9BC + 54, 5);

}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginOn)
        {
            pluginOn = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            Era::RegisterHandler(OnAfterWog, "OnAfterWog");
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
