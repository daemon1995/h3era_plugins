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

int Hero_GetSkillsNumberToReachComboSkill(const H3Hero *hero, const eSecondary skillId)
{
    switch (skillId)
    {
    case eSecondary::AIR_MAGIC:
    case eSecondary::FIRE_MAGIC:
    case eSecondary::WATER_MAGIC:
    case eSecondary::EARTH_MAGIC:
        return 4 - (hero->secSkill[eSecondary::AIR_MAGIC]) + (hero->secSkill[eSecondary::FIRE_MAGIC]) +
               (hero->secSkill[eSecondary::WATER_MAGIC]) + (hero->secSkill[eSecondary::EARTH_MAGIC]);

    case eSecondary::ARCHERY:
    case eSecondary::OFFENSE:
    case eSecondary::ARMORER:
    case eSecondary::TACTICS:
        return 4 - (hero->secSkill[eSecondary::ARCHERY]) + (hero->secSkill[eSecondary::OFFENSE]) +
               (hero->secSkill[eSecondary::ARMORER]) + (hero->secSkill[eSecondary::TACTICS]);
    case eSecondary::LOGISTICS:
    case eSecondary::PATHFINDING:
    case eSecondary::SCOUTING:
    case eSecondary::NAVIGATION:
        return 4 - (hero->secSkill[eSecondary::LOGISTICS]) + (hero->secSkill[eSecondary::PATHFINDING]) +
               (hero->secSkill[eSecondary::SCOUTING]) + (hero->secSkill[eSecondary::NAVIGATION]);

    case eSecondary::WISDOM:
    case eSecondary::SORCERY:
    case eSecondary::MYSTICISM:
    case eSecondary::INTELLIGENCE:
        return 4 - (hero->secSkill[eSecondary::WISDOM]) + (hero->secSkill[eSecondary::SORCERY]) +
               (hero->secSkill[eSecondary::MYSTICISM]) + (hero->secSkill[eSecondary::INTELLIGENCE]);
    case eSecondary::ARTILLERY:
    case eSecondary::BALLISTICS:
    case eSecondary::FIRST_AID:
        return 3 - (hero->secSkill[eSecondary::ARTILLERY]) + (hero->secSkill[eSecondary::BALLISTICS]) +
               (hero->secSkill[eSecondary::FIRST_AID]);
    case eSecondary::DIPLOMACY:
    case eSecondary::LEARNING:
    case eSecondary::ESTATES:
    case eSecondary::SCHOLAR:
        return 4 - (hero->secSkill[eSecondary::DIPLOMACY]) + (hero->secSkill[eSecondary::LEARNING]) +
               (hero->secSkill[eSecondary::ESTATES]) + (hero->secSkill[eSecondary::SCHOLAR]);
    case eSecondary::LUCK:
    case eSecondary::RESISTANCE:
    case eSecondary::EAGLE_EYE:
        return 4 - (hero->secSkill[eSecondary::LUCK]) + (hero->secSkill[eSecondary::RESISTANCE]) +
               (hero->secSkill[eSecondary::EAGLE_EYE]) +
               ((hero->secSkill[eSecondary::NECROMANCY]) || (hero->secSkill[eSecondary::LEADERSHIP]));
    case eSecondary::LEADERSHIP:
    case eSecondary::NECROMANCY:
        return 4 - (hero->secSkill[eSecondary::LUCK]) + (hero->secSkill[eSecondary::RESISTANCE]) +
               (hero->secSkill[eSecondary::EAGLE_EYE]);
    default:
        break;
    }

    return 4;
}

int __stdcall AI_H3Hero_GetSkillValue(HiHook *h, const H3Hero *hero, const eSecondary skillId, const char complexChoice)
{

    const int result = FASTCALL_3(int, h->GetDefaultFunc(), hero, skillId, complexChoice);

    // if hero does not have the skill
    if (hero->secSkill[skillId] == eSecSkillLevel::NONE)
    {
        const int skillsNeeded = Hero_GetSkillsNumberToReachComboSkill(hero, skillId);

        switch (skillsNeeded)
        {
        case 1:
            return 1000000;

        case 2:
            // free skill slots available
            if (IntAt(0x4DAFCE) - hero->secSkillCount > 2)
            {
                return 1000000;
            }
            break;
        default:
            break;
        }
    }

    return result;
}
char __stdcall AI_H3Hero_WantsSecondarySkills(HiHook *h, const H3Hero *hero, const eSecondary skillId,
                                              const char complexChoice)

{
    if (hero->secSkill[skillId] == eSecSkillLevel::NONE)
    {

        const int skillsNeeded = Hero_GetSkillsNumberToReachComboSkill(hero, skillId);
        switch (skillsNeeded)
        {
        case 1:
            return true;
        case 2:
            // check if there is enough free skill slots
            if (IntAt(0x4DAFCE) - hero->secSkillCount > 2)
            {
                return true;
            }
            break;
        default:
            break;
        }
    }

    return FASTCALL_3(char, h->GetDefaultFunc(), hero, skillId, complexChoice);
}
int __stdcall AI_H3Hero_ChooseLevelUpSecondarySkill(HiHook *h, const H3Hero *hero, const eSecondary leftSkillId,
                                                    const eSecondary rightSkillId, const char complexChoice)
{

    if (leftSkillId != eSecondary::NONE && rightSkillId != eSecondary::NONE &&

        hero->secSkill[leftSkillId] == eSecSkillLevel::NONE && hero->secSkill[rightSkillId] == eSecSkillLevel::NONE)
    {
        const int leftSkillsNeeded = Hero_GetSkillsNumberToReachComboSkill(hero, leftSkillId);
        const int rightSkillsNeeded = Hero_GetSkillsNumberToReachComboSkill(hero, rightSkillId);
        if (leftSkillsNeeded < 3 && leftSkillsNeeded < rightSkillsNeeded)
        {
            return leftSkillId;
        }
        else if (rightSkillsNeeded < 3 && rightSkillsNeeded < leftSkillsNeeded)
        {
            return rightSkillId;
        }
    }

    return FASTCALL_4(char, h->GetDefaultFunc(), hero, leftSkillId, rightSkillId, complexChoice);
}

_LHF_(HooksInit)
{

    _PI->WriteHiHook(0x0524B70, FASTCALL_, AI_H3Hero_GetSkillValue);
    _PI->WriteHiHook(0x05252B0, FASTCALL_, AI_H3Hero_WantsSecondarySkills);
    _PI->WriteHiHook(0x052C0B0, FASTCALL_, AI_H3Hero_ChooseLevelUpSecondarySkill);

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
