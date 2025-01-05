// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

Patcher* globalPatcher;
PatcherInstance* _PI;


int zoneCounters[100]{};
_LHF_(hota_CleanGroundToMonster)
{
    auto rmgMapItem = reinterpret_cast<H3RmgMapItem*>(c->eax);
    int a = 5;
    int zoneId = IntAt(c->ebp - 0x10);
    if (zoneCounters[zoneId]++ > 10)
    {
        c->return_address = 0x540FBE;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(hota_RMG_InitGenZones_One)
{
    c->edx = 0xD0; // Jebus Cross
    c->return_address = 0x53C257;
    return NO_EXEC_DEFAULT;
}

_LHF_(hota_RMG_0053BE60)
{
    auto v32 = c->edx;
    return EXEC_DEFAULT;
}

_LHF_(js_test123)
{
    //c->return_address = 0x53C01D;
    //return NO_EXEC_DEFAULT;
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
    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("js_RMG_MapGenerationFixes");
            
            // 1-tile path - change jump type
            _PI->WriteHexPatch(0x540F07, "E9C200");
            _PI->WriteHexPatch(0x540F0C, "90");

            // alt decision
            //_PI->WriteHexPatch(0x540FB7, "909090 90909090");
            //_PI->WriteHexPatch(0x541B6A, "9090");
            // alt decision 2
            //_PI->WriteLoHook(0x540FB1, hota_CleanGroundToMonster);

            // Используется для расчета размера пространства под генерацию? (edx)
            //_PI->WriteLoHook(0x53C1C7, hota_RMG_InitGenZones_One);

            // Это 99% используется для добавления в итерацию новых городов
            //_PI->WriteLoHook(0x532F15, hota_RMGGenZone_Ctor_One);

            // Сначала центр идет, потом зоны игроков
            //_PI->WriteLoHook(0x532F4B, hota_RMGGenZone_Ctor_Two);
            
            // Опять патч под новые города?
            //_PI->WriteLoHook(0x53C2C0, hota_RMG_InitGenZones_Two);

            //_PI->WriteLoHook(0x53BF76, hota_RMG_0053BE60);
            //_PI->WriteLoHook(0x53BF41, js_test123);
            
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

