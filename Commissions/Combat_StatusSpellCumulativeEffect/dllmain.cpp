// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

// void __thiscall BattleStack__ApplySpell(_BattleStack_ *this, int spell, int SpellPower, int school_level, H3Hero
// *hero)

struct StackSpellsApplied
{
    static constexpr int MAX_BONUS_APPLIES = 2;
    int spellsApplied[h3::limits::TOTAL_SPELLS]{};

} stackSpellsApplied[2][h3::limits::TOTAL_COMBAT_CREATURES];

// reset applied spells on battle start, otherwise they will be applied infinitely until the spell wears off
_ERH_(OnSetupBattleField)
{
    libc::memset(stackSpellsApplied, 0, sizeof(stackSpellsApplied));
}

void __stdcall BattleStack_ApplySpell(HiHook *h, H3CombatCreature *stack, const eSpell spellId, const int spellPower,
                                      const int schoolLevel, H3Hero *hero)
{

    int currentSpellDuration = 0;
    switch (spellId)
    {
    case eSpell::FORTUNE:
    case eSpell::MISFORTUNE:
    case eSpell::PRECISION:
    case eSpell::STONE_SKIN:
    case eSpell::SORROW:
    case eSpell::MIRTH:
    case eSpell::BLOODLUST:
    case eSpell::WEAKNESS:

        currentSpellDuration = stack->activeSpellDuration[spellId];

        //    case eSpell::SLAYER :
        break;
    default:
        break;
    }

    THISCALL_5(void, h->GetDefaultFunc(), stack, spellId, spellPower, schoolLevel, hero);

    if (currentSpellDuration && stackSpellsApplied[stack->side][stack->sideIndex].spellsApplied[spellId] <
                                    StackSpellsApplied::MAX_BONUS_APPLIES)
    {

        int *currentEffectPtr = nullptr;
        switch (spellId)
        {
        case eSpell::FORTUNE:
            currentEffectPtr = &stack->fortuneEffect;
            break;
        case eSpell::MISFORTUNE:
            currentEffectPtr = &stack->misfortuneEffect;
            break;
        case eSpell::PRECISION:
            currentEffectPtr = &stack->precisionEffect;
            break;
        case eSpell::STONE_SKIN:
            currentEffectPtr = &stack->stoneSkinEffect;
            break;
        case eSpell::SORROW:
            currentEffectPtr = &stack->sorrowEffect;
            break;
        case eSpell::MIRTH:
            currentEffectPtr = &stack->mirthEffect;
            break;
        case eSpell::BLOODLUST:
            currentEffectPtr = &stack->bloodlustEffect;
            break;
        case eSpell::WEAKNESS:
            currentEffectPtr = &stack->weaknessEffect;
            //    case eSpell::SLAYER :
            break;
        default:
            break;
        }
        if (currentEffectPtr)
        {
            const int spellBaseValue = P_Spell[spellId].baseValue[schoolLevel];
            const int heroSpecialityEffect =
                hero ? hero->GetSpellSpecialtyEffect(spellId, stack->info.level, spellBaseValue) : 0;
            const int newEffectValue = *currentEffectPtr + spellBaseValue + heroSpecialityEffect;
            *currentEffectPtr = newEffectValue;
            stackSpellsApplied[stack->side][stack->sideIndex].spellsApplied[spellId]++;
        }
    }
    if (stack->activeSpellDuration[spellId] == 0)
    {
        stackSpellsApplied[stack->side][stack->sideIndex].spellsApplied[spellId] = 0;
    }
}

_LHF_(HooksInit)
{
    _PI->WriteHiHook(0x0444610, THISCALL_, BattleStack_ApplySpell);
    _REH_(OnSetupBattleField);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            initialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::instanceName);
            Era::ConnectEra(hModule, dllText::instanceName);
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
