#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;
namespace dllText
{
    LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

const double maxAttackDamageBonus = 1.0; // +100%
const double attackDiffBonus = 0.025;    // 2.5% per point

const double maxDefenceDamageBonus = 0.65;   // (1.0 - 0.65)
const double defenceDiffBonus = 0.0125;      // 2.5% per point

double __stdcall Hero_GetOffenceArmorerPower(HiHook* hook, H3Hero* hero)
{
    const int heroLevel = hero->level;
    hero->level = 15; // set level to 15 to limit level effect on offence or armorer power
    const double armorerPower = THISCALL_1(double, hook->GetDefaultFunc(), hero);
    hero->level = heroLevel; // restore level as it may be changed in the original function

    return armorerPower;
}
void ChangeAttackFormula()
{
    // set attack difference bonus to 2.5% per point

    constexpr DWORD attackAddressesToPatch[] = {
        0x044286F + 2,
        0x0438AB5 + 2,
        0x0442904 + 2,
        0x04430DF + 2,
        // 0x04E56CB + 2,
        // 0x04E56E0 + 2
    };

    for (size_t i = 0; i < std::size(attackAddressesToPatch); i++)
    {
        _PI->WriteDword(attackAddressesToPatch[i], (DWORD)&attackDiffBonus);
    }

    // set max attack limit to 100%
    _PI->WriteDword(0x04430EB + 2, (DWORD)&maxAttackDamageBonus);
    _PI->WriteHexPatch(0x04430FF + 3, "0000F03F");

    // limit offence power calculation to hero level 15
    _PI->WriteHiHook(0x04E4520, THISCALL_, Hero_GetOffenceArmorerPower);

}
void ChangeDefenceFormula()
{
    // set defence difference bonus to 1.25% per point
    _PI->WriteDword(0x04438F4 + 2, (DWORD)&defenceDiffBonus);
    // set max defence limit to 35% (1.0 - 0.65)
    _PI->WriteDword(0x0443906 + 2, (DWORD)&maxDefenceDamageBonus);
    _PI->WriteHexPatch(0x0443913, "B9 CDCCCCCC B8 CCCCE43F");

    _PI->WriteHiHook(0x04E4580, THISCALL_, Hero_GetOffenceArmorerPower);
}


_LHF_(HooksInit)
{
    ChangeAttackFormula();
    ChangeDefenceFormula();
    return EXEC_DEFAULT;

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginInitialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginInitialized)
        {
            pluginInitialized = true;
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
