#define _H3API_PLUGINS_
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}
int moraleBonus = 0;

int GetHeroArmyAlignmentsBonus(H3Hero *hero)
{
    int funcId = 11111111;
    Era::AllocErmFunc("acm_GetHeroArmyAlignmentsBonus", funcId);

    auto args = *Era::GetArgXVars();
    args[0] = hero->id;
    Era::FireErmEvent(funcId);

    return (*Era::GetRetXVars())[1];
}
_LHF_(Army_GetMorale)
{
    auto hero = ValueAt<H3Hero *>(c->ebp + 0x8);
    if (hero)
    {
        moraleBonus = GetHeroArmyAlignmentsBonus(hero);
    }

    return EXEC_DEFAULT;
}
_LHF_(Army_GetMoraleList)
{
    auto hero = ValueAt<H3Hero *>(c->ebp + 0x14);
    if (hero)
    {
        moraleBonus = GetHeroArmyAlignmentsBonus(hero);
    }

    return EXEC_DEFAULT;
}

int __stdcall Army_GetFactionsNum(HiHook *h, H3Army *army, char *factions)
{

    int result = THISCALL_2(int, h->GetDefaultFunc(), army, factions);

    if (!moraleBonus)
    {
        return result;
    }

    result = Clamp(1, result - moraleBonus, result);
    moraleBonus = 0;
    return result;
}

_LHF_(HooksInit)
{

    // No hooks yet; placeholder for future implementation
    _PI->WriteHiHook(0x044B852, THISCALL_, Army_GetFactionsNum);
    _PI->WriteLoHook(0x044B852, Army_GetMoraleList);

    _PI->WriteHiHook(0x044AB70, THISCALL_, Army_GetFactionsNum);
    _PI->WriteLoHook(0x044AB70, Army_GetMorale);

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
