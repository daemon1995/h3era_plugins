#include "../../include/Plugin.h"
#include "../../include/PluginRng.h"
#include <algorithm>

namespace gbf
{
namespace game_logic_remaining
{
int __stdcall ScholarChoice(LoHook *, HookContext *c)
{
    const int skill = c->edi;
    auto *hero = reinterpret_cast<h3::H3Hero *>(c->ebx);
    const char *text = P_AdveventText->GetText(116);
    FASTCALL_12(void, 0x4F6C00, text, 2, -1, -1, 20, hero->secSkill[skill] + 3 * skill + 2, -1, 0, -1, 0, -1, 0);
    if (h3::H3WindowManager::Get()->resultItemID != 30725)
    {
        if (hero->secSkill[skill] == 1)
        {
            --hero->secSkillCount;
            hero->secSkillPosition[skill] = 0;
        }
        --hero->secSkill[skill];
    }
    c->return_address = 0x4A4B86;
    return NO_EXEC_DEFAULT;
}

void __stdcall TownCombatManager(HiHook *hook, h3::H3ExecutiveMgr *exec, h3::H3Manager *manager)
{
    auto *townManager = h3::H3TownManager::Get();
    auto *combat = h3::H3CombatManager::Get();
    if (exec->active_mgr == townManager && manager == combat)
    {
        exec->RemoveManager(townManager);
        exec->AddManager(manager, -1);
        THISCALL_1(h3::H3Manager *, 0x4B0BA0, exec);
        exec->RemoveManager(manager);
        const DWORD pos = *reinterpret_cast<const DWORD *>(reinterpret_cast<const char *>(combat) + 0x13FF0);
        const auto signed10 = [](DWORD v) {
            const int n = int(v & 1023);
            return n >= 512 ? n - 1024 : n;
        };
        const int z = int((pos >> 26) & 15);
        auto *main = h3::H3Main::Get();
        const int id = THISCALL_4(int, 0x4BB530, main, signed10(pos), signed10(pos >> 16), z >= 8 ? z - 16 : z);
        townManager->town = id >= 0 && static_cast<UINT>(id) < main->towns.Size() ? &main->towns[id] : nullptr;
        exec->AddManager(townManager, -1);
        exec->active_mgr = townManager;
    }
    else
        THISCALL_2(void, hook->GetDefaultFunc(), exec, manager);
}
// © SadnessPower
// Пожиратель душ ранее воскрешал стеки существ, уровень которых больше 4,
// если их здоровье было меньше 50
int __stdcall BattleStack_AtGettingResurrectionResistance(LoHook *, HookContext *c)
{
    // если кастует стек
    if (IntAt(c->ebp + 0x1C))
    {
        if (const auto activeStack = h3::H3CombatManager::Get()->activeStack)
        {
            // если кастует Пожиратель Душ
            if (activeStack->type == 178 || activeStack->type == 187)
            {
                if (reinterpret_cast<h3::H3CombatCreature *>(c->edi)->info.level > 4)
                {
                    // ставим иммунитет к касту
                    c->return_address = 0x05A8824;
                    return NO_EXEC_DEFAULT;
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

// © JackSlater
// Фикс бага SoD - Невозможно колдовать более сильную версию заклинания "Полёт"
// Если герой уже колдовал его на этот ход
int __stdcall js_Cast_AdventureMagic_BeforeCheckFlyPower(LoHook *, HookContext *c)
{
    if (const auto *hero = reinterpret_cast<h3::H3Hero *>(c->esi))
    {
        // cast power is more than current fly power
        const auto &spell = h3::H3Spell::Get()[6];
        const int currentPower = hero->flyPower;
        if (currentPower >= 0 && currentPower < 4 && c->edi >= 0 && c->edi < 4 &&
            spell.baseValue[c->edi] > spell.baseValue[currentPower])
        {
            c->return_address = 0x041C886;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}

// Фикс бага SoD - усилители заклинаний от героя не работали при касте существами
int __stdcall js_BattleMgr_CastSpell_BeforeSwitchCase(LoHook *, HookContext *c)
{
    // if monster casts spell and it affects enemy stack
    if (IntAt(c->ebp + 0x10) == 1 && h3::H3Spell::Get()[IntAt(c->ebp + 0x8)].type == -1)
    {
        // set active hero from function return
        auto *mgr = h3::H3CombatManager::Get();
        *reinterpret_cast<h3::H3Hero **>(c->ebp - 0x14) =
            THISCALL_1(h3::H3Hero *, 0x04423B0, &mgr->stacks[mgr->currentActiveSide][mgr->currentMonIndex]);
    }
    return EXEC_DEFAULT;
}

void __stdcall OnGameLeave(Era::TEvent *)
{
    ByteAt(0x06AAAC4) = false;
}

static thread_local PluginRNG g_pluginAIRng;
int wogTownDemolishionType = 0;
static void __stdcall BattleMgr_SetRandomSeed(HiHook *h, unsigned int seed)
{
    // исправляем значение настройки опции "каст заклинаний в автобитве", которая иногда может быть не равна 0 или 1,
    // что ломает логику проверки в игре
    IntAt(0x06987E8) = std::clamp(IntAt(0x06987E8), 0, 1);

    THISCALL_1(void, h->GetDefaultFunc(), seed);
    CombatVisualRng().srand(seed);
    g_pluginAIRng.srand(seed);

    // g_pluginAIQucikRng.srand(seed);
    // g_pluginDebugRng.srand(seed);
}

static int __stdcall BattleMgrProc_AnimationRandom(HiHook *h, const int low, const int high)
{
    return CombatVisualRng().range(low, high);
}

static unsigned int __cdecl BattleMgrProc_AnimationPureRandom()
{
    return CombatVisualRng().next();
}

static int __stdcall BattleMgrProc_AIRandom(HiHook *h, const int low, const int high)
{
    return g_pluginAIRng.range(low, high);
}

int __stdcall TownDemolishion_BeforeBuildingRebuild(LoHook *, HookContext *c)
{
    h3::H3Town *town = h3::H3TownManager::Get()->town;
    if (town && !town->IsBuildingBuilt(7))
    {
        wogTownDemolishionType = 1;
        CDECL_1(void, 0x070A999, -1); // меняем индекс на массив с префектурами (хотя этот код и работаает, но
                                      // игра позднее вызывает эту же ф-цию и перезатирает результат)
    }

    return EXEC_DEFAULT;
}

int __stdcall TownDemolishion_BeforeGraphicsChange(LoHook *, HookContext *c)
{
    if (wogTownDemolishionType)
    {
        // 70AA96 calls a cdecl function with flags on the stack, not in ECX.
        IntAt(c->esp) = -1;
        wogTownDemolishionType = 0;
    }
    return EXEC_DEFAULT;
}

// убираем обновление инфо-панели, если активный менеджер не менеджер карты приключений
int __stdcall Y_AdvMgr_RedrawInfoPanel(LoHook *, HookContext *c)
{
    if (!h3::H3ExecutiveMgr::Get()->active_mgr ||
        h3::H3ExecutiveMgr::Get()->active_mgr == h3::H3AdventureManager::Get())
        return EXEC_DEFAULT;

    c->return_address = 0x415E5D;
    return NO_EXEC_DEFAULT;
}

int globRecord = 0;
// by igrik /////////////////////////////////////////////////////////////////////////////////////////
int __stdcall Y_SetCanselWitchHut(HiHook *hook, h3::H3Hero *hero, int skill, BYTE skill_lvl)
{
    if (h3::H3WindowManager::Get()->resultItemID == 30726)
    {
        return 0;
    }

    return THISCALL_3(int, hook->GetDefaultFunc(), hero, skill, skill_lvl);
}

int __stdcall Y_Fix_HeroesInteract(LoHook *, HookContext *c)
{
    // восстанавливаем затёртую и важную команду игры
    c->ebx = c->ecx;
    // сохраняем номер героя, чтобы потом восстановить
    int HeroRid = IntAt(DwordAt(c->ebp + 0xC));

    // получаем структуру героя правой стороны
    h3::H3Hero *HeroL = *(h3::H3Hero **)(c->ebp + 0x8);

    if (!HeroL || HeroRid < 0 || HeroRid > 255 || HeroL->id == HeroRid)
    {
        // запрещаем вызов обмена героями
        c->return_address = 0x4A251F;
        return NO_EXEC_DEFAULT;
    }

    h3::H3AdventureManager::Get()->DemobilizeHero();
    // после выполнения DeMobilize(), в (c->ebp +0xC) в ряде случаев портится указатель на номер героя
    // поэтому мы сначала сохранили номер героя, а щас восстанавливаем
    c->ecx = HeroRid;

    // пропускаем стандартный код игры (мы его выполнили сами)
    c->return_address = 0x4A2486;
    return NO_EXEC_DEFAULT;
}

int __stdcall Y_Fix_HeroesOnWaterCheckInteract(LoHook *, HookContext *c)
{
    const h3::H3MapItem *mapItem = reinterpret_cast<h3::H3MapItem *>(c->eax);
    const h3::H3Hero *heroRight = mapItem ? h3::H3Main::Get()->GetHero(mapItem->setup) : nullptr;
    c->eax = mapItem && ((heroRight && (heroRight->flags & 0x40000)) || mapItem->land == 8);
    c->return_address = 0x4814DD;
    return NO_EXEC_DEFAULT;
}

int __stdcall HiHook_004aa820(HiHook *h, h3::H3AdventureManager *a2, h3::H3MapItem *a3, int XYZ, int record)
{
    const int previousRecord = globRecord;
    globRecord = record;
    const int result = THISCALL_4(int, h->GetDefaultFunc(), a2, a3, XYZ, record);
    globRecord = previousRecord;
    return result;
}

int __stdcall LoHook_004AA9B3(LoHook *, HookContext *c)
{

    struct LocalBoat
    {
        INT16 x;
        /** @brief [02]*/
        INT16 y;
        /** @brief [04]*/
        INT16 z;
        /** @brief [06]*/
        INT8 visible;
        /** @brief [07] no clue how to get this offset without align-1, may be substructure*/
        h3::H3MapItem *item;
        char _f_0B;
        /** @brief [0C]*/
        INT32 objectType;
        /** @brief [10]*/
        INT8 objectFlag;
        char _f_11[3];
        /** @brief [14]*/
        INT32 objectSetup;
        /** @brief [18]*/
        INT8 exists;
        /** @brief [19]*/
        INT8 index;
        /** @brief [1A]*/
        INT8 par1;
        /** @brief [1B]*/
        INT8 par2;
        /** @brief [1C]*/
        INT8 owner;
        char _f_1D[3];
        /** @brief [20]*/
        INT32 heroId;
        /** @brief [24]*/
        char hasHero;
        char _f_25[3];
    };
    h3::H3MapItem *cell; // esi
    h3::H3Hero *hero;    // ebx
    h3::H3Boat *v4;      // edi

    cell = (h3::H3MapItem *)c->edx;
    hero = 0;
    v4 = 0;
    if (cell->objectType == 34)
    {
        hero = h3::H3Main::Get()->GetHero(cell->setup);
        if (hero)
            hero->Hide();
    }
    if (cell->objectType == 8)
    {
        auto &boats = h3::H3Main::Get()->boats;
        if (cell->setup >= 0 && static_cast<UINT>(cell->setup) < boats.Size())
        {
            v4 = &boats[cell->setup];
            // Like heroes, boats must be hidden before clearing the object below.
            // Show is a no-op while visible and cannot restore a cleared setup.
            THISCALL_1(void, 0x4D7950, v4);
        }
    }
    cell->setup = -1;
    if (v4)
        THISCALL_3(void, 0x4D7840, v4, 8, v4->index);
    if (hero)
        hero->Show();
    return 0;
}

int __stdcall LoHook_004AA9BF(LoHook *, HookContext *c)
{
    return globRecord != 0;
}

Patch *skipMeleeBerserkReset = nullptr;
Patch *skipRangeBerserkReset = nullptr;
h3::H3CombatCreature *combatActionAttacker = nullptr;
h3::H3CombatCreature *afterSorceressAbilityTarget = nullptr;
h3::H3CombatCreature *afterHellSteedAbilityTarget = nullptr;
h3::H3CombatCreature *afterAnyAttackAbilityTarget = nullptr;

// Preserve the attacker context if another hook reenters PrepareMelee/PrepareShoot.
struct AttackContextScope
{
    Patch *patch;
    bool previousPatchState;
    h3::H3CombatCreature *previousAttacker;
    h3::H3CombatCreature *previousSorceressTarget;
    h3::H3CombatCreature *previousHellSteedTarget;
    h3::H3CombatCreature *previousAbilityTarget;

    AttackContextScope(h3::H3CombatCreature *attacker, Patch *resetPatch)
        : patch(resetPatch), previousPatchState(patch && patch->IsApplied()), previousAttacker(combatActionAttacker),
          previousSorceressTarget(afterSorceressAbilityTarget), previousHellSteedTarget(afterHellSteedAbilityTarget),
          previousAbilityTarget(afterAnyAttackAbilityTarget)
    {
        if (patch)
        {
            if (attacker->activeSpellDuration[62] == 0)
                patch->Apply();
            else
                patch->Undo();
        }
        combatActionAttacker = attacker;
        afterSorceressAbilityTarget = nullptr;
        afterHellSteedAbilityTarget = nullptr;
        afterAnyAttackAbilityTarget = nullptr;
    }
    ~AttackContextScope()
    {
        if (patch)
        {
            if (previousPatchState)
                patch->Apply();
            else
                patch->Undo();
        }
        combatActionAttacker = previousAttacker;
        afterSorceressAbilityTarget = previousSorceressTarget;
        afterHellSteedAbilityTarget = previousHellSteedTarget;
        afterAnyAttackAbilityTarget = previousAbilityTarget;
    }
};
void __stdcall BattleStack_PrepareMelee(HiHook *h, h3::H3CombatCreature *attacker, int direction)
{
    const AttackContextScope scope(attacker, skipMeleeBerserkReset);
    THISCALL_2(void, h->GetDefaultFunc(), attacker, direction);
}
void __stdcall BattleStack_PrepareShoot(HiHook *h, h3::H3CombatCreature *attacker)
{
    const AttackContextScope scope(attacker, skipRangeBerserkReset);
    THISCALL_1(void, h->GetDefaultFunc(), attacker);
}
struct CreatureSpellData
{
    const eSpell spellID = eSpell::NONE;
    const int chance = 0;
    static constexpr int MaxSpells = 16;
};
struct
{
    static constexpr CreatureSpellData basedOnCreatureAmountSpell[4][CreatureSpellData::MaxSpells] = {
        {
            {eSpell::WEAKNESS, 25},
            {eSpell::DISRUPTING_RAY, 25},
            {eSpell::MISFORTUNE, 25},
            {eSpell::DISEASE, 25},
            {eSpell::NONE, 0},
        },
        {

            {eSpell::SLOW, 20},
            {eSpell::CURSE, 20},
            {eSpell::DRAGONFLY_DISPEL, 20},
            {eSpell::MISFORTUNE, 10},
            {eSpell::DISRUPTING_RAY, 10},
            {eSpell::DISEASE, 10},
            {eSpell::WEAKNESS, 10},
            {eSpell::NONE, 0}

        },
        {
            {eSpell::POISON, 15},
            {eSpell::SORROW, 15},
            {eSpell::ACID_BREATH, 15},
            {eSpell::FORGETFULNESS, 15},
            {eSpell::SLOW, 8},
            {eSpell::CURSE, 8},
            {eSpell::DRAGONFLY_DISPEL, 8},
            {eSpell::MISFORTUNE, 8},
            {eSpell::DISRUPTING_RAY, 8},
            {eSpell::DISEASE, 8},
            {eSpell::WEAKNESS, 8},
            {eSpell::NONE, 0},
        },
        {

            {eSpell::HYPNOTIZE, 4},
            {eSpell::BLIND, 4},
            {eSpell::BERSERK, 4},
            {eSpell::AGING, 4},
            {eSpell::POISON, 11},
            {eSpell::SORROW, 11},
            {eSpell::ACID_BREATH, 11},
            {eSpell::FORGETFULNESS, 11},
            {eSpell::SLOW, 8},
            {eSpell::CURSE, 8},
            {eSpell::DRAGONFLY_DISPEL, 8},
            {eSpell::MISFORTUNE, 8},
            {eSpell::DISRUPTING_RAY, 8},
            {eSpell::DISEASE, 8},
            {eSpell::WEAKNESS, 8},
            {eSpell::NONE, 0},
        }

    };

} constexpr wogCreatureSpellData;

eSpell GetRandomDebuffSpellToCast(h3::H3CombatCreature *atkStack, h3::H3CombatCreature *targetMon)
{

    const int creatureNum = atkStack->numberAlive;
    const int arrayIndex = (creatureNum < 10) ? 0 : (creatureNum < 20) ? 1 : (creatureNum < 50) ? 2 : 3; // hate that
    const auto &spellDataArray = wogCreatureSpellData.basedOnCreatureAmountSpell[arrayIndex];
    int totalChance = 0;

    const int casterSide = atkStack->GetSide();

    for (const auto &spellData : spellDataArray)
    {
        if (spellData.spellID == eSpell::NONE)
            break;

        if (THISCALL_6(bool, 0x5A3F90, h3::H3CombatManager::Get(), spellData.spellID, casterSide, targetMon, 1, 1))
            totalChance += spellData.chance;
    }

    if (totalChance == 0)
        return eSpell::NONE;

    int randomValue = h3::H3Random::Rand() % totalChance;
    for (const auto &spellData : spellDataArray)
    {
        if (spellData.spellID == eSpell::NONE)
            break;

        if (!THISCALL_6(bool, 0x5A3F90, h3::H3CombatManager::Get(), spellData.spellID, casterSide, targetMon, 1, 1))
            continue;

        if (randomValue < spellData.chance)
            return spellData.spellID;

        randomValue -= spellData.chance;
    }
    return eSpell::NONE;
}

int __stdcall OnBeforeSorceressShot(LoHook *, HookContext *c)
{
    afterSorceressAbilityTarget = reinterpret_cast<h3::H3CombatCreature *>(c->eax);
    c->return_address = 0x075CA76;
    return NO_EXEC_DEFAULT;
}
int __stdcall OnBeforeHellSteedAttack(LoHook *, HookContext *c)
{
    afterHellSteedAbilityTarget = reinterpret_cast<h3::H3CombatCreature *>(c->eax);
    c->return_address = 0x075CA76;
    return NO_EXEC_DEFAULT;
}
int __stdcall OnBeforeCreatureAttack(LoHook *, HookContext *c)
{
    afterAnyAttackAbilityTarget = reinterpret_cast<h3::H3CombatCreature *>(c->eax);
    c->return_address = 0x075CAFE;
    return NO_EXEC_DEFAULT;
}

eSpell WoG_GetStackSpellToCast(h3::H3CombatCreature *atkStack, h3::H3CombatCreature *targetMon,
                               const eSpell fixedSpellToCast)
{

    const eSpell spellToCast =
        fixedSpellToCast != eSpell::NONE ? fixedSpellToCast : GetRandomDebuffSpellToCast(atkStack, targetMon);

    if (spellToCast != eSpell::NONE &&
        THISCALL_6(BYTE, 0x05A8950, h3::H3CombatManager::Get(), spellToCast, atkStack->GetSide(), targetMon, 1, 1))
    {
        return spellToCast;
    }

    return eSpell::NONE;
}

void __stdcall OnAfterAttackDrawActionPlay(HiHook *h, h3::H3CombatManager *mgr, const int anim, const int showAttack)
{

    THISCALL_3(void, h->GetDefaultFunc(), mgr, anim, showAttack);
    h3::H3CombatCreature *atkStack = &mgr->stacks[mgr->currentActiveSide][mgr->currentMonIndex];

    if (atkStack != combatActionAttacker || !combatActionAttacker)
    {
        return;
    }
    const int creatureNum = atkStack->numberAlive;

    // sorcery shooting attack
    auto target = afterSorceressAbilityTarget;
    if (target && target->numberAlive > 0 &&
        (creatureNum >= 40 || (creatureNum > 0 && (creatureNum << 1) + 20 > h3::H3Random::Rand(1, 100))))
    {
        const eSpell spellId = WoG_GetStackSpellToCast(atkStack, target, eSpell::NONE);
        if (spellId != eSpell::NONE)
        {
            target->spellToApply = spellId;
            THISCALL_3(BYTE, 0x468570, mgr, -1, showAttack);
        }
    }

    // hell steed Fire Wall Attack
    target = afterHellSteedAbilityTarget;
    if (target && target->numberAlive > 0 && creatureNum > 0)
    {
        const eSpell spellId = WoG_GetStackSpellToCast(atkStack, target, eSpell::FIRE_WALL);
        if (spellId != eSpell::NONE)
        {
            target->spellToApply = spellId;
            THISCALL_3(BYTE, 0x468570, mgr, -1, showAttack);
        }
    }

    // wog ability cast
    target = afterAnyAttackAbilityTarget;
    if (!target || target->numberAlive < 1 || creatureNum < 1 || IntAt(0x079F138) || // if Complete AI combat
        !CDECL_2(BOOL, 0x071C16D, atkStack, 1)) // or CrExpo isn't prepared for attacker stack
        return;

    struct CrExpBonStr
    {
        unsigned __int32 Act : 1;
        unsigned __int32 _un : 31;
        char Type;
        unsigned __int8 Mod;
        unsigned char Lvls[11];
    };
    const CrExpBonStr *bonusStr = *reinterpret_cast<CrExpBonStr **>(0x071D826 + 2);
    const int expLvl = IntAt(0x0847D94);
    if (!bonusStr || expLvl < 0 || expLvl >= 11)
        return;

    for (size_t i = 0; i < 14; i++)
    {
        auto &bonus = bonusStr[i];
        if (bonus.Act && bonus.Type == 99)
        {

            const eSpell spellToCast = eSpell(bonus.Mod);
            const int chanceToCast = bonus.Lvls[expLvl];
            if (spellToCast < 0 || spellToCast > eSpell::ACID_BREATH || chanceToCast < 1)
                continue;

            if (chanceToCast >= 100 || chanceToCast >= h3::H3Random::Rand(1, 100))
            {
                const eSpell spellId = WoG_GetStackSpellToCast(atkStack, target, spellToCast);

                if (spellId != eSpell::NONE)
                {
                    target->spellToApply = spellId;
                    THISCALL_3(BYTE, 0x468570, mgr, -1, 0);
                }
            }
        }
    }
}

// static int __stdcall DEBUG_Random(HiHook *h, const int low, const int high)
//  static int __stdcall BattleMgrProc_AIQuickRandom(HiHook *h, const int low, const int high)
// исправление активной стороны при контратаке
char __stdcall HiHook_00441b5d(HiHook *h, h3::H3CombatCreature *attacker, h3::H3CombatCreature *target, int direction)
{
    int currentStackIndex = h3::H3CombatManager::Get()->currentMonIndex;
    int currentStackSide = h3::H3CombatManager::Get()->currentMonSide;
    h3::H3CombatManager::Get()->currentMonSide = attacker->side;
    h3::H3CombatManager::Get()->currentMonIndex = attacker->sideIndex;

    char result = THISCALL_3(char, h->GetDefaultFunc(), attacker, target, direction);

    h3::H3CombatManager::Get()->currentMonSide = currentStackSide;
    h3::H3CombatManager::Get()->currentMonIndex = currentStackIndex;
    return result;
}

// убираем обновление панели ресурсов, если активный менеджер это менеджер города
void __stdcall Y_RedrawResources(HiHook *hook, DWORD a1, int a2, int a3)
{
    if (!(h3::IntAt(0x40144F) >= 660 && h3::H3ExecutiveMgr::Get()->active_mgr == h3::H3TownManager::Get()))
        THISCALL_3(void, hook->GetDefaultFunc(), a1, a2, a3);
}

bool IsHexOccupied(h3::H3CombatManager *mgr, int hex)
{
    // Bound 187 is invalid for the 187-element H3API arrays.
    if (hex <= 0 || hex >= 187)
        return true;
    auto *stack = mgr->squares[hex].GetCreature();
    return (stack && stack->numberAlive > 0) || THISCALL_2(BYTE, 0x4695F0, mgr, hex);
}
int AdjacentHex(h3::H3CombatManager *mgr, int hex, int direction)
{
    if (hex <= 0 || hex >= 187 || direction < 0 || direction >= 6)
        return -1;
    const auto *adj = reinterpret_cast<const INT16 *>(&mgr->adjacentSquares[hex]);
    int result = adj[direction];
    return IsHexOccupied(mgr, result) ? -1 : result;
}
// фикс: атакующий летающий стек НЕ пропускает ход при атаке заблоченного со всех сторон целевого стека
BYTE __stdcall AIMgr_Stack_SetHexes_WayToMoveLength(HiHook *hook, DWORD this_, h3::H3CombatCreature *stack, int side,
                                                    int gex_id, int isTactics, int speed, int a7)
{
    // результат по умолчанию: клетка не доступна
    BYTE result = FALSE;

    // если стек существует (защита от вылетов)
    if (stack && gex_id >= 0 && gex_id < 187)
    {
        // пишем сразу структуру менеджера битвы
        h3::H3CombatManager *bm = h3::H3CombatManager::Get();

        // проходим по гексам вокруг целевого гекса (gex_id)
        for (int i = 0; i < 6; i++)
        {
            // сначала исключаем случай, если атакующий уже стоит рядом с целью
            int stackGexID = reinterpret_cast<const INT16 *>(&bm->adjacentSquares[gex_id])[i];

            if (stackGexID == stack->position || stackGexID == stack->GetSecondSquare())
            {
                result = TRUE;
                break;
            }

            // теперь ищем пустой гекс вокруг цели
            int emptyGexID = AdjacentHex(bm, gex_id, i);

            // если в текущем цикле найден пустой доступный гекс вокруг цели
            if (emptyGexID != -1)
            {
                // клетка найдена
                result = TRUE;

                // проверяем на доступность широкому атакующему монстру
                if (stack->info.doubleWide)
                {
                    // проверяем справа и слева клетку на под атакующим монстром
                    if (stack->GetSecondSquare() == (emptyGexID + 1) || stack->GetSecondSquare() == (emptyGexID - 1))
                    {
                        result = TRUE;
                        break;
                    }

                    // ищем второй гекс: справа или слева от текущего
                    BYTE isNotEmptyGexID_Right = IsHexOccupied(bm, emptyGexID + 1);
                    BYTE isNotEmptyGexID_Left = IsHexOccupied(bm, emptyGexID - 1);

                    // если оба гекса заняты: убираем флаг найденного и ищем дальше в следующей итерации
                    if (isNotEmptyGexID_Right && isNotEmptyGexID_Left)
                        result = FALSE;
                }

                // если гекс(ы) найден(ы) - выход из цикла
                // если не найдены: ищем в следующей итерации
                if (result)
                    break;
            }
        }
    }

    // если клетка найдена: добавляем её в список
    if (result)
        return THISCALL_7(BYTE, hook->GetDefaultFunc(), this_, stack, side, gex_id, isTactics, speed, a7);
    else
        return FALSE;
}

// фикс: двуклеточные монстры в бою могут сделать 1 шаг назад
int __stdcall Y_FixBattle_StackStepBack(LoHook *, HookContext *c)
{
    // At 0x476020 the native POP EDI / POP ESI have already run.
    // EAX still holds the active stack from the self-hover comparison.
    // Replace only MOV EAX, 5; leave the complete native epilogue intact.
    h3::H3CombatCreature *stack = (h3::H3CombatCreature *)c->eax;
    const int targetHex = *(int *)(c->ebp + 8);
    const int flags = stack->info.flags;
    c->eax = 5;

    if ((flags & (1 | 0x200000)) != 1 || targetHex != stack->position || stack->activeSpellDuration[50] ||
        stack->GetStackSpeed() <= 0)
        return NO_EXEC_DEFAULT;

    // The native move needs one more free cell opposite the current rear hex.
    const int backHex = targetHex + (stack->secondHexOrientation ? -1 : 1);
    const int column = backHex % 17;
    if (backHex >= 0 && backHex < 187 && column > 0 && column < 16 &&
        !IsHexOccupied(h3::H3CombatManager::Get(), backHex))
        c->eax = (flags & 2) ? 2 : 1;

    return NO_EXEC_DEFAULT;
}

void Install(PatcherInstance *pi)
{
    if (Era::PluginExists && !Era::PluginExists("typhon"))
    {
        // фикс бага при получении хинта от Магических Святынь
        // ранее использовался массив с заклинаиями от артефактов 0x430 -> 0x3EA
        pi->WriteWord(0x40D97C, 0x3EA);
        auto *heroInfo = h3::H3HeroInfo::Get();
        heroInfo[eHero::ULAND].sskills[0].level = h3::eSecSkillLevel::BASIC;
        heroInfo[eHero::DEEMER].sskills[1].level = h3::eSecSkillLevel::BASIC;
    }
    // Делаем кнопку отмены у ученого, предлагающего втор.навык
    pi->WriteLoHook(0x4A4AFE, ScholarChoice);
    // решение вылета битвы, когда её инициируют в городе (например ERM или очарование Суккубов)
    pi->WriteHiHook(0x4B09D0, SPLICE_, EXTENDED_, THISCALL_, TownCombatManager);
    // Hook only MOV EAX, 5; keep the native POP EDI / POP ESI.
    pi->WriteLoHook(0x476020, Y_FixBattle_StackStepBack);
    pi->WriteHiHook(0x523FE6, CALL_, EXTENDED_, THISCALL_, AIMgr_Stack_SetHexes_WayToMoveLength);
    // Баг? _BattleStack_::MeleeAtack (контратака) - фикс сайда
    pi->WriteHiHook(0x441B5D, CALL_, EXTENDED_, THISCALL_, HiHook_00441b5d);
    // убираем обновление инфо-панели и панели ресурсов, если активный менеджер не менеджер карты приключений
    pi->WriteHiHook(0x403F00, SPLICE_, EXTENDED_, THISCALL_, Y_RedrawResources);
    // © daemon_n
    // переписываем логику колдовства заклинания после атаки на действительное колдовство после атаки
    pi->WriteLoHook(0x75C9A1, OnBeforeSorceressShot);
    pi->WriteLoHook(0x75CA66, OnBeforeHellSteedAttack);
    pi->WriteLoHook(0x75CAA6, OnBeforeCreatureAttack);
    for (DWORD address : {DWORD(0x43FA11), DWORD(0x43FE0B), DWORD(0x43FA7C), DWORD(0x4418A5)})
        pi->WriteHiHook(address, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay);
    // отключение сброса эффекта берсеркера, если он был получен лишь в процессе атаки, а не на начало хода
    // исправление имеет смысл для срабатывания берсерка при получении эффекта в качестве ответного удара/выстрела;
    skipMeleeBerserkReset = pi->WriteJmp(0x441C75, 0x441C7E);
    skipRangeBerserkReset = pi->WriteJmp(0x440017, 0x440020);
    if (skipMeleeBerserkReset)
        skipMeleeBerserkReset->Undo();
    if (skipRangeBerserkReset)
        skipRangeBerserkReset->Undo();
    pi->WriteHiHook(0x4419D0, SPLICE_, EXTENDED_, THISCALL_, BattleStack_PrepareMelee);
    pi->WriteHiHook(0x43FE80, SPLICE_, EXTENDED_, THISCALL_, BattleStack_PrepareShoot);
    // Делаем кнопку отмены в Хижине Ведьмы
    pi->WriteDword(0x4A7E64, 2);
    pi->WriteHiHook(0x4A7E8A, CALL_, EXTENDED_, THISCALL_, Y_SetCanselWitchHut);
    // фикс бага встречи одного и того-же героя: игрой в редких случаях перезатирается ecx
    // в дополнение: запрещаем встречу, если номера у героев одинаковые
    pi->WriteLoHook(0x4A2476, Y_Fix_HeroesInteract);
    // проверка взаимодействия героя в лодке с героем в другой лодке
    // фиксим: вносим проверку на воду под героем 2, вдруг он стоит на воде
    // например если остановился на MapEvent
    pi->WriteLoHook(0x4814C0, Y_Fix_HeroesOnWaterCheckInteract);
    // прописываем возврат регистров
    pi->WriteByte(0x4814DD, 0x90);
    pi->WriteByte(0x4814DE, 0x5F);
    pi->WriteByte(0x4814DF, 0x5E);
    // добавляем проверку на воду: mapItem->land == 8 (и убираем проверку на лодку)
    pi->WriteCodePatch(0x4806D9, "F743 04 08000000 752A 909090");
    // фикс бага при удалении объектов - затираются данные,
    // если жёлтая клетка другого объекта
    // стоит выше на 1 ед. по y-координате
    pi->WriteHiHook(0x4AA820, SPLICE_, EXTENDED_, THISCALL_, HiHook_004aa820);
    pi->WriteLoHook(0x4AA9B3, LoHook_004AA9B3);
    pi->WriteLoHook(0x4AA9BF, LoHook_004AA9BF);
    pi->WriteLoHook(0x4AA9EA, LoHook_004AA9BF);
    // фикс RNG: отвязываем генерацию случайных чисел от проигрывания анимации в битве
    pi->WriteHiHook(0x463606, CALL_, EXTENDED_, THISCALL_, BattleMgr_SetRandomSeed);
    const DWORD animationSites[] = {0x47847D, 0x4789DB, 0x478B4F, 0x478C59, 0x478D2B, 0x478E77, 0x478FEB,
                                    0x4794A6, 0x4798AC, 0x4961ED, 0x496246, 0x4EB256, 0x4EB3CC, 0x5A570C,
                                    0x5A5788, 0x5A61F2, 0x5A6262, 0x5A6283, 0x5A62AA};
    for (DWORD address : animationSites)
        pi->WriteHiHook(address, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AnimationRandom);
    pi->WriteHiHook(0x50B3DF, CALL_, DIRECT_, CDECL_, BattleMgrProc_AnimationPureRandom);
    for (DWORD address : {DWORD(0x421C82), DWORD(0x4647D0)})
        pi->WriteHiHook(address, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AIRandom);
    // исправление отображаемого def при разрушении зданий в городе без построенного форта
    pi->WriteLoHook(0x70BF78, TownDemolishion_BeforeBuildingRebuild);
    pi->WriteLoHook(0x70AA96, TownDemolishion_BeforeGraphicsChange);
    pi->WriteLoHook(0x415D4D, Y_AdvMgr_RedrawInfoPanel);
    // Фикс Бага воскрешения командиром существ со здоровьем <=50, а не макс 5-го уровня
    pi->WriteLoHook(0x5A881C, BattleStack_AtGettingResurrectionResistance);
    pi->WriteLoHook(0x41C879, js_Cast_AdventureMagic_BeforeCheckFlyPower);
    pi->WriteLoHook(0x5A065D, js_BattleMgr_CastSpell_BeforeSwitchCase);
    Era::RegisterHandler(OnGameLeave, "OnGameLeave");
}
} // namespace game_logic_remaining
} // namespace gbf
