// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"


Patcher* globalPatcher;
PatcherInstance* _PI;


void js_IncreaseAllElemsGrowth(H3Town* town, H3Dwelling* dwelling, int amount)
{
    if (dwelling->type == eObject::CREATURE_GENERATOR4 && dwelling->subtype == 0) // Conflux compound dwelling
    {
        for (size_t i = 0; i < 3; i++)
        {
            int monId = 113 + i;
            //void __thiscall Town::CorrectCreatureGrowth(_Town_ *town, eCreatures creatureId, int number)
            THISCALL_3(void, 0x5C0250, town, monId, amount);
        }
    }
}

_LHF_(js_DwellingInitBonus_BeforeCorrectCreatureGrowth_Subtract)
{
    H3Town* town = reinterpret_cast<H3Town*>(c->ecx);
    H3Dwelling* dwelling = reinterpret_cast<H3Dwelling*>(c->esi); // esi??? need to check
    js_IncreaseAllElemsGrowth(town, dwelling, -1);

    return EXEC_DEFAULT;
}

_LHF_(js_DwellingInitBonus_BeforeCorrectCreatureGrowth_Add)
{
    H3Town* town = reinterpret_cast<H3Town*>(c->ecx);
    H3Dwelling* dwelling = reinterpret_cast<H3Dwelling*>(c->esi); // esi??? need to check
    js_IncreaseAllElemsGrowth(town, dwelling, 1);
    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////

_LHF_(js_DwellingCapture_BeforeCorrectCreatureGrowth)
{
    H3Town* town = reinterpret_cast<H3Town*>(c->ecx);
    H3Dwelling* dwelling = reinterpret_cast<H3Dwelling*>(c->esi);
    js_IncreaseAllElemsGrowth(town, dwelling, -1);

    return EXEC_DEFAULT;
}

_LHF_(js_DwellingAddGrowthToTowns_BeforeCorrectCreatureGrowth)
{
    H3Town* town = reinterpret_cast<H3Town*>(c->ecx);
    H3Dwelling* dwelling = reinterpret_cast<H3Dwelling*>(c->ebx);
    js_IncreaseAllElemsGrowth(town, dwelling, 1);

    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////

_LHF_(js_CaptureTown_BeforeCorrectCreatureGrowth_Subtract)
{
    H3Town* town = reinterpret_cast<H3Town*>(c->ecx);
    H3Dwelling* dwelling = reinterpret_cast<H3Dwelling*>(c->esi);
    js_IncreaseAllElemsGrowth(town, dwelling, -1);

    return EXEC_DEFAULT;
}

_LHF_(js_CaptureTown_BeforeCorrectCreatureGrowth_Add)
{
    H3Town* town = reinterpret_cast<H3Town*>(c->eax);
    H3Dwelling* dwelling = reinterpret_cast<H3Dwelling*>(c->esi);
    js_IncreaseAllElemsGrowth(town, dwelling, 1);

    return EXEC_DEFAULT;
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
            _PI = globalPatcher->CreateInstance("js_ConfluxDwellingType20");

            _PI->WriteLoHook(0x4B868E, js_DwellingInitBonus_BeforeCorrectCreatureGrowth_Subtract);
            _PI->WriteLoHook(0x4B873A, js_DwellingInitBonus_BeforeCorrectCreatureGrowth_Add);

            _PI->WriteLoHook(0x4C655C, js_DwellingCapture_BeforeCorrectCreatureGrowth);
            _PI->WriteLoHook(0x4B853D, js_DwellingAddGrowthToTowns_BeforeCorrectCreatureGrowth);

            _PI->WriteLoHook(0x4C5FCF, js_CaptureTown_BeforeCorrectCreatureGrowth_Subtract);
            _PI->WriteLoHook(0x4C6319, js_CaptureTown_BeforeCorrectCreatureGrowth_Add);
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

