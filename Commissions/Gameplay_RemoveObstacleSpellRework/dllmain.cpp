// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
using namespace h3;
namespace dllText
{
constexpr const char *PLUGIN_AUTHOR = "daemon_n";
constexpr const char *PLUGIN_VERSION = "1.0";
constexpr const char *PLUGIN_DATA = __DATE__;
constexpr const char *INSTANCE_NAME = "EraPlugin.Gameplay_RemoveObstacleSpellRework.daemon_n";
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

struct RemoveObstacleCasting
{
    BOOL isRemoveObstacleCasting = FALSE;
    eSecSkillLevel removeObstacleSkillLevel = eSecSkillLevel::NONE; // store Remove Obstacle skill level
    INT spellEffect[4] = {0, 0, 0, 0};                              // store spell durations
} removeObstacleCasting;

_ERH_(OnSetupBattlefield)
{

    for (size_t i = 0; i < 4; i++)
    {
        libc::sprintf(h3_TextBuffer, "shimmy_blizzard_spell_speed_decreasing_%d", i);
        removeObstacleCasting.spellEffect[i] = Era::GetAssocVarIntValue(h3_TextBuffer);
    }
}
int __stdcall BattleStack_DoPhysicalDamageAtAreaSpellCast(HiHook *h, H3CombatCreature *stack, const int damage)
{

    const int result = THISCALL_2(int, h->GetDefaultFunc(), stack, damage);
    if (removeObstacleCasting.isRemoveObstacleCasting)
    {
        if (auto &speed = stack->info.speed)
        {
            const int spellEffect = removeObstacleCasting.spellEffect[removeObstacleCasting.removeObstacleSkillLevel];
            if (spellEffect)
            {
                //  int currentSpeedDecrease = 0;
                if (!stack->activeSpellDuration[eSpell::REMOVE_OBSTACLE])
                {
                    stack->activeSpellNumber += 1; // increase active spell number

                    speed -= spellEffect; // remove speed penalty

                    if (speed < 1)
                    {
                        speed = 1;
                    }
                }
                stack->activeSpellDuration[eSpell::REMOVE_OBSTACLE] = 255; // renew Remove Obstacle spell
            }
        }
    }
    return result;
}

void __stdcall BattleMgr_ShowSpellsHightLightes(HiHook *h, H3CombatManager *bm, signed int hexId, int radius,
                                                int includeCenter, DWORD result)
{

    if (bm->actionParameter == eSpell::REMOVE_OBSTACLE)
    {
        radius = 2; // replace Remove Obstacle with Inferno
    }
    THISCALL_5(void, h->GetDefaultFunc(), bm, hexId, radius, includeCenter, result);
}
void __stdcall BattleMgr__TryToDispellSpell(HiHook *h, H3CombatCreature *stack, eSpell spellId)
{
    if (spellId == eSpell::REMOVE_OBSTACLE)
    {
        // skip Remove Obstacle spell dispelling
        return;
    }
    THISCALL_2(void, h->GetDefaultFunc(), stack, spellId);
}
_LHF_(BattleMgr__CastRemoveObstacleSpell)
{

    // cast areae spell void __thiscall CombatMan_CastAreaSpell(_BattleMgr_ *this, int GEX_ID, eSpells spellId, int
    // spel_lvl, int spellPower)
    removeObstacleCasting.isRemoveObstacleCasting = true; // set flag to apply speed penalty
    removeObstacleCasting.removeObstacleSkillLevel = eSecSkillLevel(c->esi);
    THISCALL_5(void, 0x05A4C80, c->ebx, IntAt(c->ebp + 0xC), eSpell::REMOVE_OBSTACLE, c->esi, IntAt(c->ebp + 0x1C));
    removeObstacleCasting.isRemoveObstacleCasting = false; // reset flag
    c->return_address = 0x05A2368;

    return NO_EXEC_DEFAULT;
}
void __stdcall BattleMgr_PrepareMessageTo_BattleLog(HiHook *h, H3CombatManager *bm, const int spellId,
                                                    const int targetHex, const int a4)
{
    if (spellId == eSpell::REMOVE_OBSTACLE)
    {
        libc::sprintf(h3_TextBuffer, P_GeneralText->GetText(28), P_Spell[eSpell::REMOVE_OBSTACLE].name);
        bm->dlg->ShowHint(h3_TextBuffer);
        return;
    }
    THISCALL_4(void, h->GetDefaultFunc(), bm, spellId, targetHex, a4);
}
void __stdcall BattleStack_DrawOrRedraw(HiHook *h, H3CombatCreature *stack, DWORD a2, DWORD a3, DWORD a4)
{
    const auto stored = P_Spell[eSpell::REMOVE_OBSTACLE].type; // store original spell type
    P_Spell[eSpell::REMOVE_OBSTACLE].type = eSpellTarget::ENEMY;
    THISCALL_4(int, h->GetDefaultFunc(), stack, a2, a3, a4);
    P_Spell[eSpell::REMOVE_OBSTACLE].type = stored;
}
_ERH_(OnAfterErmInstructions)
{
    // set Remove Obstacle spell level to Inferno spell level
    auto &spell = P_Spell[eSpell::REMOVE_OBSTACLE];
    spell.flags = P_Spell[eSpell::INFERNO].flags;
    spell.level = 5;
    spell.school = eSpellchool::WATER;
}
_ERH_(OnAfterWog)
{

    // blizzard spell rework

    _PI->WriteByte(0x059F9BC + 54, 5); // set chosen remove obstacle spell to Inferno spell
    _PI->WriteByte(0x043BB64 + 2, 2);  //  AI now handles Remove Obstacle spell as area casting spell
    // _PI->WriteByte(0x05A2B7E, 12);     //  now Remove Obstacle spell is Inferno spell
    //  _PI->WriteByte(0x05A8C5C, 0); //  now Remove Obstacle spell hint text is same as Inferno spell hint text

    _PI->WriteLoHook(0x05A1F14, BattleMgr__CastRemoveObstacleSpell);
    _PI->WriteHiHook(0x05A18FF, THISCALL_, BattleMgr__TryToDispellSpell); // target dispell cast
    _PI->WriteHiHook(0x05A198D, THISCALL_, BattleMgr__TryToDispellSpell); // area dispell cast
    _PI->WriteHiHook(0x0443F58, THISCALL_, BattleMgr__TryToDispellSpell); // stack dies

    _PI->WriteHiHook(0x043DE60, THISCALL_, BattleStack_DrawOrRedraw);

    _PI->WriteHiHook(0x05A4DED, THISCALL_, BattleStack_DoPhysicalDamageAtAreaSpellCast);
    _PI->WriteHiHook(0x05A4A00, THISCALL_, BattleMgr_ShowSpellsHightLightes);
    _PI->WriteHiHook(0x059FD3F, THISCALL_, BattleMgr_ShowSpellsHightLightes);
    _REH_(OnSetupBattlefield);
    if (globalPatcher->GetInstance("ERA Spells Description"))
    {
        _PI->WriteHiHook(0x05A89A0, THISCALL_, BattleMgr_PrepareMessageTo_BattleLog);
    }
    H3MagicAnimation::GetAnim(eSpell::REMOVE_OBSTACLE)->type =
        H3MagicAnimation::GetAnim(eSpell::INFERNO)
            ->type; // set Remove Obstacle spell animation type to Inferno spell animation
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginOn)
        {
            pluginOn = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            _REH_(OnAfterWog);
            _REH_(OnAfterErmInstructions);
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
