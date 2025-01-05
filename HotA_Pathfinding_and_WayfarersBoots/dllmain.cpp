// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"


Patcher* globalPatcher;
PatcherInstance* _PI;


struct PathfindingSettings
{
    BOOL hotaPathfindingEnabled = false;
    eArtifact wayfarersBootsId = eArtifact::NONE;
};

static PathfindingSettings pathfindingSettings;


int js_HotaCalcStepCost(const int stepCost, const int pathfindSkill, char direction)
{
    int stepCostReduction = 25 * pathfindSkill;
    int minStepCost = 100 - 5 * pathfindSkill;

    if ((direction & 1) != 0)
    {
        stepCostReduction = static_cast <int>(static_cast <float>(stepCostReduction) * 1.4142135);
        minStepCost = static_cast <int>(static_cast <float>(minStepCost) * 1.4142135);
    }

    const int updStepCost = stepCost - stepCostReduction;

    if (updStepCost < minStepCost)
    {
        return minStepCost;
    }

    return updStepCost;
}


int __stdcall Get_Step_Cost(
    HiHook* h,
    H3MapItem* mapItem,
    char direction,
    signed int movementLeft,
    int pathfindSkill,
    int road,
    int flyPower,
    int waterWalkPower,
    int nativeLand,
    int nomadsCount
)
{
    bool wfBootsEquipped = false;
    INT8 targetLandType = mapItem->land;

    if (H3Hero* hero = P_ActivePlayer->GetActiveHero())
    {
        if (pathfindingSettings.wayfarersBootsId > 0)
        {
            if (hero->WearsArtifact(pathfindingSettings.wayfarersBootsId))
            {
                wfBootsEquipped = true;

                if (!pathfindingSettings.hotaPathfindingEnabled)
                {
                    pathfindSkill = eSecSkillLevel::EXPERT;
                    //return FASTCALL_9(int, h->GetDefaultFunc(), mapItem, direction, movementLeft, pathfindSkill, road, flyPower, waterWalkPower, nativeLand, nomadsCount);
                }
            }
        }
    }

    if (pathfindingSettings.hotaPathfindingEnabled)
    {
        //int terrainOverlayType = THISCALL_1(int, 0x4FD470, mapItem);

        //if (targetLandType != nativeLand || terrainOverlayType == eObject::CURSED_GROUND)
        //{
            const int skillLvl = wfBootsEquipped ? eSecSkillLevel::EXPERT : eSecSkillLevel::NONE;
            const int stepCost = FASTCALL_9(int, h->GetDefaultFunc(), mapItem, direction, movementLeft, skillLvl, road, flyPower, waterWalkPower, nativeLand, nomadsCount);
            const int updStepCost = pathfindSkill > 0 ? js_HotaCalcStepCost(stepCost, pathfindSkill, direction) : stepCost;

            return updStepCost;
        //}
    }

    return FASTCALL_9(int, h->GetDefaultFunc(), mapItem, direction, movementLeft, pathfindSkill, road, flyPower, waterWalkPower, nativeLand, nomadsCount);
}

_LHF_(LoadCrgen_1_4_TXT)
{
    pathfindingSettings.wayfarersBootsId = eArtifact(EraJS::readInt("jsEmerald.settings.wayfarersBootsId"));
    pathfindingSettings.hotaPathfindingEnabled = EraJS::readInt("jsMod.settings.hotaPathfinding.value");

    if (pathfindingSettings.wayfarersBootsId > 0 || pathfindingSettings.hotaPathfindingEnabled)
    {
        _PI->WriteHiHook(0x4B14A0, FASTCALL_, Get_Step_Cost);
    }

    return EXEC_DEFAULT;
}


BOOL APIENTRY DllMain(HMODULE hModule,
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
            _PI = globalPatcher->CreateInstance("js_HotA_Pathfinding_and_WayfarersBoots");

            _PI->WriteLoHook(0x4EDE4F, LoadCrgen_1_4_TXT);
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
