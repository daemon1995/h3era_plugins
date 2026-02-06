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
    float wallHpMultiplier = 3.0f;           // 3 times wall HP
    float arrowTowerDamageMultiplier = 3.0f; // 3 times arrow tower damage

} pluginSettings;
static void __stdcall CombatManager_InitTownWalls(HiHook *h, H3CombatManager *cmbMgr)
{
    THISCALL_1(void, h->GetDefaultFunc(), cmbMgr);
    if (!cmbMgr->creatureBank && cmbMgr->siegeKind2 > 0)
    {
        for (auto &i : cmbMgr->fortWallsHp)
        {
            i *= pluginSettings.wallHpMultiplier; // Triple wall HP
        }
    }
}

static long double __stdcall BattleStack_DoDamageByArrowTower(HiHook *h, const H3CombatCreature *stack,
                                                              const BOOL checkAirShield)
{
    return THISCALL_2(long double, h->GetDefaultFunc(), stack, checkAirShield) *
           pluginSettings.arrowTowerDamageMultiplier;
}

_LHF_(HooksInit)
{

    bool readSuccess = false;

    float temp = EraJS::readFloat("suft.combat.siege.walls.health_points_multiplier", readSuccess);
    if (readSuccess && temp > 0)
    {
        pluginSettings.wallHpMultiplier = temp;
    }
    if (pluginSettings.wallHpMultiplier > 0.f && pluginSettings.wallHpMultiplier != 1.f)
    {
        _PI->WriteHiHook(0x465E70, THISCALL_, CombatManager_InitTownWalls);
    }
    temp = EraJS::readFloat("suft.combat.siege.arrow_tower.damage", readSuccess);
    if (readSuccess && temp > 0)
    {
        pluginSettings.arrowTowerDamageMultiplier = temp;
    }
    if (pluginSettings.arrowTowerDamageMultiplier > 0.f && pluginSettings.arrowTowerDamageMultiplier != 1.f)
    {
        _PI->WriteHiHook(0x0443AB0, THISCALL_, BattleStack_DoDamageByArrowTower);
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
