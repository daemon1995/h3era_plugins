#define _H3API_PLUGINS_
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

int GetHeroShootingDistanceBonus(H3Hero *hero)
{

    int funcId = 11111111;
    Era::AllocErmFunc("acm_GetHeroShootingDistanceBonus", funcId);

    auto args = *Era::GetArgXVars();
    args[0] = hero->id;
    Era::FireErmEvent(funcId);

    return (*Era::GetRetXVars())[1];
}
void SetNoPenaltyLimit(const char distance)
{
    ByteAt(0x046732E + 2) = distance;
    ByteAt(0x04673E2 + 2) = distance;
    ByteAt(0x0467401 + 2) = distance;
}

BOOL8 __stdcall BattleMgr_ShooterHasDistancePenalty(HiHook *h, H3CombatManager *mgr, H3CombatCreature *stack,
                                                    H3CombatCreature *target)

{
    int distanceBonus = 0, distanceBackup;
    if (auto hero = stack->GetOwner())
    {
        distanceBackup = ByteAt(0x046732E + 2);

        distanceBonus = GetHeroShootingDistanceBonus(hero) + distanceBackup;
    }

    if (distanceBonus)
    {
        SetNoPenaltyLimit(distanceBonus);
    }
    BOOL8 result = THISCALL_3(BOOL8, h->GetDefaultFunc(), mgr, stack, target);

    if (distanceBonus)
    {
        SetNoPenaltyLimit(distanceBackup);
    }
    return result;
}

_LHF_(HooksInit)
{

    _PI->WriteHiHook(0x04671E0, THISCALL_, BattleMgr_ShooterHasDistancePenalty);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(dllText::instanceName);
        Era::ConnectEra(hModule, dllText::instanceName);
        _PI->WriteLoHook(0x4EEAF2, HooksInit); // minimal init hook (no-op)

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
