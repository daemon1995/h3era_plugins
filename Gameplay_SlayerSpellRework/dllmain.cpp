// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
using namespace h3;
namespace dllText
{
constexpr const char *PLUGIN_AUTHOR = "daemon_n";
constexpr const char *PLUGIN_VERSION = "1.0";
constexpr const char *PLUGIN_DATA = __DATE__;
constexpr const char *INSTANCE_NAME = "EraPlugin.Gameplay_SlayerSpellRework.daemon_n";
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

float slayerSpecialtyPower = 0.05f;

int __stdcall BattleStack_CalculateDamageBonus(HiHook *h, H3CombatCreature *_this, signed int damage, char shoot,
                                               H3CombatCreature *enemy, int IsRealDamage, signed int StepsTaken)
{
    int result = THISCALL_6(int, h->GetDefaultFunc(), _this, damage, shoot, enemy, IsRealDamage, StepsTaken);
    if (_this->activeSpellDuration[eSpell::SLAYER])
    {
        if (float slayerPower = P_Spell[eSpell::SLAYER].baseValue[_this->slayerType])
        {
            if (auto hero = _this->GetOwner())
            {
                const auto &speciality = P_HeroSpecialty[hero->id];
                if (speciality.type == eHeroSpecialty::SPELL && speciality.GetSpell() == eSpell::SLAYER)
                {
                    slayerPower += ceil(slayerPower * static_cast<float>(hero->level / (_this->info.level + 1)) *
                                        slayerSpecialtyPower);
                }
            }
            // apply slayer bonus
            result += static_cast<int>(static_cast<float>(damage * (slayerPower / 100)));
        }
    }
    return result;
}
struct RemoveObstacleCasting
{
    BOOL isRemoveObstacleCasting = FALSE;
    eSecSkillLevel removeObstacleSkillLevel = eSecSkillLevel::NONE; // store Remove Obstacle skill level
    INT spellEffect[4] = {0, 0, 0, 0};                              // store spell durations
} removeObstacleCasting;

void __stdcall OnSetupBattlefield(Era::TEvent *e)
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
                int currentSpeedDecrease = 0;
                if (!stack->activeSpellDuration[eSpell::REMOVE_OBSTACLE])
                {
                    stack->activeSpellNumber += 1; // increase active spell number
                }
                else
                {
                    currentSpeedDecrease =
                        removeObstacleCasting.spellEffect[stack->activeSpellLevel[eSpell::REMOVE_OBSTACLE]];
                }

                if (stack->activeSpellLevel[eSpell::REMOVE_OBSTACLE] < spellEffect)
                {
                    stack->activeSpellLevel[eSpell::REMOVE_OBSTACLE] = spellEffect; // remove Remove Obstacle spell
                }

                stack->activeSpellDuration[eSpell::REMOVE_OBSTACLE] = 255; // remove Remove Obstacle spell
                speed -= spellEffect - currentSpeedDecrease;               // remove speed penalty

                if (speed < 1)
                {
                    speed = 1;
                }
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

    // if (IntAt(c->ebp + 0x8) == eSpell::REMOVE_OBSTACLE)
    //{
    //     ByteAt(0x05A0F6B +1) = eSpell::REMOVE_OBSTACLE;
    //     return NO_EXEC_DEFAULT;
    // }

    // cast areae spell void __thiscall CombatMan_CastAreaSpell(_BattleMgr_ *this, int GEX_ID, eSpells spellId, int
    // spel_lvl, int spellPower)
    removeObstacleCasting.isRemoveObstacleCasting = true; // set flag to apply speed penalty
    removeObstacleCasting.removeObstacleSkillLevel = eSecSkillLevel(c->esi);
    THISCALL_5(void, 0x05A4C80, c->ebx, IntAt(c->ebp + 0xC), eSpell::REMOVE_OBSTACLE, c->esi, IntAt(c->ebp + 0x1C));
    removeObstacleCasting.isRemoveObstacleCasting = false; // reset flag
    c->return_address = 0x05A2368;

    return NO_EXEC_DEFAULT;
}
void __stdcall BattleMgr_PrepareMessageTo_BattleLog(HiHook *h, H3CombatManager *bm, int spellId, int targetHex, int a4)
{
    if (spellId == eSpell::REMOVE_OBSTACLE)
    {
        libc::sprintf(h3_TextBuffer, P_GeneralText->GetText(28), P_Spell[eSpell::REMOVE_OBSTACLE].name);
        bm->dlg->ShowHint(h3_TextBuffer);
        return;
    }
    THISCALL_4(void, h->GetDefaultFunc(), bm, spellId, targetHex, a4);
}

void __stdcall OnAfterWog(Era::TEvent *event)
{
    _PI->WriteDword(0x0442178, 0xA4E9);
    _PI->WriteByte(0x0442178 + 5, 0x90);

    _PI->WriteHiHook(0x0443040, THISCALL_, BattleStack_CalculateDamageBonus);
    //_PI->WriteHiHook(0x0443040, THISCALL_, BattleStack_GetSpeed);

    slayerSpecialtyPower = Clamp(1, EraJS::readInt("shimmy.spells.55.speciality"), 100) / static_cast<float>(100);

    _PI->WriteByte(0x059F9BC + 54, 5); // set chosen remove obstacle spell to Inferno spell
    _PI->WriteByte(0x043BB64 + 2, 2);  //  AI now handles Remove Obstacle spell as area casting spell
    // _PI->WriteByte(0x05A2B7E, 12);     //  now Remove Obstacle spell is Inferno spell
    //  _PI->WriteByte(0x05A8C5C, 0); //  now Remove Obstacle spell hint text is same as Inferno spell hint text

    _PI->WriteLoHook(0x05A1F14, BattleMgr__CastRemoveObstacleSpell);
    _PI->WriteHiHook(0x05A18FF, THISCALL_, BattleMgr__TryToDispellSpell); // target dispell cast
    _PI->WriteHiHook(0x05A198D, THISCALL_, BattleMgr__TryToDispellSpell); // area dispell cast
    _PI->WriteHiHook(0x0443F58, THISCALL_, BattleMgr__TryToDispellSpell); // stack dies

    _PI->WriteHiHook(0x05A4DED, THISCALL_, BattleStack_DoPhysicalDamageAtAreaSpellCast);
    _PI->WriteHiHook(0x05A4A00, THISCALL_, BattleMgr_ShowSpellsHightLightes);
    _PI->WriteHiHook(0x059FD3F, THISCALL_, BattleMgr_ShowSpellsHightLightes);
    Era::RegisterHandler(OnSetupBattlefield, "OnSetupBattlefield");
    if (globalPatcher->GetInstance("ERA Spells Description"))
    {
        _PI->WriteHiHook(0x05A89A0, THISCALL_, BattleMgr_PrepareMessageTo_BattleLog);
    }

    //
    //
    //
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
            Era::RegisterHandler(OnAfterWog, "OnAfterWog");
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
