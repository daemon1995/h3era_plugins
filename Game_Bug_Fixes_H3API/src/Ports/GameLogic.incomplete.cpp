#include "../../include/Plugin.h"
#include "../../include/PluginRng.h"
#include <algorithm>
#include <cstring>
#include <string>

namespace gbf { namespace gamelogic {

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
// by igrik /////////////////////////////////////////////////////////////////////////////////////////

int __stdcall Y_SetCanselWitchHut(HiHook *hook, h3::H3Hero *hero, int skill, BYTE skill_lvl)
{
    if (h3::H3WindowManager::Get()->result_dlg_item_id == DIID_CANCEL)
    {
        return 0;
    }

    return THISCALL_3(int, hook->GetDefaultFunc(), hero, skill, skill_lvl);
}

int __stdcall Y_SetCanselScholarlySS(LoHook *h, HookContext *c)
{
    int sskill = c->edi;
    h3::H3Hero *hero = (h3::H3Hero *)c->ebx;

    if (!h3::H3Messagebox::Choice(h3::H3TextTable::Get()[116][0], 2, 20, hero->secSkill[sskill] + 3 * sskill + 2))
    {
        if (hero->secSkill[sskill] == 1)
        {
            hero->secSkillCount -= 1;
            hero->secSkillPosition[sskill] = 0;
        }
        hero->secSkill[sskill] -= 1;
    }
    c->return_address = 0x4A4B86;
    return NO_EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// исправление созданий WoG'ом корявых пакованых координат
DWORD __cdecl Y_WoG_MixedPos_Fix(int x, int y, int z)
{
    DWORD xyz = b_pack_xyz(x, y, z);

    return xyz;
}

// исправление созданий WoG'ом корявых разпакованных координат
void __cdecl Y_WoG_UnMixedPos_Fix(DWORD x, DWORD y, DWORD z, DWORD xyz)
{
    *(DWORD *)x = b_unpack_x(xyz);
    *(DWORD *)y = b_unpack_y(xyz);
    *(DWORD *)z = b_unpack_z(xyz);

    return;
}

// фикс вылета: нет проверки на наличие стуктуры целевого стека
// но тут не хватает проверки на c->edi
int __stdcall Y_FixCrash_CastSpell_38(LoHook *h, HookContext *c)
{
    if (c->edi)
    {
        c->eax = *(int *)(c->edi + 0x38);
        c->ecx = *(int *)(c->ebx + 0x132C0);
        c->return_address = 0x5A1C20;
    }
    else
        c->return_address = 0x5A2368;
    return NO_EXEC_DEFAULT;
}

// фикс вылета: при удалении препятствия в битве нет проверки на наличие стуктуры препятствия
// (привет WoG и его стена огня у Огненных Лошадей)
int __stdcall Y_FixCrash_RemoveObstacle(LoHook *h, HookContext *c)
{
    // проверяем на пустую структуру боевого пропятствия
    // чтобы пропустить код обращения к ней и как следствие крит.краш.игры
    if (!c->ecx || !c->edi)
    {
        c->return_address = 0x466826;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

// фикс: атакующий летающий стек НЕ пропускает ход при атаке заблоченного со всех сторон целевого стека
BYTE __stdcall AIMgr_Stack_SetHexes_WayToMoveLength(HiHook *hook, DWORD this_, h3::H3CombatCreature *stack, int side,
                                                      int gex_id, int isTactics, int speed, int a7)
{
    // результат по умолчанию: клетка не доступна
    BYTE result = FALSE;

    // если стек существует (защита от вылетов)
    if (stack)
    {
        // пишем сразу структуру менеджера битвы
        h3::H3CombatManager *bm = h3::H3CombatManager::Get();

        // проходим по гексам вокруг целевого гекса (gex_id)
        for (int i = 0; i < 6; i++)
        {
            // сначала исключаем случай, если атакующий уже стоит рядом с целью
            int stackGexID = bm->adjacentSquares[gex_id].hexAdjacent[i];

            if (stackGexID == stack->position || stackGexID == stack->GetSecondSquare())
            {
                result = TRUE;
                break;
            }

            // теперь ищем пустой гекс вокруг цели
            int emptyGexID = bm->HexAround(gex_id, i);

            // если в текущем цикле найден пустой доступный гекс вокруг цели
            if (emptyGexID != -1)
            {
                // клетка найдена
                result = TRUE;

                // проверяем на доступность широкому атакующему монстру
                if (stack->info.flags & BCF_2HEX_WIDE)
                {
                    // проверяем справа и слева клетку на под атакующим монстром
                    if (stack->GetSecondSquare() == (emptyGexID + 1) || stack->GetSecondSquare() == (emptyGexID - 1))
                    {
                        result = TRUE;
                        break;
                    }

                    // ищем второй гекс: справа или слева от текущего
                    BYTE isNotEmptyGexID_Right = bm->IsHexOccupied(emptyGexID + 1);
                    BYTE isNotEmptyGexID_Left = bm->IsHexOccupied(emptyGexID - 1);

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
int __stdcall Y_FixBattle_StackStepBack(LoHook*, HookContext* c)
{
    // At 0x476020 the native POP EDI / POP ESI have already run.
    // EAX still holds the active stack from the self-hover comparison.
    // Replace only MOV EAX, 5; leave the complete native epilogue intact.
    h3::H3CombatCreature *stack = (h3::H3CombatCreature *)c->eax;
    const int targetHex = *(int *)(c->ebp + 8);
    const int flags = stack->info.flags;
    c->eax = 5;

    if ((flags & (BCF_2HEX_WIDE | BCF_CANT_MOVE)) != BCF_2HEX_WIDE ||
        targetHex != stack->position || stack->activeSpellDuration[50] || stack->GetStackSpeed() <= 0)
        return NO_EXEC_DEFAULT;

    // The native move needs one more free cell opposite the current rear hex.
    const int backHex = targetHex + (stack->orientation ? -1 : 1);
    const int column = backHex % 17;
    if (backHex >= 0 && backHex < 187 && column > 0 && column < 16 &&
        !h3::H3CombatManager::Get()->IsHexOccupied(backHex))
        c->eax = (flags & BCF_CAN_FLY) ? 2 : 1;

    return NO_EXEC_DEFAULT;
}

bool __cdecl WoG_OnMapRestartCrtraitsReset(const BOOL isWogMap)
{
    return 1;
}

// убираем клонов из показа в диалоге результатов битвы
int __stdcall Y_Dlg_BattleResults_IgnoreClones(LoHook*, HookContext* c)
{
    h3::H3CombatCreature *stack = (h3::H3CombatCreature *)(c->eax - 96);

    if (stack && stack->info.flags & BCF_CLONE)
    {
        c->return_address = 0x470958;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// © daemon_n
// Добавить мифрил игроку при вводе чит-кода на ресурсы
int __stdcall AdvMgr_ApplyResourceCheat(LoHook*, HookContext* c)
{
    IntAt(0x27F9A00 + (*reinterpret_cast<h3::H3Player**>(0x69CCFC))->id * 4) += 100;
    return EXEC_DEFAULT;
}
/**
 * Фикс бага встречи одного и того-же героя: игрой в редких случаях перезатирается ecx
 * в дополнение: запрещаем встречу, если номера у героев одинаковые
 */
int __stdcall Y_Fix_HeroesInteract(LoHook*, HookContext* c)
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

    h3::H3AdventureManager::Get()->HeroActive_DeMobilize();
    // после выполнения DeMobilize(), в (c->ebp +0xC) в ряде случаев портится указатель на номер героя
    // поэтому мы сначала сохранили номер героя, а щас восстанавливаем
    c->ecx = HeroRid;

    // пропускаем стандартный код игры (мы его выполнили сами)
    c->return_address = 0x4A2486;
    return NO_EXEC_DEFAULT;
}

/**
 * Добавляем проверку правого героя на воде (если он не в лодке)
 * иначе левый герой в лодке просто "наезжает" на правого героя (который не в лодке)
 * и получаем "баг раздвоения" героев
 */
int __stdcall Y_Fix_HeroesOnWaterCheckInteract(LoHook*, HookContext* c)
{
    const h3::H3MapItem *mapItem = reinterpret_cast<h3::H3MapItem *>(c->eax);
    const h3::H3Hero *heroRight = mapItem ? h3::H3Main::Get()->GetHero(mapItem->setup) : nullptr;
    c->eax = mapItem && ((heroRight && (heroRight->flags & 0x40000)) || mapItem->land == 8);
    c->return_address = 0x4814DD;
    return NO_EXEC_DEFAULT;
}
// Исправление копейщиков в лагерях беженцев (только для клетки с триггером).
int __stdcall LoHook_FixRefugeeCamp_dx(LoHook*, HookContext* c)
{
    // Клетка.
    h3::H3MapItem *item = reinterpret_cast<h3::H3MapItem *>(c->edi);

    // Если это загрузка карты или не лагерь беженцев - устанавливаем подтип.
    if (ByteAt(c->ebp + 0xC) || item->objectType != 78)
    {
        item->objectSubtype = LOWORD(c->edx);
    }

    return EXEC_DEFAULT;
}

// решение вылета в городе после битвы, когда её инициируют в городе (например ERM или очарование Суккубов)
void __stdcall Y_CallManager(HiHook *hook, h3::H3ExecutiveMgr *execMgr, h3::H3Manager *callMgr)
{
    if (execMgr->active_mgr == h3::H3TownManager::Get() && callMgr == h3::H3CombatManager::Get())
    {
        h3::H3Manager *mgr = h3::H3TownManager::Get();
        execMgr->RemoveManager(mgr);
        execMgr->AddManager(callMgr, 0);
        execMgr->StartManager(callMgr);
        execMgr->RemoveManager(callMgr);

        h3::H3TownManager::Get()->town = h3::H3Main::Get()nullptr /* replace with native GetTownByMapPoint */;
        execMgr->AddManager(mgr, 0);
        execMgr->active_mgr = mgr;
    }
    else
    {
        THISCALL_2(void, hook->GetDefaultFunc(), execMgr, callMgr);
    }
}

// убираем обновление панели ресурсов, если активный менеджер это менеджер города
void __stdcall Y_RedrawResources(HiHook *hook, DWORD a1, int a2, int a3)
{
    if (!(h3::IntAt(0x40144F) >= 660 && h3::H3ExecutiveMgr::Get()->active_mgr == h3::H3TownManager::Get()))
        THISCALL_3(void, hook->GetDefaultFunc(), a1, a2, a3);
}

// убираем обновление инфо-панели, если активный менеджер не менеджер карты приключений
int __stdcall Y_AdvMgr_RedrawInfoPanel(LoHook*, HookContext* c)
{
    if (!h3::H3ExecutiveMgr::Get()->active_mgr || h3::H3ExecutiveMgr::Get()->active_mgr == h3::H3AdventureManager::Get())
        return EXEC_DEFAULT;

    c->return_address = 0x415E5D;
    return NO_EXEC_DEFAULT;
}

// фикс бага при удалении объектов - не даём удалять разные типы объектов
int __stdcall Y_OnDeleteObjectOnMap(LoHook*, HookContext* c)
{
    h3::H3MapItem *currentMapItem = (h3::H3MapItem *)c->edx;
    h3::H3MapItem *deletingMapItem = *(h3::H3MapItem **)(c->ebp + 8);

    if (currentMapItem && deletingMapItem)
    {
        if (currentMapItem->objectType != 62 &&
            currentMapItem->GetRealType() != deletingMapItem->GetRealType())
        {
            c->return_address = 0x4AA984;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}
int globRecord = false;
int __stdcall HiHook_004aa820(HiHook *h, h3::H3AdventureManager *a2, h3::H3MapItem *a3, int XYZ, int record)
{
    const int previousRecord = globRecord;
    globRecord = record;
    const int result = THISCALL_4(int, h->GetDefaultFunc(), a2, a3, XYZ, record);
    globRecord = previousRecord;
    return result;
}

int __stdcall LoHook_004AA9B3(LoHook*, HookContext* c)
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
        auto &boats = h3::H3Main::Get()->Field<h3::H3Vector<h3::H3Boat>>(0x4E3B8);
        if (boats.Data && cell->setup >= 0 && cell->setup < boats.EndData - boats.Data)
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
        hero->Show(34, hero->id);
    return 0;
}

int __stdcall LoHook_004AA9BF(LoHook*, HookContext* c)
{
    return globRecord != 0;
}
// © SadnessPower
// Пожиратель душ ранее воскрешал стеки существ, уровень которых больше 4,
// если их здоровье было меньше 50
int __stdcall BattleStack_AtGettingResurrectionResistance(LoHook*, HookContext* c)
{
    // если кастует стек
    if (IntAt(c->ebp + 0x1C))
    {
        if (const auto activeStack = h3::H3CombatManager::Get()->activeStack)
        {
            // если кастует Пожиратель Душ
            if (activeStack->creature_id == -101 || activeStack->creature_id == -102)
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

// © daemon_n
// фикс бага расчёта защиты с возвратом 0 (чаще всего при атаке медведями существ с низкой защитой)
int __stdcall WoG_BattleStack_GetDefenceAgainst(LoHook*, HookContext* c)
{
    // поместить сразу в переменную возврата результат выполнения оригинальной ф-ции
    // вот если бы не Ghost Behemoth, проблема бы решалась в 2 счёта (байта, кек)

    IntAt(0x2832700) = c->eax;
    return EXEC_DEFAULT;
}
// © daemon_n
// Ошибка WoG/ERM -- помещение банка существ не типа объекта 16 не инициализировала его как банк существ
int __stdcall WoG_PlaceObject(LoHook*, HookContext* c)
{
    const int objectType = IntAt(c->ebp + 0x14);

    int creatureBankType = -1;
    switch (objectType)
    {
    case 24:
        creatureBankType = 8;
        break;
    case 25:
        creatureBankType = 10;
        break;
    case 84:
        creatureBankType = 9;
        break;
    case 85:
        creatureBankType = 7;
        break;
    default:
        break;
    }
    // если это банк существ
    if (creatureBankType != -1)
    {
        // помещаем подтип согласно типу
        IntAt(c->ebp + 0x18) = creatureBankType;
        // и прыгаем в тип 16
        c->return_address = 0x07141B6;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
// © daemon_n
// фикс SoD бага, который не отмечает посещённосто университа игроком
int __stdcall AdvMgr_EnterToUniversity(LoHook*, HookContext* c)
{
    const auto hero = *reinterpret_cast<h3::H3Hero **>(c->ebp + 0x8);
    if (hero && hero->owner_id >= 0 && hero->owner_id < 8)
    {
        THISCALL_2(void, 0x4FC620, c->edi, hero->owner_id);
    }
    return EXEC_DEFAULT;
}

// © JackSlater
// Фикс бага SoD - Невозможно колдовать более сильную версию заклинания "Полёт"
// Если герой уже колдовал его на этот ход
int __stdcall js_Cast_AdventureMagic_BeforeCheckFlyPower(LoHook*, HookContext* c)
{
    if (const auto *hero = reinterpret_cast<h3::H3Hero *>(c->esi))
    {
        // cast power is more than current fly power
        const auto &spell = h3::H3Spell::Get()[6];
        const int currentPower = hero->fly_cast_power;
        if (currentPower >= 0 && currentPower < 4 && c->edi >= 0 && c->edi < 4 &&
            spell.effect[c->edi] > spell.effect[currentPower])
        {
            c->return_address = 0x041C886;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}
// © JackSlater
// Фикс бага SoD - усилители заклинаний от героя не работали при касте существами
int __stdcall js_BattleMgr_CastSpell_BeforeSwitchCase(LoHook*, HookContext* c)
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

// © JackSlater
// Прыжок через ProcessMessagesForTime, если скрытая битва

int __stdcall LoHook_00441B25(LoHook*, HookContext* c)
{
    if (!h3::H3CombatManager::Get()->IsHiddenBattle())
    {
        return EXEC_DEFAULT;
    }

    c->return_address = 0x441B43;
    return NO_EXEC_DEFAULT;
}
// © JackSlater
// Прыжок через ProcessMessagesForTime, если скрытая битва
int __stdcall LoHook_00441BD6(LoHook*, HookContext* c)
{
    if (!h3::H3CombatManager::Get()->IsHiddenBattle())
    {
        return EXEC_DEFAULT;
    }

    c->return_address = 0x441BF5;
    return NO_EXEC_DEFAULT;
}
// Оковы войны действуют только в битве двух героев (ИИ).
int __stdcall LoHook_ShacklesRest_AI(LoHook *h, HookContext *c)
{
    // Менеджер боя.
    h3::H3CombatManager *b_mgr = (h3::H3CombatManager *)c->esi;

    // Оковы действуют только если есть
    if (!b_mgr->hero[0] || !b_mgr->hero[1])
    {
        c->return_address = 0x41E77C;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Оковы войны действуют только в битве двух героев (побег).
int __stdcall LoHook_ShacklesRest_Surrend(LoHook *h, HookContext *c)
{
    // Менеджер боя.
    h3::H3CombatManager *b_mgr = (h3::H3CombatManager *)c->esi;

    // Оковы действуют только если есть
    if (!b_mgr->hero[0] || !b_mgr->hero[1])
    {
        c->return_address = 0x478E1E;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Оковы войны действуют только в битве двух героев (сдача).
int __stdcall LoHook_ShacklesRest_GU(LoHook *h, HookContext *c)
{
    // Менеджер боя.
    h3::H3CombatManager *b_mgr = (h3::H3CombatManager *)c->esi;

    // Оковы действуют только если есть
    if (!b_mgr->hero[0] || !b_mgr->hero[1])
    {
        c->return_address = 0x478EE6;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}
// Баг: срабатывание огненного щита по трупу - перепрыгиваем AfterAttackAbilities и GetFireshieldDamage, если
// count_current <=0
int __stdcall LoHook_00441982(LoHook*, HookContext* c)
{
    if (reinterpret_cast<h3::H3CombatCreature *>(c->esi)->numberAlive > 0)
    {
        return EXEC_DEFAULT;
    }
    c->return_address = 0x4419A5;
    return NO_EXEC_DEFAULT;
}
// Баг: перед контратакой - проверка на count_current цели
int __stdcall LoHook_00441AFF(LoHook*, HookContext* c)
{

    if (reinterpret_cast<h3::H3CombatCreature *>(c->edi)->numberAlive > 0)
    {
        return EXEC_DEFAULT;
    }
    c->return_address = 0x441B85;
    return NO_EXEC_DEFAULT;
}

static thread_local PluginRNG g_pluginAIRng;

static void __stdcall BattleMgr_SetRandomSeed(HiHook *h, unsigned int seed)
{
    // исправляем значение настройки опции "каст заклинаний в автобитве", которая иногда может быть не равна 0 или 1,
    // что ломает логику проверки в игре
    IntAt(0x06987E8) = std::clamp(0, IntAt(0x06987E8), 1);

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
void __stdcall OnSetupBattleField(Era::TEvent*)
{

    static int comatCounter = 0;
    std::string message = "Battle " + std::to_string(comatCounter++) + " started";
    Era::WriteLog("GBFE", "New Combat Begins", message.c_str());
}

// static int __stdcall DEBUG_Random(HiHook *h, const int low, const int high)
//{
//     return g_pluginDebugRng.range(low, high);
// }
//  static int __stdcall BattleMgrProc_AIQuickRandom(HiHook *h, const int low, const int high)
//{
//      return g_pluginAIQucikRng.range(low, high);
//  }

// исправление активной стороны при контратаке
char __stdcall HiHook_00441b5d(HiHook *h, h3::H3CombatCreature *attacker, h3::H3CombatCreature *target, int direction)
{
    int currentStackIndex = h3::H3CombatManager::Get()->active_mgrMonIndex;
    int currentStackSide = h3::H3CombatManager::Get()->active_mgrMonSide;
    h3::H3CombatManager::Get()->active_mgrMonSide = attacker->side;
    h3::H3CombatManager::Get()->active_mgrMonIndex = attacker->sideIndex;

    char result = THISCALL_3(char, h->GetDefaultFunc(), attacker, target, direction);

    h3::H3CombatManager::Get()->active_mgrMonSide = currentStackSide;
    h3::H3CombatManager::Get()->active_mgrMonIndex = currentStackIndex;
    return result;
}
// © daemon_n
// исправление отображаемого def при разрушении зданий в городе без построенного форта
int wogTownDemolishionType = 0;

int __stdcall TownDemolishion_BeforeBuildingRebuild(LoHook*, HookContext* c)
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
int __stdcall TownDemolishion_BeforeGraphicsChange(LoHook*, HookContext* c)
{
    if (wogTownDemolishionType)
    {
        // 70AA96 calls a cdecl function with flags on the stack, not in ECX.
        IntAt(c->esp) = -1;
        wogTownDemolishionType = 0;
    }
    return EXEC_DEFAULT;
}
// исправление чита "построить все здания". На сбрасывается флаг введённого чита
void __stdcall OnGameLeave(Era::TEvent*)
{
    ByteAt(0x06AAAC4) = false;
}

// © daemon_n
// отключение сброса эффекта берсеркера, если он был получен лишь в процессе атаки, а не на начало хода
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
        : patch(resetPatch), previousPatchState(patch && patch->IsApplied()),
          previousAttacker(combatActionAttacker), previousSorceressTarget(afterSorceressAbilityTarget),
          previousHellSteedTarget(afterHellSteedAbilityTarget), previousAbilityTarget(afterAnyAttackAbilityTarget)
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

        if (targetMon->CanReceiveSpell(spellData.spellID, casterSide, 1, 1))
            totalChance += spellData.chance;
    }

    if (totalChance == 0)
        return eSpell::NONE;

    int randomValue = h3::H3Random::Rand() % totalChance;
    for (const auto &spellData : spellDataArray)
    {
        if (spellData.spellID == eSpell::NONE)
            break;

        if (!targetMon->CanReceiveSpell(spellData.spellID, casterSide, 1, 1))
            continue;

        if (randomValue < spellData.chance)
            return spellData.spellID;

        randomValue -= spellData.chance;
    }
    return eSpell::NONE;
}

int __stdcall OnBeforeSorceressShot(LoHook*, HookContext* c)
{
    afterSorceressAbilityTarget = reinterpret_cast<h3::H3CombatCreature *>(c->eax);
    c->return_address = 0x075CA76;
    return NO_EXEC_DEFAULT;
}
int __stdcall OnBeforeHellSteedAttack(LoHook*, HookContext* c)
{
    afterHellSteedAbilityTarget = reinterpret_cast<h3::H3CombatCreature *>(c->eax);
    c->return_address = 0x075CA76;
    return NO_EXEC_DEFAULT;
}
int __stdcall OnBeforeCreatureAttack(LoHook*, HookContext* c)
{
    afterAnyAttackAbilityTarget = reinterpret_cast<h3::H3CombatCreature *>(c->eax);
    c->return_address = 0x075CAFE;
    return NO_EXEC_DEFAULT;
}

eSpell WoG_GetStackSpellToCast(h3::H3CombatCreature *atkStack, h3::H3CombatCreature *targetMon, const eSpell fixedSpellToCast)
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
            mgr->PlayMagicAnimation(-1, showAttack);
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
            mgr->PlayMagicAnimation(-1, showAttack);
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
        byte Lvls[11];
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
                    mgr->PlayMagicAnimation(-1, 0);
                }
            }
        }
    }
}

int __stdcall AdvManager_KeyPress(LoHook*, HookContext* c)
{
    THISCALL_2(void, 0x47F0F0, h3::H3AdventureManager::Get(), 1);// H3AdventureManager::StopCursor
    return EXEC_DEFAULT;
}
// ##############################################################################################################################
// ##############################################################################################################################
// ##############################################################################################################################

void Install(PatcherInstance *pi)
{

    // фикс: двуклеточные монстры в бою могут сделать 1 шаг назад
    // Hook only MOV EAX, 5; keep the native POP EDI / POP ESI.
    pi->WriteLoHook(0x476020, Y_FixBattle_StackStepBack);

    // фикс: атакующий летающий стек НЕ пропускает ход при атаке заблоченного со всех сторон целевого стека
    pi->WriteHiHook(0x523FE6, CALL_, EXTENDED_, THISCALL_, AIMgr_Stack_SetHexes_WayToMoveLength);

    // исправление созданий WoG'ом корявых пакованых координат
    pi->WriteHiHook(0x711E7F, SPLICE_, DIRECT_, CDECL_, Y_WoG_MixedPos_Fix);
    pi->WriteHiHook(0x711F49, SPLICE_, DIRECT_, CDECL_, Y_WoG_UnMixedPos_Fix);

    // исправление бага с исчезновением стартового героя при переигрывании
    pi->WriteByte(0x5029C0, 0xEB);

    // Делаем кнопку отмены в Хижине Ведьмы
    pi->WriteDword(0x4A7E63 + 1, 2);
    pi->WriteHiHook(0x4A7E8A, CALL_, EXTENDED_, THISCALL_, Y_SetCanselWitchHut);

    // Делаем кнопку отмены у ученого, предлагающего втор.навык
    pi->WriteLoHook(0x4A4AFE, Y_SetCanselScholarlySS);

    // фикс выбора типа атаки при битве ИИ vs человек (человек не мог выбрать тип атаки)
    // суть в том, что была проверка на флаг V997, а должна быть V998
    pi->WriteByte(0x762601 + 3, 0xC5);

    // пропускаем показ всем игрокам захват Двеллинга 8-го уровня существ
    pi->WriteByte(0x70DB3B + 1, 0x37);

    // логическая ошибка SOD:
    // пропускаем показ обновления экрана для ИИ в телепортах:
    // - если мы не видим территорию
    // - если стоит опция "не показывать передвижения противника"
    pi->WriteByte(0x41DBE8 + 1, 0x5C);

    // радус открытия всей карты
    pi->WriteDword(0x4F4B57, 320); // [чит - меню(ориг = 180)]
    pi->WriteDword(0x4026FA, 360); // [чит wogeyeofsauron(ориг = 200)]
                                    // радус закрытия всей карты
    pi->WriteDword(0x402751, 360); //[чит wogeyeofsauron(ориг = 200)]
    // © daemon_n
    // Добавить мифрил игроку при вводе чит-кода на ресурсы
    pi->WriteLoHook(0x04027FE, AdvMgr_ApplyResourceCheat);
    // © daemon_n
    // при доступе к рынку в окне союзника без своих собственных (возможно через торговца артефактов)
    // курс делится на 0, что приводит к крашу при клику на ресурсах и артефактах
    pi->WriteDword(0x678344, 0x3dcccccd);          // float 0.1
    pi->WriteDword(0x678344 + 11 * 4, 0x3dcccccd); // float 0.1
    pi->WriteDword(0x678344 + 22 * 4, 0x3dcccccd); // float 0.1

    // фикс WoG'a

    pi->WriteHiHook(0x070561A, CALL_, DIRECT_, CDECL_, WoG_OnMapRestartCrtraitsReset);

    // © daemon_n
    // фикс ERM команды CB:M: при проверке/установке типа и количество существ значение ограничивалось 196 (Драколич)
    const int MAX_MON_ID = IntAt(0x4A1657);
    pi->WriteDword(0x073A847 + 3, MAX_MON_ID + 1); // увечиличить макс id при проверке на выход из границы
    pi->WriteDword(0x073A850 + 3, MAX_MON_ID);     // увеличить макс id при выходе за границы

    // © daemon_n
    // WoG баг, при котором, если защита отряда из оригинальной ф-ции = 0
    // и нет нападающего отряда при расчёте, игра возвращала мусорное значение;
    pi->WriteLoHook(0x075D42C, WoG_BattleStack_GetDefenceAgainst);

    // © daemon_n
    // Ошибка WoG/ERM -- помещение банка существ не типа объекта 16 не инициализировала его как банк существ
    pi->WriteLoHook(0x0713429, WoG_PlaceObject);

    // © daemon_n
    // фикс SoD бага, который не отмечает посещённосто университа игроком
    pi->WriteLoHook(0x04AA153, AdvMgr_EnterToUniversity);

    // убираем клонов из показа в диалоге результатов битвы
    pi->WriteLoHook(0x4708FC, Y_Dlg_BattleResults_IgnoreClones);

    // Темница(5) на поверхности генерируется на родном типе земли(6), а не на грязи(0)
    pi->WriteByte(0x464044, 0xEB);
    //pi->WriteDword(0x6408D8 + 5*4, 6);

    // фикс бага при удалении объектов - затираются данные,
    // если жёлтая клетка другого объекта
    // стоит выше на 1 ед. по y-координате
    // pi->WriteLoHook(0x4AA979, Y_OnDeleteObjectOnMap);
    pi->WriteHiHook(0x4aa820, SPLICE_, EXTENDED_, THISCALL_, HiHook_004aa820);
    pi->WriteLoHook(0x04AA9B3, LoHook_004AA9B3);
    pi->WriteLoHook(0x04AA9BF, LoHook_004AA9BF);
    pi->WriteLoHook(0x04AA9EA, LoHook_004AA9BF);

    // © JackSlater
    // Фикс бага SoD - Невозможно колдовать более сильную версию заклинания "Полёт"
    // Если герой уже колдовал его на этот ход
    pi->WriteLoHook(0x041C879, js_Cast_AdventureMagic_BeforeCheckFlyPower);

    // © JackSlater
    // Фикс бага SoD - усилители заклинаний от героя не работали при касте существами
    pi->WriteLoHook(0x05A065D, js_BattleMgr_CastSpell_BeforeSwitchCase);

    // © JackSlater
    // Исправляем баг SoD: сброс посещённости сирен после боя.
    pi->WriteCodePatch(0x4DA8FC, "%n", 24); // 24 nop

    // © SadnessPower
    // Фикс Бага воскрешения командиром существ со здоровьем <=50, а не макс 5-го уровня
    pi->WriteLoHook(0x05A881C, BattleStack_AtGettingResurrectionResistance);

    // © JackSlater
    // Прыжок через ProcessMessagesForTime, если скрытая битва
    pi->WriteLoHook(0x441B25, LoHook_00441B25);
    pi->WriteLoHook(0x441BD6, LoHook_00441BD6); // return address ->0x441BF5

    // Оковы войны действуют только в битве двух героев (ИИ).
    pi->WriteLoHook(0x41E74A, LoHook_ShacklesRest_AI);
    // Оковы войны действуют только в битве двух героев (побег).
    pi->WriteLoHook(0x478DA2, LoHook_ShacklesRest_Surrend);
    // Оковы войны действуют только в битве двух героев (сдача).
    pi->WriteLoHook(0x478EBC, LoHook_ShacklesRest_GU);

    // Баг: срабатывание огненного щита по трупу - перепрыгиваем AfterAttackAbilities и GetFireshieldDamage, если
    // count_current <=0
    pi->WriteLoHook(0x441982, LoHook_00441982);
    // Баг: перед контратакой - проверка на count_current цели
    pi->WriteLoHook(0x441AFF, LoHook_00441AFF);

    // © daemon_n
    // фикс RNG: отвязываем генерацию случайных чисел от проигрывания анимации в битве

    pi->WriteHiHook(0x0463606, CALL_, EXTENDED_, THISCALL_, BattleMgr_SetRandomSeed);

    DWORD lowHighDrawAnimationRandomFunctionAddresses[] = {
        0x0047847D, // move and attack animation
        0x004789DB, // cast spell animation
        0x00478B4F, // walk animation
        0x00478C59, // shoot animation
        0x00478D2B, // monster cast spell animation
        0x00478E77, // retreat animation
        0x00478FEB, // surrender animation
        0x004794A6, // wall attack animation
        0x004798AC, // battle begin animation

        0x004961ED, // play wait animation (safe random)
        0x00496246, // play wait animation (safe random)

        0x004EB256, // creature info dlg monster def animation
        0x004EB3CC, // creature info dlg machine def animation

        0x005A570C, // ray attack animation
        0x005A5788, // ray attack animation
        0x005A61F2, // ray attack animation
        0x005A6262, // ray attack animation
        0x005A6283, // ray attack animation
        0x005A62AA, // ray attack animation

    };
    for (DWORD addr : lowHighDrawAnimationRandomFunctionAddresses)
    {
        pi->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AnimationRandom);
    }
    DWORD pureRandomFunctionAddresses[] = {0x0050B3DF}; // wait animation
    for (DWORD addr : pureRandomFunctionAddresses)
    {
        pi->WriteHiHook(addr, CALL_, DIRECT_, CDECL_, BattleMgrProc_AnimationPureRandom);
    }
    // DWORD lowHighAIQuickRandomFunctionAddresses[] = {0x0042723C, 0x00427260}; // AI_Calc_FastBattle_Finishing
    // for (DWORD addr : lowHighAIQuickRandomFunctionAddresses)
    //{
    //     pi->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AIQuickRandom);
    // }
    // Random AI tie-break and the AI-only extra bad-morale roll.
    // Luck rolls and the primary morale rolls stay on the game RNG.
    DWORD lowHighAIRandomFunctionAddresses[] = {0x00421C82, 0x004647D0};
    for (DWORD addr : lowHighAIRandomFunctionAddresses)
    {
        pi->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AIRandom);
    }

    // Era::RegisterHandler(OnSetupBattleField, "OnSetupBattleField");

    // DWORD lowHighDebugRandomFunctionAddresses[] = {/*0x00443024*/,
    //                                                /* 0x00442FE9,*/ /* 0x004645B3, 0x004647A7, 0x04647D0,*/
    //                                                                 /* 0x00443F00, 0x00443F29, 0x00420F18,*/
    //                                                /*0x005A2100*/};
    // for (DWORD addr : lowHighDebugRandomFunctionAddresses)
    //{
    //     // pi->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, DEBUG_Random);
    // }
    // © daemon_n


    if (!TIPHON)
    {

        // © JackSlater
        // фикс бага при получении хинта от Магических Святынь
        // ранее использовался массив с заклинаиями от артефактов 0x430 -> 0x3EA

        pi->WriteWord(0x40D979 + 3, 0x3EA);
        // Фикс Уланда - герой имеет продвинутую мудрость на старте
        h3::H3HeroInfo::Get()[100].second_skill_1_lvl = 1;
        // Фикс Димера - герой имеет продвинутую разведку на старте
        h3::H3HeroInfo::Get()[33].second_skill_2_lvl = 1;
    }

    // © daemon_n
    // исправление отображаемого def при разрушении зданий в городе без построенного форта
    pi->WriteLoHook(0x70BF78, TownDemolishion_BeforeBuildingRebuild);
    pi->WriteLoHook(0x070AA96, TownDemolishion_BeforeGraphicsChange);

    // Баг? h3::H3CombatCreature::MeleeAtack (контратака) - фикс сайда
    pi->WriteHiHook(0x441b5d, CALL_, EXTENDED_, THISCALL_, HiHook_00441b5d);
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////// Фиксы раздвоения героя //////////////////////////

    // фикс бага встречи одного и того-же героя: игрой в редких случаях перезатирается ecx
    // в дополнение: запрещаем встречу, если номера у героев одинаковые
    pi->WriteLoHook(0x4A2476, Y_Fix_HeroesInteract);

    // проверка взаимодействия героя в лодке с героем в другой лодке
    // фиксим: вносим проверку на воду под героем 2, вдруг он стоит на воде
    // например если остановился на MapEvent
    pi->WriteLoHook(0x4814C0, Y_Fix_HeroesOnWaterCheckInteract);
    // прописываем возврат регистров
    pi->WriteByte(0x4814DD + 0, 0x90); // nop
    pi->WriteByte(0x4814DD + 1, 0x5F); // pop EDI
    pi->WriteByte(0x4814DD + 2, 0x5E); // pop ESI

    // добавляем проверку на воду: mapItem->land == 8 (и убираем проверку на лодку)
    pi->WriteCodePatch(0x4806D9, "F743 04 08000000 752A 909090");

    // © daemon_n
    // фикс: посещение мифрила обновляет экран
    pi->WriteCodePatch(0x07060AC, "%n", 7); // удалить добавление мифрила костылём
    pi->WriteJmp(0x0705F42, 0x0705F7D);     // пропустить изменение объекта на карте на костыльное решение

    // Исправление копейщиков в лагерях беженцев (только для клетки с триггером).
    pi->WriteCodePatch(0x505E15, "%n", 4);
    pi->WriteLoHook(0x505E15, LoHook_FixRefugeeCamp_dx);

    // Теперь в городах игрока при наличии форта всегда будет отстроено 2 уровня существ.
    pi->WriteCodePatch(0x5C1051, "%n", 20); // 20 nop
    // Убираем случайность количества стеков в начальной армии.
    pi->WriteCodePatch(0x4C948F, "%n", 5);  // 5 nop
    pi->WriteCodePatch(0x4C9497, "%n", 10); // 10 nop
    pi->WriteCodePatch(0x4C950F, "%n", 20); // 20 nop

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////// Фиксы крит.крашей игры //////////////////////////

    // фикс вылета: нет проверки на на наличие стуктуры целевого стека
    // но тут не хватает проверки на c->edi
    pi->WriteLoHook(0x5A1C17, Y_FixCrash_CastSpell_38);

    // фикс вылета: при удалении препятствия в битве, когда его стуктура таблицы равна нуля
    // (привет WoG и его стена огня у Огненных Лошадей)
    pi->WriteLoHook(0x46681B, Y_FixCrash_RemoveObstacle);

    // решение вылета битвы, когда её инициируют в городе (например ERM или очарование Суккубов)
    pi->WriteHiHook(0x4B09D0, SPLICE_, EXTENDED_, THISCALL_, Y_CallManager);

    // убираем обновление инфо-панели и панели ресурсов, если активный менеджер не менеджер карты приключений
    pi->WriteHiHook(0x403F00, SPLICE_, EXTENDED_, THISCALL_, Y_RedrawResources);
    pi->WriteLoHook(0x415D4D, Y_AdvMgr_RedrawInfoPanel);

    //// ЧИТ-Меню ////
    // Увеличиваем кол-во заклинаний 999 -> 9999
    pi->WriteWord(0x4F508F + 4, 9999);
    pi->WriteWord(0x4F50DE + 4, 9999);
    pi->WriteWord(0x4F511A + 4, 9999);
    // Увеличиваем кол-во существ 5 -> 100
    pi->WriteByte(0x4F4ED2 + 1, 100);

    // © daemon_n
    // исправление чита "построить все здания". На сбрасывается флаг введённого чита
    Era::RegisterHandler(OnGameLeave, "OnGameLeave");
    // © daemon_n
    // отключение сброса эффекта берсеркера, если он был получен лишь в процессе атаки, а не на начало хода
    // исправление имеет смысл для срабатывания берсерка при получении эффекта в качестве ответного удара/выстрела;
    skipMeleeBerserkReset = pi->WriteJmp(0x0441C75, 0x0441C7E);
    skipRangeBerserkReset = pi->WriteJmp(0x0440017, 0x0440020);
    // WriteJmp applies immediately; the skips belong only to an active attack.
    if (skipMeleeBerserkReset)
        skipMeleeBerserkReset->Undo();
    if (skipRangeBerserkReset)
        skipRangeBerserkReset->Undo();
    pi->WriteHiHook(0x4419D0, SPLICE_, EXTENDED_, THISCALL_, BattleStack_PrepareMelee);
    pi->WriteHiHook(0x43FE80, SPLICE_, EXTENDED_, THISCALL_, BattleStack_PrepareShoot);

    // © daemon_n
    // переписываем логику колдовства заклинания после атаки на действительное колдовство после атаки
    pi->WriteLoHook(0x075C9A1, OnBeforeSorceressShot);
    pi->WriteLoHook(0x075CA66, OnBeforeHellSteedAttack);
    pi->WriteLoHook(0x075CAA6, OnBeforeCreatureAttack);

    pi->WriteHiHook(0x43FA11, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // magog shooting
    pi->WriteHiHook(0x43FE0B, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // lich shooting
    pi->WriteHiHook(0x43FA7C, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // native shooting
    pi->WriteHiHook(0x4418A5, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // melee attack
    // исправляем хук "OnAfterShooting", чтобы работал для всех типов атак, а не только для стрельбы
    // заодно специальные стрелки получают звук атаки;
    // pi->WriteJmp(0x43FA16, 0x43FAD9); // magog shooting
    // pi->WriteJmp(0x43FE10, 0x43FAD9); // lich shooting IMPLEMENT LATER W/ LoHook- It has some referes to the
    // replaceable address;



    pi->WriteCodePatch(0x408ff7, "%n", 9);

    pi->WriteLoHook(0x408ff2, AdvManager_KeyPress);


}

void InstallGameLogic(PatcherInstance* pi) { gamelogic::Install(pi); }
} }
