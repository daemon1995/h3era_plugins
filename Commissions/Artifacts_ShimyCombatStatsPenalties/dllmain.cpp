// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
constexpr LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}
struct PluginSettings
{
    INT singleArtStatsPenalty{20};
    INT comboArtsStatsPenalty{25};
    INT spellCostMultiplier{3};
} pluginSettings;
//- Cards of Prophecy: triple enemy spell cost

int __stdcall Hero__GetSpellCost(HiHook *h, H3Hero *heroCaster, const int spell, const H3Army *hostileArmy,
                                 const int ground)
{

    int spellCost = THISCALL_4(int, h->GetDefaultFunc(), heroCaster, spell, hostileArmy, ground);

    if (spellCost)
    {
        H3Hero *heroes[2] = {heroCaster, nullptr};

        if (hostileArmy)
        {
            for (size_t i = 0; i < 2; i++)
            {
                if (P_CombatManager->army[i] == hostileArmy)
                    heroes[1] = P_CombatManager->hero[i];
            }
            if (auto hostileHero = heroes[1])
            {
                if (hostileHero->WearsArtifact(eArtifact::CARDS_OF_PROPHECY))
                    spellCost *= pluginSettings.spellCostMultiplier;
            }
        }
    }

    return spellCost;
}

struct HeroStats
{
    BOOL isChanged{FALSE};
    unsigned char primarySkill[4]{};
    int spellPoints{0};

} heroStats[2];

_LHF_(BattleMgr_InitLogicLo)
{
    auto _this = c->Esi<H3CombatManager *>();
    if (_this && _this->hero[0] && _this->hero[1])
    {

        for (size_t i = 0; i < 2; i++)
        {
            heroStats[i] = {};
            int artsCounter = 0;
            int decreaseBy[4]{};
            auto wearingHero = _this->hero[i];
            auto hostileHero = _this->hero[1 - i];
            if (wearingHero->WearsArtifact(eArtifact::STATESMANS_MEDAL))
            {
                decreaseBy[ePrimary::ATTACK] = pluginSettings.singleArtStatsPenalty;
                artsCounter++;
            }
            if (wearingHero->WearsArtifact(eArtifact::AMBASSADORS_SASH))
            {
                decreaseBy[ePrimary::DEFENSE] = pluginSettings.singleArtStatsPenalty;
                artsCounter++;
            }
            if (wearingHero->WearsArtifact(eArtifact::DIPLOMATS_RING))
            {
                decreaseBy[ePrimary::SPELL_POWER] = pluginSettings.singleArtStatsPenalty;
                artsCounter++;
            }
            if (artsCounter)
            {
                heroStats[1 - i].isChanged = TRUE;
                if (artsCounter == 3 && pluginSettings.comboArtsStatsPenalty)
                {
                    for (size_t sk = 0; sk < 3; sk++)
                    {
                        heroStats[1 - i].primarySkill[sk] = hostileHero->primarySkill[sk];
                        hostileHero->primarySkill[sk] =
                            static_cast<UINT8>(static_cast<float>(hostileHero->primarySkill[sk]) * 0.75f);
                    }
                    heroStats[1 - i].spellPoints = hostileHero->spellPoints / 4;
                    hostileHero->spellPoints -= heroStats[1 - i].spellPoints;
                }
                else if (pluginSettings.singleArtStatsPenalty)
                {
                    for (size_t sk = 0; sk < 4; sk++)
                    {
                        if (int decrease = decreaseBy[sk])
                        {
                            heroStats[1 - i].primarySkill[sk] = hostileHero->primarySkill[sk];
                            hostileHero->primarySkill[sk] =
                                static_cast<UINT8>(static_cast<float>(hostileHero->primarySkill[sk]) * 0.8f);
                        }
                    }
                }
            }
        }
    }

    //   BattleMgr_InitLogic(h, P_CombatManager);
    return EXEC_DEFAULT;
}

void __stdcall BattleMgr_EndBattle(HiHook *h, H3CombatManager *_this, const int sideWinner)
{
    for (size_t i = 0; i < 2; i++)
    {
        if (heroStats[i].isChanged)
        {
            _this->hero[i]->spellPoints += heroStats[i].spellPoints;
            if (i)
            {
                heroStats[i] = {};
                continue;
            }
            for (size_t k = 0; k < 3; k++)
            {
                if (heroStats[i].primarySkill[k])
                {
                    _this->hero[i]->primarySkill[k] = heroStats[i].primarySkill[k];
                }
            }
            heroStats[i] = {};
        }
    }

    THISCALL_2(void, h->GetDefaultFunc(), _this, sideWinner);
}

_LHF_(HooksInit)
{
    //- Cards of Prophecy: triple enemy spell cost
    pluginSettings.spellCostMultiplier =
        EraJS::readInt("shimy.artifacts.cards_of_prophecy.hostile_spell_cost_multiplier");
    if (pluginSettings.spellCostMultiplier > 0)
    {
        _PI->WriteHiHook(0x4E54B0, THISCALL_, Hero__GetSpellCost);
    }

    pluginSettings.singleArtStatsPenalty = EraJS::readInt("shimy.artifacts.diplomacy.single_art_penalty");
    pluginSettings.comboArtsStatsPenalty = EraJS::readInt("shimy.artifacts.diplomacy.combo_art_penalty");
    if (pluginSettings.singleArtStatsPenalty || pluginSettings.comboArtsStatsPenalty)
    {
        _PI->WriteLoHook(0x463B09, BattleMgr_InitLogicLo);
        //  _PI->WriteHiHook(0x4627B5, THISCALL_, BattleMgr_InitLogic);
        _PI->WriteHiHook(0x475CFD, THISCALL_, BattleMgr_EndBattle);
    }

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
