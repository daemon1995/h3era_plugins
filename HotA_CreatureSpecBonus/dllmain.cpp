// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"


Patcher* globalPatcher;
PatcherInstance* _PI;

static constexpr LPCSTR HOTA_CREATURE_SPEC_OPT = "js_hota_specialties_creatures"; // global var in erm file
static constexpr LPCSTR HOTA_SPELL_SPEC_OPT = "js_hota_specialties_spells";
double standartCoeff = 0;

_LHF_(js_OnBeforeUseSpecMult)
{
    bool hotaCreatureSpecOpt = Era::GetAssocVarIntValue(HOTA_CREATURE_SPEC_OPT);

    if (hotaCreatureSpecOpt)
    {
        H3CreatureInformation* crInfo = reinterpret_cast<H3CreatureInformation*>(c->esi);
        int monId = c->ebx;
        int crLvl = crInfo->level;

        standartCoeff = DoubleAt(0x63AC58); // store original value

        double newCoeff = 0;

        if (crLvl == 0)
        {
            newCoeff = 0.1;
        }
        else if (monId == eCreature::BALLISTA)
        {
            newCoeff = 0.3;
        }
        else
        {
            newCoeff = 0.2;
        }

        DoubleAt(0x63AC58) = newCoeff;
    }


    return EXEC_DEFAULT;
}

_LHF_(js_OnAfterUseSpecMult)
{
    bool hotaCreatureSpecOpt = Era::GetAssocVarIntValue(HOTA_CREATURE_SPEC_OPT);

    if (hotaCreatureSpecOpt)
    {
        DoubleAt(0x63AC58) = standartCoeff; // restore original value
    }
    
    return EXEC_DEFAULT;
}

int __stdcall HeroSpellSpecialityEffect(
    HiHook* h,
    H3Hero* hero,
    eSpell spell,
    int monLvl,
    signed int effect
)
{
    bool hotaSpellSpecOpt = Era::GetAssocVarIntValue(HOTA_SPELL_SPEC_OPT);

    if (!hotaSpellSpecOpt)
    {
        return THISCALL_4(int, h->GetDefaultFunc(), hero, spell, monLvl, effect);
    }
    else
    {
        int bonus = 0;

        if (P_HeroSpecialty->Get()[hero->id].GetSpell() == spell)
        {
            switch (spell)
            {
                // nerf Luna
                case eSpell::FIRE_WALL:
                    bonus = effect / 4;
					break;

                // +5% for Meteor, Chain Lightning, Resurrection and Animate Undead
                case eSpell::METEOR_SHOWER:
                case eSpell::CHAIN_LIGHTNING:
                case eSpell::RESURRECTION:
                case eSpell::ANIMATE_DEAD:
                    bonus = (__int64)ceil((double)(effect * (hero->level / (monLvl + 1))) * 0.05);
                    break;

                // forgetfullness: no hero in game for this
                // +10% for n hero lvls, where n — target lvl
                
                // adds +10% to effect for (8 – n) hero lvls, where n — target lvl
                case eSpell::CURE:
                    bonus = (__int64)ceil((double)(effect * (hero->level / (7 - monLvl))) * 0.1);
                    break;

                // adds +10/8/6/4 to effect for creatures 1-2/3-4/5-6/7 lvl
                case eSpell::BLOODLUST:
                case eSpell::STONE_SKIN:
                case eSpell::PRECISION:
                    if (monLvl == 0 || monLvl == 1)
                    {
                        bonus = 10;
                        break;
                    }
                    else if (monLvl == 2 || monLvl == 3)
                    {
                        bonus = 8;
                        break;
                    }
                    else if (monLvl == 4 || monLvl == 5)
                    {
                        bonus = 6;
                        break;
                    }
                    else
                    {
                        bonus = 4;
                        break;
                    }

                // adds +4/6/8/10 to effect for creatures 1-2/3-4/5-6/7 lvl
                case eSpell::WEAKNESS:
                    if (monLvl == 0 || monLvl == 1)
                    {
                        bonus = 4;
                        break;
                    }
                    else if (monLvl == 2 || monLvl == 3)
                    {
                        bonus = 6;
                        break;
                    }
                    else if (monLvl == 4 || monLvl == 5)
                    {
                        bonus = 8;
                        break;
                    }
                    else
                    {
                        bonus = 10;
                        break;
                    }

                // adds +3/2/1 to effect for creatures 1-4/5-6/7 lvl
                case eSpell::HASTE:
                case eSpell::PRAYER:
                    if (monLvl >=0 && monLvl <= 3)
                    {
                        bonus = 3;
                        break;
                    }
                    else if (monLvl == 4 || monLvl == 5)
                    {
                        bonus = 2;
                        break;
                    }
                    else
                    {
                        bonus = 1;
                        break;
                    }

                default:
                    // +10% per hero lvl for all other spells
                    bonus = (__int64)ceil((double)(effect * (hero->level / (monLvl + 1))) * 0.1);
                    break;
            }
        }

        return bonus;
    }
}

void __stdcall OnAfterErmInstructions(Era::TEvent* event)
{
    bool hotaSpellSpecOpt = Era::GetAssocVarIntValue(HOTA_SPELL_SPEC_OPT);

    if (hotaSpellSpecOpt)
    {
        //slayer
        IntAt(0x63EAC4) = 0x14;
        IntAt(0x63EAC8) = 0x14;
        IntAt(0x63EACC) = 0x10;
        IntAt(0x63EAD0) = 0x10;
        IntAt(0x63EAD4) = 0xC;
        IntAt(0x63EAD8) = 0xC;
        IntAt(0x63EADC) = 0x8;

        //disrupting ray
        _PI->WriteHexPatch(0x4E62E6, "0A");
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static BOOL plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        //!< Attach process, initialize the plugin if it hasn't been initialized before

        //! Check if the plugin is already on
        if (!plugin_On)
        {
            plugin_On = 1;

            //! Get the global patcher
            globalPatcher = GetPatcher();

            //! Create an instance of the plugin
            _PI = globalPatcher->CreateInstance("js_HotA_CreatureSpecBonus");

            _PI->WriteLoHook(0x4E6515, js_OnBeforeUseSpecMult);
            _PI->WriteLoHook(0x4E659C, js_OnAfterUseSpecMult);

            _PI->WriteHiHook(0x4E6260, THISCALL_, HeroSpellSpecialityEffect);

            Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
