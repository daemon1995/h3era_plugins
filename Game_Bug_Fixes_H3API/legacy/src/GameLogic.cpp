#include "PluginRng.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
// by igrik /////////////////////////////////////////////////////////////////////////////////////////

int __stdcall Y_SetCanselWitchHut(HiHook *hook, _Hero_ *hero, _int_ skill, _byte_ skill_lvl)
{
    if (o_WndMgr->result_dlg_item_id == DIID_CANCEL)
    {
        return 0;
    }

    return CALL_3(int, __thiscall, hook->GetDefaultFunc(), hero, skill, skill_lvl);
}

int __stdcall Y_SetCanselScholarlySS(LoHook *h, HookContext *c)
{
    int sskill = c->edi;
    _Hero_ *hero = (_Hero_ *)c->ebx;

    if (!b_MsgBoxD(o_ADVEVENT_TXT->GetString(116), 2, 20, hero->second_skill[sskill] + 3 * sskill + 2))
    {
        if (hero->second_skill[sskill] == 1)
        {
            hero->second_skill_count -= 1;
            hero->second_skill_show[sskill] = 0;
        }
        hero->second_skill[sskill] -= 1;
    }
    c->return_address = 0x4A4B86;
    return NO_EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// исправление созданий WoG'ом корявых пакованых координат
_dword_ __cdecl Y_WoG_MixedPos_Fix(int x, int y, int z)
{
    _dword_ xyz = b_pack_xyz(x, y, z);

    return xyz;
}

// исправление созданий WoG'ом корявых разпакованных координат
void __cdecl Y_WoG_UnMixedPos_Fix(_dword_ x, _dword_ y, _dword_ z, _dword_ xyz)
{
    *(_dword_ *)x = b_unpack_x(xyz);
    *(_dword_ *)y = b_unpack_y(xyz);
    *(_dword_ *)z = b_unpack_z(xyz);

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
_byte_ __stdcall AIMgr_Stack_SetHexes_WayToMoveLength(HiHook *hook, _dword_ this_, _BattleStack_ *stack, _int_ side,
                                                      _int_ gex_id, _int_ isTactics, _int_ speed, _int_ a7)
{
    // результат по умолчанию: клетка не доступна
    _byte_ result = FALSE;

    // если стек существует (защита от вылетов)
    if (stack)
    {
        // пишем сразу структуру менеджера битвы
        _BattleMgr_ *bm = o_BattleMgr;

        // проходим по гексам вокруг целевого гекса (gex_id)
        for (int i = 0; i < 6; i++)
        {
            // сначала исключаем случай, если атакующий уже стоит рядом с целью
            _int_ stackGexID = bm->adjacentSquares[gex_id].hexAdjacent[i];

            if (stackGexID == stack->hex_ix || stackGexID == stack->GetSecondGexID())
            {
                result = TRUE;
                break;
            }

            // теперь ищем пустой гекс вокруг цели
            _int_ emptyGexID = bm->GetEmptySquareAroundThis(gex_id, i);

            // если в текущем цикле найден пустой доступный гекс вокруг цели
            if (emptyGexID != -1)
            {
                // клетка найдена
                result = TRUE;

                // проверяем на доступность широкому атакующему монстру
                if (stack->creature.flags & BCF_2HEX_WIDE)
                {
                    // проверяем справа и слева клетку на под атакующим монстром
                    if (stack->GetSecondGexID() == (emptyGexID + 1) || stack->GetSecondGexID() == (emptyGexID - 1))
                    {
                        result = TRUE;
                        break;
                    }

                    // ищем второй гекс: справа или слева от текущего
                    _byte_ isNotEmptyGexID_Right = bm->IsGexNotFree(emptyGexID + 1);
                    _byte_ isNotEmptyGexID_Left = bm->IsGexNotFree(emptyGexID - 1);

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
        return CALL_7(_byte_, __thiscall, hook->GetDefaultFunc(), this_, stack, side, gex_id, isTactics, speed, a7);
    else
        return FALSE;
}

// фикс: двуклеточные монстры в бою могут сделать 1 шаг назад
_LHF_(Y_FixBattle_StackStepBack)
{
    // At 0x476020 the native POP EDI / POP ESI have already run.
    // EAX still holds the active stack from the self-hover comparison.
    // Replace only MOV EAX, 5; leave the complete native epilogue intact.
    _BattleStack_ *stack = (_BattleStack_ *)c->eax;
    const int targetHex = *(int *)(c->ebp + 8);
    const int flags = stack->creature.flags;
    c->eax = 5;

    if ((flags & (BCF_2HEX_WIDE | BCF_CANT_MOVE)) != BCF_2HEX_WIDE ||
        targetHex != stack->hex_ix || stack->active_spell_duration[SPL_BIND] || stack->GetSpeed() <= 0)
        return NO_EXEC_DEFAULT;

    // The native move needs one more free cell opposite the current rear hex.
    const int backHex = targetHex + (stack->orientation ? -1 : 1);
    const int column = backHex % 17;
    if (backHex >= 0 && backHex < 187 && column > 0 && column < 16 &&
        !o_BattleMgr->IsGexNotFree(backHex))
        c->eax = (flags & BCF_CAN_FLY) ? 2 : 1;

    return NO_EXEC_DEFAULT;
}

bool __cdecl WoG_OnMapRestartCrtraitsReset(const BOOL isWogMap)
{
    return 1;
}

// убираем клонов из показа в диалоге результатов битвы
_LHF_(Y_Dlg_BattleResults_IgnoreClones)
{
    _BattleStack_ *stack = (_BattleStack_ *)(c->eax - 96);

    if (stack && stack->creature.flags & BCF_CLONE)
    {
        c->return_address = 0x470958;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// © daemon_n
// Добавить мифрил игроку при вводе чит-кода на ресурсы
_LHF_(AdvMgr_ApplyResourceCheat)
{
    IntAt(0x27F9A00 + o_ActivePlayer->id * 4) += 100;
    return EXEC_DEFAULT;
}
/**
 * Фикс бага встречи одного и того-же героя: игрой в редких случаях перезатирается ecx
 * в дополнение: запрещаем встречу, если номера у героев одинаковые
 */
_LHF_(Y_Fix_HeroesInteract)
{
    // восстанавливаем затёртую и важную команду игры
    c->ebx = c->ecx;
    // сохраняем номер героя, чтобы потом восстановить
    _int_ HeroRid = IntAt(DwordAt(c->ebp + 0xC));

    // получаем структуру героя правой стороны
    _Hero_ *HeroL = *(_Hero_ **)(c->ebp + 0x8);

    if (!HeroL || HeroRid < 0 || HeroRid > 255 || HeroL->id == HeroRid)
    {
        // запрещаем вызов обмена героями
        c->return_address = 0x4A251F;
        return NO_EXEC_DEFAULT;
    }

    o_AdvMgr->HeroActive_DeMobilize();
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
_LHF_(Y_Fix_HeroesOnWaterCheckInteract)
{
    const _MapItem_ *mapItem = reinterpret_cast<_MapItem_ *>(c->eax);
    const _Hero_ *heroRight = mapItem ? o_GameMgr->GetHero(mapItem->setup) : nullptr;
    c->eax = mapItem && ((heroRight && (heroRight->temp_mod_flags & 0x40000)) || mapItem->land == 8);
    c->return_address = 0x4814DD;
    return NO_EXEC_DEFAULT;
}
// Исправление копейщиков в лагерях беженцев (только для клетки с триггером).
_LHF_(LoHook_FixRefugeeCamp_dx)
{
    // Клетка.
    _MapItem_ *item = reinterpret_cast<_MapItem_ *>(c->edi);

    // Если это загрузка карты или не лагерь беженцев - устанавливаем подтип.
    if (ByteAt(c->ebp + 0xC) || item->object_type != 78)
    {
        item->os_type = LOWORD(c->edx);
    }

    return EXEC_DEFAULT;
}

// решение вылета в городе после битвы, когда её инициируют в городе (например ERM или очарование Суккубов)
void __stdcall Y_CallManager(HiHook *hook, _ExecMgr_ *execMgr, _Manager_ *callMgr)
{
    if (execMgr->current == &o_TownMgr->mgr && callMgr == &o_BattleMgr->mgr)
    {
        _Manager_ *mgr = &o_TownMgr->mgr;
        execMgr->RemoveManager(mgr);
        execMgr->AddManager(callMgr);
        execMgr->RunManager();
        execMgr->RemoveManager(callMgr);

        o_TownMgr->town = o_GameMgr->GetTownByMapPoint(o_BattleMgr->mapPoint);
        execMgr->AddManager(mgr);
        execMgr->current = mgr;
    }
    else
    {
        CALL_2(void, __thiscall, hook->GetDefaultFunc(), execMgr, callMgr);
    }
}

// убираем обновление панели ресурсов, если активный менеджер это менеджер города
void __stdcall Y_RedrawResources(HiHook *hook, _dword_ a1, _int_ a2, _int_ a3)
{
    if (!(o_HD_Y >= 660 && o_ExecMgr->current == &o_TownMgr->mgr))
        CALL_3(void, __thiscall, hook->GetDefaultFunc(), a1, a2, a3);
}

// убираем обновление инфо-панели, если активный менеджер не менеджер карты приключений
_LHF_(Y_AdvMgr_RedrawInfoPanel)
{
    if (!o_ExecMgr->current || o_ExecMgr->current == &o_AdvMgr->mgr)
        return EXEC_DEFAULT;

    c->return_address = 0x415E5D;
    return NO_EXEC_DEFAULT;
}

// фикс бага при удалении объектов - не даём удалять разные типы объектов
_LHF_(Y_OnDeleteObjectOnMap)
{
    _MapItem_ *currentMapItem = (_MapItem_ *)c->edx;
    _MapItem_ *deletingMapItem = *(_MapItem_ **)(c->ebp + 8);

    if (currentMapItem && deletingMapItem)
    {
        if (currentMapItem->object_type != 62 &&
            currentMapItem->GetReal__object_type() != deletingMapItem->GetReal__object_type())
        {
            c->return_address = 0x4AA984;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}
int globRecord = false;
int __stdcall HiHook_004aa820(HiHook *h, _AdvMgr_ *a2, _MapItem_ *a3, int XYZ, int record)
{
    const int previousRecord = globRecord;
    globRecord = record;
    const int result = CALL_4(int, __thiscall, h->GetDefaultFunc(), a2, a3, XYZ, record);
    globRecord = previousRecord;
    return result;
}

_LHF_(LoHook_004AA9B3)
{

    NOALIGN struct _Boat_
    {
        INT16 x;
        /** @brief [02]*/
        INT16 y;
        /** @brief [04]*/
        INT16 z;
        /** @brief [06]*/
        INT8 visible;
        /** @brief [07] no clue how to get this offset without align-1, may be substructure*/
        _MapItem_ *item;
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
    _MapItem_ *cell; // esi
    _Hero_ *hero;    // ebx
    _Boat_ *v4;      // edi

    cell = (_MapItem_ *)c->edx;
    hero = 0;
    v4 = 0;
    if (cell->object_type == 34)
    {
        hero = o_GameMgr->GetHero(cell->setup);
        if (hero)
            hero->Hide();
    }
    if (cell->object_type == 8)
    {
        auto &boats = o_GameMgr->Field<_List_<_Boat_>>(0x4E3B8);
        if (boats.Data && cell->setup >= 0 && cell->setup < boats.EndData - boats.Data)
        {
            v4 = &boats[cell->setup];
            // Like heroes, boats must be hidden before clearing the object below.
            // Show is a no-op while visible and cannot restore a cleared setup.
            CALL_1(void, __thiscall, 0x4D7950, v4);
        }
    }
    cell->setup = -1;
    if (v4)
        CALL_3(void, __thiscall, 0x4D7840, v4, 8, v4->index);
    if (hero)
        hero->Show(34, hero->id);
    return 0;
}

_LHF_(LoHook_004AA9BF)
{
    return globRecord != 0;
}
// © SadnessPower
// Пожиратель душ ранее воскрешал стеки существ, уровень которых больше 4,
// если их здоровье было меньше 50
_LHF_(BattleStack_AtGettingResurrectionResistance)
{
    // если кастует стек
    if (IntAt(c->ebp + 0x1C))
    {
        if (const auto activeStack = o_BattleMgr->activeStack)
        {
            // если кастует Пожиратель Душ
            if (activeStack->creature_id == CID_SOUL_EATER_A || activeStack->creature_id == CID_SOUL_EATER_D)
            {
                if (reinterpret_cast<_BattleStack_ *>(c->edi)->creature.level > 4)
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
_LHF_(WoG_BattleStack_GetDefenceAgainst)
{
    // поместить сразу в переменную возврата результат выполнения оригинальной ф-ции
    // вот если бы не Ghost Behemoth, проблема бы решалась в 2 счёта (байта, кек)

    IntAt(0x2832700) = c->eax;
    return EXEC_DEFAULT;
}
// © daemon_n
// Ошибка WoG/ERM -- помещение банка существ не типа объекта 16 не инициализировала его как банк существ
_LHF_(WoG_PlaceObject)
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
_LHF_(AdvMgr_EnterToUniversity)
{
    const auto hero = *reinterpret_cast<_Hero_ **>(c->ebp + 0x8);
    if (hero && hero->owner_id >= 0 && hero->owner_id < 8)
    {
        CALL_2(void, __thiscall, 0x4FC620, c->edi, hero->owner_id);
    }
    return EXEC_DEFAULT;
}

// © JackSlater
// Фикс бага SoD - Невозможно колдовать более сильную версию заклинания "Полёт"
// Если герой уже колдовал его на этот ход
_LHF_(js_Cast_AdventureMagic_BeforeCheckFlyPower)
{
    if (const auto *hero = reinterpret_cast<_Hero_ *>(c->esi))
    {
        // cast power is more than current fly power
        const auto &spell = o_Spell[SPL_FLY];
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
_LHF_(js_BattleMgr_CastSpell_BeforeSwitchCase)
{
    // if monster casts spell and it affects enemy stack
    if (IntAt(c->ebp + 0x10) == 1 && o_Spell[IntAt(c->ebp + 0x8)].type == -1)
    {
        // set active hero from function return
        *reinterpret_cast<_Hero_ **>(c->ebp - 0x14) =
            CALL_1(_Hero_ *, __thiscall, 0x04423B0, o_BattleMgr->GetCurrentStack());
    }
    return EXEC_DEFAULT;
}

// © JackSlater
// Прыжок через ProcessMessagesForTime, если скрытая битва

_LHF_(LoHook_00441B25)
{
    if (!o_BattleMgr->IsHiddenBattle())
    {
        return EXEC_DEFAULT;
    }

    c->return_address = 0x441B43;
    return NO_EXEC_DEFAULT;
}
// © JackSlater
// Прыжок через ProcessMessagesForTime, если скрытая битва
_LHF_(LoHook_00441BD6)
{
    if (!o_BattleMgr->IsHiddenBattle())
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
    _BattleMgr_ *b_mgr = (_BattleMgr_ *)c->esi;

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
    _BattleMgr_ *b_mgr = (_BattleMgr_ *)c->esi;

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
    _BattleMgr_ *b_mgr = (_BattleMgr_ *)c->esi;

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
_LHF_(LoHook_00441982)
{
    if (reinterpret_cast<_BattleStack_ *>(c->esi)->count_current > 0)
    {
        return EXEC_DEFAULT;
    }
    c->return_address = 0x4419A5;
    return NO_EXEC_DEFAULT;
}
// Баг: перед контратакой - проверка на count_current цели
_LHF_(LoHook_00441AFF)
{

    if (reinterpret_cast<_BattleStack_ *>(c->edi)->count_current > 0)
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
    IntAt(0x06987E8) = Clamp(0, IntAt(0x06987E8), 1);

    CALL_1(void, __thiscall, h->GetDefaultFunc(), seed);
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
_ERH_(OnSetupBattleField)
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
char __stdcall HiHook_00441b5d(HiHook *h, _BattleStack_ *attacker, _BattleStack_ *target, int direction)
{
    int currentStackIndex = o_BattleMgr->currentStackIndex;
    int currentStackSide = o_BattleMgr->currentStackSide;
    o_BattleMgr->currentStackSide = attacker->side;
    o_BattleMgr->currentStackIndex = attacker->index_on_side;

    char result = CALL_3(char, __thiscall, h->GetDefaultFunc(), attacker, target, direction);

    o_BattleMgr->currentStackSide = currentStackSide;
    o_BattleMgr->currentStackIndex = currentStackIndex;
    return result;
}
// © daemon_n
// исправление отображаемого def при разрушении зданий в городе без построенного форта
int wogTownDemolishionType = 0;

_LHF_(TownDemolishion_BeforeBuildingRebuild)
{
    _Town_ *town = o_TownMgr->town;
    if (town && !town->IsBuildingBuilt(7, 1))
    {
        wogTownDemolishionType = 1;
        CALL_1(void, __cdecl, 0x070A999, -1); // меняем индекс на массив с префектурами (хотя этот код и работаает, но
                                              // игра позднее вызывает эту же ф-цию и перезатирает результат)
    }

    return EXEC_DEFAULT;
}
_LHF_(TownDemolishion_BeforeGraphicsChange)
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
_ERH_(OnGameLeave)
{
    ByteAt(0x06AAAC4) = false;
}

// © daemon_n
// отключение сброса эффекта берсеркера, если он был получен лишь в процессе атаки, а не на начало хода
Patch *skipMeleeBerserkReset = nullptr;
Patch *skipRangeBerserkReset = nullptr;
_BattleStack_ *combatActionAttacker = nullptr;
_BattleStack_ *afterSorceressAbilityTarget = nullptr;
_BattleStack_ *afterHellSteedAbilityTarget = nullptr;
_BattleStack_ *afterAnyAttackAbilityTarget = nullptr;

// Preserve the attacker context if another hook reenters PrepareMelee/PrepareShoot.
struct AttackContextScope
{
    Patch *patch;
    bool previousPatchState;
    _BattleStack_ *previousAttacker;
    _BattleStack_ *previousSorceressTarget;
    _BattleStack_ *previousHellSteedTarget;
    _BattleStack_ *previousAbilityTarget;

    AttackContextScope(_BattleStack_ *attacker, Patch *resetPatch)
        : patch(resetPatch), previousPatchState(patch && patch->IsApplied()),
          previousAttacker(combatActionAttacker), previousSorceressTarget(afterSorceressAbilityTarget),
          previousHellSteedTarget(afterHellSteedAbilityTarget), previousAbilityTarget(afterAnyAttackAbilityTarget)
    {
        if (patch)
        {
            if (attacker->active_spell_duration[SPL_BERSERK] == 0)
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
void __stdcall BattleStack_PrepareMelee(HiHook *h, _BattleStack_ *attacker, int direction)
{
    const AttackContextScope scope(attacker, skipMeleeBerserkReset);
    CALL_2(void, __thiscall, h->GetDefaultFunc(), attacker, direction);
}
void __stdcall BattleStack_PrepareShoot(HiHook *h, _BattleStack_ *attacker)
{
    const AttackContextScope scope(attacker, skipRangeBerserkReset);
    CALL_1(void, __thiscall, h->GetDefaultFunc(), attacker);
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

eSpell GetRandomDebuffSpellToCast(_BattleStack_ *atkStack, _BattleStack_ *targetMon)
{

    const int creatureNum = atkStack->count_current;
    const int arrayIndex = (creatureNum < 10) ? 0 : (creatureNum < 20) ? 1 : (creatureNum < 50) ? 2 : 3; // hate that
    const auto &spellDataArray = wogCreatureSpellData.basedOnCreatureAmountSpell[arrayIndex];
    int totalChance = 0;

    const int casterSide = atkStack->GetSide();

    for (const auto &spellData : spellDataArray)
    {
        if (spellData.spellID == eSpell::NONE)
            break;

        if (targetMon->CanUseSpell(spellData.spellID, casterSide, 1, 1))
            totalChance += spellData.chance;
    }

    if (totalChance == 0)
        return eSpell::NONE;

    int randomValue = Rand() % totalChance;
    for (const auto &spellData : spellDataArray)
    {
        if (spellData.spellID == eSpell::NONE)
            break;

        if (!targetMon->CanUseSpell(spellData.spellID, casterSide, 1, 1))
            continue;

        if (randomValue < spellData.chance)
            return spellData.spellID;

        randomValue -= spellData.chance;
    }
    return eSpell::NONE;
}

_LHF_(OnBeforeSorceressShot)
{
    afterSorceressAbilityTarget = reinterpret_cast<_BattleStack_ *>(c->eax);
    c->return_address = 0x075CA76;
    return NO_EXEC_DEFAULT;
}
_LHF_(OnBeforeHellSteedAttack)
{
    afterHellSteedAbilityTarget = reinterpret_cast<_BattleStack_ *>(c->eax);
    c->return_address = 0x075CA76;
    return NO_EXEC_DEFAULT;
}
_LHF_(OnBeforeCreatureAttack)
{
    afterAnyAttackAbilityTarget = reinterpret_cast<_BattleStack_ *>(c->eax);
    c->return_address = 0x075CAFE;
    return NO_EXEC_DEFAULT;
}

eSpell WoG_GetStackSpellToCast(_BattleStack_ *atkStack, _BattleStack_ *targetMon, const eSpell fixedSpellToCast)
{

    const eSpell spellToCast =
        fixedSpellToCast != eSpell::NONE ? fixedSpellToCast : GetRandomDebuffSpellToCast(atkStack, targetMon);

    if (spellToCast != eSpell::NONE &&
        CALL_6(_byte_, __thiscall, 0x05A8950, o_BattleMgr, spellToCast, atkStack->GetSide(), targetMon, 1, 1))
    {
        return spellToCast;
    }

    return eSpell::NONE;
}

void __stdcall OnAfterAttackDrawActionPlay(HiHook *h, _BattleMgr_ *mgr, const int anim, const int showAttack)
{

    CALL_3(void, __thiscall, h->GetDefaultFunc(), mgr, anim, showAttack);
    _BattleStack_ *atkStack = mgr->GetCurrentStack(); // reinterpret_cast<_BattleStack_ *>(0x282E60C);

    if (atkStack != combatActionAttacker || !combatActionAttacker)
    {
        return;
    }
    const int creatureNum = atkStack->count_current;

    // sorcery shooting attack
    auto target = afterSorceressAbilityTarget;
    if (target && target->count_current > 0 &&
        (creatureNum >= 40 || (creatureNum > 0 && (creatureNum << 1) + 20 > Randint(1, 100))))
    {
        const eSpell spellId = WoG_GetStackSpellToCast(atkStack, target, eSpell::NONE);
        if (spellId != eSpell::NONE)
        {
            target->spellAfterAttack = spellId;
            mgr->PlayMagicAnimation(-1, showAttack);
        }
    }

    // hell steed Fire Wall Attack
    target = afterHellSteedAbilityTarget;
    if (target && target->count_current > 0 && creatureNum > 0)
    {
        const eSpell spellId = WoG_GetStackSpellToCast(atkStack, target, eSpell::FIRE_WALL);
        if (spellId != eSpell::NONE)
        {
            target->spellAfterAttack = spellId;
            mgr->PlayMagicAnimation(-1, showAttack);
        }
    }

    // wog ability cast
    target = afterAnyAttackAbilityTarget;
    if (!target || target->count_current < 1 || creatureNum < 1 || IntAt(0x079F138) || // if Complete AI combat
        !CALL_2(BOOL, __cdecl, 0x071C16D, atkStack, 1)) // or CrExpo isn't prepared for attacker stack
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

            if (chanceToCast >= 100 || chanceToCast >= Randint(1, 100))
            {
                const eSpell spellId = WoG_GetStackSpellToCast(atkStack, target, spellToCast);

                if (spellId != eSpell::NONE)
                {
                    target->spellAfterAttack = spellId;
                    mgr->PlayMagicAnimation(-1, 0);
                }
            }
        }
    }
}

_LHF_(AdvManager_KeyPress)
{
    CALL_2(void, __thiscall, 0x47F0F0, o_AdvMgr, 1);// H3AdventureManager::StopCursor
    return EXEC_DEFAULT;
}
// ##############################################################################################################################
// ##############################################################################################################################
// ##############################################################################################################################

void GameLogic(PatcherInstance *_PI)
{

    // фикс: двуклеточные монстры в бою могут сделать 1 шаг назад
    // Hook only MOV EAX, 5; keep the native POP EDI / POP ESI.
    _PI->WriteLoHook(0x476020, Y_FixBattle_StackStepBack);

    // фикс: атакующий летающий стек НЕ пропускает ход при атаке заблоченного со всех сторон целевого стека
    _PI->WriteHiHook(0x523FE6, CALL_, EXTENDED_, THISCALL_, AIMgr_Stack_SetHexes_WayToMoveLength);

    // исправление созданий WoG'ом корявых пакованых координат
    _PI->WriteHiHook(0x711E7F, SPLICE_, DIRECT_, CDECL_, Y_WoG_MixedPos_Fix);
    _PI->WriteHiHook(0x711F49, SPLICE_, DIRECT_, CDECL_, Y_WoG_UnMixedPos_Fix);

    // исправление бага с исчезновением стартового героя при переигрывании
    _PI->WriteByte(0x5029C0, 0xEB);

    // Делаем кнопку отмены в Хижине Ведьмы
    _PI->WriteDword(0x4A7E63 + 1, 2);
    _PI->WriteHiHook(0x4A7E8A, CALL_, EXTENDED_, THISCALL_, Y_SetCanselWitchHut);

    // Делаем кнопку отмены у ученого, предлагающего втор.навык
    _PI->WriteLoHook(0x4A4AFE, Y_SetCanselScholarlySS);

    // фикс выбора типа атаки при битве ИИ vs человек (человек не мог выбрать тип атаки)
    // суть в том, что была проверка на флаг V997, а должна быть V998
    _PI->WriteByte(0x762601 + 3, 0xC5);

    // пропускаем показ всем игрокам захват Двеллинга 8-го уровня существ
    _PI->WriteByte(0x70DB3B + 1, 0x37);

    // логическая ошибка SOD:
    // пропускаем показ обновления экрана для ИИ в телепортах:
    // - если мы не видим территорию
    // - если стоит опция "не показывать передвижения противника"
    _PI->WriteByte(0x41DBE8 + 1, 0x5C);

    // радус открытия всей карты
    _PI->WriteDword(0x4F4B57, 320); // [чит - меню(ориг = 180)]
    _PI->WriteDword(0x4026FA, 360); // [чит wogeyeofsauron(ориг = 200)]
                                    // радус закрытия всей карты
    _PI->WriteDword(0x402751, 360); //[чит wogeyeofsauron(ориг = 200)]
    // © daemon_n
    // Добавить мифрил игроку при вводе чит-кода на ресурсы
    _PI->WriteLoHook(0x04027FE, AdvMgr_ApplyResourceCheat);
    // © daemon_n
    // при доступе к рынку в окне союзника без своих собственных (возможно через торговца артефактов)
    // курс делится на 0, что приводит к крашу при клику на ресурсах и артефактах
    _PI->WriteDword(0x678344, 0x3dcccccd);          // float 0.1
    _PI->WriteDword(0x678344 + 11 * 4, 0x3dcccccd); // float 0.1
    _PI->WriteDword(0x678344 + 22 * 4, 0x3dcccccd); // float 0.1

    // фикс WoG'a

    _PI->WriteHiHook(0x070561A, CALL_, DIRECT_, CDECL_, WoG_OnMapRestartCrtraitsReset);

    // © daemon_n
    // фикс ERM команды CB:M: при проверке/установке типа и количество существ значение ограничивалось 196 (Драколич)
    const int MAX_MON_ID = IntAt(0x4A1657);
    _PI->WriteDword(0x073A847 + 3, MAX_MON_ID + 1); // увечиличить макс id при проверке на выход из границы
    _PI->WriteDword(0x073A850 + 3, MAX_MON_ID);     // увеличить макс id при выходе за границы

    // © daemon_n
    // WoG баг, при котором, если защита отряда из оригинальной ф-ции = 0
    // и нет нападающего отряда при расчёте, игра возвращала мусорное значение;
    _PI->WriteLoHook(0x075D42C, WoG_BattleStack_GetDefenceAgainst);

    // © daemon_n
    // Ошибка WoG/ERM -- помещение банка существ не типа объекта 16 не инициализировала его как банк существ
    _PI->WriteLoHook(0x0713429, WoG_PlaceObject);

    // © daemon_n
    // фикс SoD бага, который не отмечает посещённосто университа игроком
    _PI->WriteLoHook(0x04AA153, AdvMgr_EnterToUniversity);

    // убираем клонов из показа в диалоге результатов битвы
    _PI->WriteLoHook(0x4708FC, Y_Dlg_BattleResults_IgnoreClones);

    // Темница(5) на поверхности генерируется на родном типе земли(6), а не на грязи(0)
    _PI->WriteByte(0x464044, 0xEB);
    //_PI->WriteDword(0x6408D8 + 5*4, 6);

    // фикс бага при удалении объектов - затираются данные,
    // если жёлтая клетка другого объекта
    // стоит выше на 1 ед. по y-координате
    // _PI->WriteLoHook(0x4AA979, Y_OnDeleteObjectOnMap);
    _PI->WriteHiHook(0x4aa820, SPLICE_, EXTENDED_, THISCALL_, HiHook_004aa820);
    _PI->WriteLoHook(0x04AA9B3, LoHook_004AA9B3);
    _PI->WriteLoHook(0x04AA9BF, LoHook_004AA9BF);
    _PI->WriteLoHook(0x04AA9EA, LoHook_004AA9BF);

    // © JackSlater
    // Фикс бага SoD - Невозможно колдовать более сильную версию заклинания "Полёт"
    // Если герой уже колдовал его на этот ход
    _PI->WriteLoHook(0x041C879, js_Cast_AdventureMagic_BeforeCheckFlyPower);

    // © JackSlater
    // Фикс бага SoD - усилители заклинаний от героя не работали при касте существами
    _PI->WriteLoHook(0x05A065D, js_BattleMgr_CastSpell_BeforeSwitchCase);

    // © JackSlater
    // Исправляем баг SoD: сброс посещённости сирен после боя.
    _PI->WriteCodePatch(0x4DA8FC, "%n", 24); // 24 nop

    // © SadnessPower
    // Фикс Бага воскрешения командиром существ со здоровьем <=50, а не макс 5-го уровня
    _PI->WriteLoHook(0x05A881C, BattleStack_AtGettingResurrectionResistance);

    // © JackSlater
    // Прыжок через ProcessMessagesForTime, если скрытая битва
    _PI->WriteLoHook(0x441B25, LoHook_00441B25);
    _PI->WriteLoHook(0x441BD6, LoHook_00441BD6); // return address ->0x441BF5

    // Оковы войны действуют только в битве двух героев (ИИ).
    _PI->WriteLoHook(0x41E74A, LoHook_ShacklesRest_AI);
    // Оковы войны действуют только в битве двух героев (побег).
    _PI->WriteLoHook(0x478DA2, LoHook_ShacklesRest_Surrend);
    // Оковы войны действуют только в битве двух героев (сдача).
    _PI->WriteLoHook(0x478EBC, LoHook_ShacklesRest_GU);

    // Баг: срабатывание огненного щита по трупу - перепрыгиваем AfterAttackAbilities и GetFireshieldDamage, если
    // count_current <=0
    _PI->WriteLoHook(0x441982, LoHook_00441982);
    // Баг: перед контратакой - проверка на count_current цели
    _PI->WriteLoHook(0x441AFF, LoHook_00441AFF);

    // © daemon_n
    // фикс RNG: отвязываем генерацию случайных чисел от проигрывания анимации в битве

    _PI->WriteHiHook(0x0463606, CALL_, EXTENDED_, THISCALL_, BattleMgr_SetRandomSeed);

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
        _PI->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AnimationRandom);
    }
    DWORD pureRandomFunctionAddresses[] = {0x0050B3DF}; // wait animation
    for (DWORD addr : pureRandomFunctionAddresses)
    {
        _PI->WriteHiHook(addr, CALL_, DIRECT_, CDECL_, BattleMgrProc_AnimationPureRandom);
    }
    // DWORD lowHighAIQuickRandomFunctionAddresses[] = {0x0042723C, 0x00427260}; // AI_Calc_FastBattle_Finishing
    // for (DWORD addr : lowHighAIQuickRandomFunctionAddresses)
    //{
    //     _PI->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AIQuickRandom);
    // }
    // Random AI tie-break and the AI-only extra bad-morale roll.
    // Luck rolls and the primary morale rolls stay on the game RNG.
    DWORD lowHighAIRandomFunctionAddresses[] = {0x00421C82, 0x004647D0};
    for (DWORD addr : lowHighAIRandomFunctionAddresses)
    {
        _PI->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, BattleMgrProc_AIRandom);
    }

    // Era::RegisterHandler(OnSetupBattleField, "OnSetupBattleField");

    // DWORD lowHighDebugRandomFunctionAddresses[] = {/*0x00443024*/,
    //                                                /* 0x00442FE9,*/ /* 0x004645B3, 0x004647A7, 0x04647D0,*/
    //                                                                 /* 0x00443F00, 0x00443F29, 0x00420F18,*/
    //                                                /*0x005A2100*/};
    // for (DWORD addr : lowHighDebugRandomFunctionAddresses)
    //{
    //     // _PI->WriteHiHook(addr, CALL_, EXTENDED_, FASTCALL_, DEBUG_Random);
    // }
    // © daemon_n


    if (!TIPHON)
    {

        // © JackSlater
        // фикс бага при получении хинта от Магических Святынь
        // ранее использовался массив с заклинаиями от артефактов 0x430 -> 0x3EA

        _PI->WriteWord(0x40D979 + 3, 0x3EA);
        // Фикс Уланда - герой имеет продвинутую мудрость на старте
        o_HeroInfo[HID_ULAND].second_skill_1_lvl = 1;
        // Фикс Димера - герой имеет продвинутую разведку на старте
        o_HeroInfo[HID_DEEMER].second_skill_2_lvl = 1;
    }

    // © daemon_n
    // исправление отображаемого def при разрушении зданий в городе без построенного форта
    _PI->WriteLoHook(0x70BF78, TownDemolishion_BeforeBuildingRebuild);
    _PI->WriteLoHook(0x070AA96, TownDemolishion_BeforeGraphicsChange);

    // Баг? _BattleStack_::MeleeAtack (контратака) - фикс сайда
    _PI->WriteHiHook(0x441b5d, CALL_, EXTENDED_, THISCALL_, HiHook_00441b5d);
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////// Фиксы раздвоения героя //////////////////////////

    // фикс бага встречи одного и того-же героя: игрой в редких случаях перезатирается ecx
    // в дополнение: запрещаем встречу, если номера у героев одинаковые
    _PI->WriteLoHook(0x4A2476, Y_Fix_HeroesInteract);

    // проверка взаимодействия героя в лодке с героем в другой лодке
    // фиксим: вносим проверку на воду под героем 2, вдруг он стоит на воде
    // например если остановился на MapEvent
    _PI->WriteLoHook(0x4814C0, Y_Fix_HeroesOnWaterCheckInteract);
    // прописываем возврат регистров
    _PI->WriteByte(0x4814DD + 0, 0x90); // nop
    _PI->WriteByte(0x4814DD + 1, 0x5F); // pop EDI
    _PI->WriteByte(0x4814DD + 2, 0x5E); // pop ESI

    // добавляем проверку на воду: mapItem->land == 8 (и убираем проверку на лодку)
    _PI->WriteCodePatch(0x4806D9, "F743 04 08000000 752A 909090");

    // © daemon_n
    // фикс: посещение мифрила обновляет экран
    _PI->WriteCodePatch(0x07060AC, "%n", 7); // удалить добавление мифрила костылём
    _PI->WriteJmp(0x0705F42, 0x0705F7D);     // пропустить изменение объекта на карте на костыльное решение

    // Исправление копейщиков в лагерях беженцев (только для клетки с триггером).
    _PI->WriteCodePatch(0x505E15, "%n", 4);
    _PI->WriteLoHook(0x505E15, LoHook_FixRefugeeCamp_dx);

    // Теперь в городах игрока при наличии форта всегда будет отстроено 2 уровня существ.
    _PI->WriteCodePatch(0x5C1051, "%n", 20); // 20 nop
    // Убираем случайность количества стеков в начальной армии.
    _PI->WriteCodePatch(0x4C948F, "%n", 5);  // 5 nop
    _PI->WriteCodePatch(0x4C9497, "%n", 10); // 10 nop
    _PI->WriteCodePatch(0x4C950F, "%n", 20); // 20 nop

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////// Фиксы крит.крашей игры //////////////////////////

    // фикс вылета: нет проверки на на наличие стуктуры целевого стека
    // но тут не хватает проверки на c->edi
    _PI->WriteLoHook(0x5A1C17, Y_FixCrash_CastSpell_38);

    // фикс вылета: при удалении препятствия в битве, когда его стуктура таблицы равна нуля
    // (привет WoG и его стена огня у Огненных Лошадей)
    _PI->WriteLoHook(0x46681B, Y_FixCrash_RemoveObstacle);

    // решение вылета битвы, когда её инициируют в городе (например ERM или очарование Суккубов)
    _PI->WriteHiHook(0x4B09D0, SPLICE_, EXTENDED_, THISCALL_, Y_CallManager);

    // убираем обновление инфо-панели и панели ресурсов, если активный менеджер не менеджер карты приключений
    _PI->WriteHiHook(0x403F00, SPLICE_, EXTENDED_, THISCALL_, Y_RedrawResources);
    _PI->WriteLoHook(0x415D4D, Y_AdvMgr_RedrawInfoPanel);

    //// ЧИТ-Меню ////
    // Увеличиваем кол-во заклинаний 999 -> 9999
    _PI->WriteWord(0x4F508F + 4, 9999);
    _PI->WriteWord(0x4F50DE + 4, 9999);
    _PI->WriteWord(0x4F511A + 4, 9999);
    // Увеличиваем кол-во существ 5 -> 100
    _PI->WriteByte(0x4F4ED2 + 1, 100);

    // © daemon_n
    // исправление чита "построить все здания". На сбрасывается флаг введённого чита
    Era::RegisterHandler(OnGameLeave, "OnGameLeave");
    // © daemon_n
    // отключение сброса эффекта берсеркера, если он был получен лишь в процессе атаки, а не на начало хода
    // исправление имеет смысл для срабатывания берсерка при получении эффекта в качестве ответного удара/выстрела;
    skipMeleeBerserkReset = _PI->WriteJmp(0x0441C75, 0x0441C7E);
    skipRangeBerserkReset = _PI->WriteJmp(0x0440017, 0x0440020);
    // WriteJmp applies immediately; the skips belong only to an active attack.
    if (skipMeleeBerserkReset)
        skipMeleeBerserkReset->Undo();
    if (skipRangeBerserkReset)
        skipRangeBerserkReset->Undo();
    _PI->WriteHiHook(0x4419D0, SPLICE_, EXTENDED_, THISCALL_, BattleStack_PrepareMelee);
    _PI->WriteHiHook(0x43FE80, SPLICE_, EXTENDED_, THISCALL_, BattleStack_PrepareShoot);

    // © daemon_n
    // переписываем логику колдовства заклинания после атаки на действительное колдовство после атаки
    _PI->WriteLoHook(0x075C9A1, OnBeforeSorceressShot);
    _PI->WriteLoHook(0x075CA66, OnBeforeHellSteedAttack);
    _PI->WriteLoHook(0x075CAA6, OnBeforeCreatureAttack);

    _PI->WriteHiHook(0x43FA11, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // magog shooting
    _PI->WriteHiHook(0x43FE0B, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // lich shooting
    _PI->WriteHiHook(0x43FA7C, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // native shooting
    _PI->WriteHiHook(0x4418A5, CALL_, EXTENDED_, THISCALL_, OnAfterAttackDrawActionPlay); // melee attack
    // исправляем хук "OnAfterShooting", чтобы работал для всех типов атак, а не только для стрельбы
    // заодно специальные стрелки получают звук атаки;
    // _PI->WriteJmp(0x43FA16, 0x43FAD9); // magog shooting
    // _PI->WriteJmp(0x43FE10, 0x43FAD9); // lich shooting IMPLEMENT LATER W/ LoHook- It has some referes to the
    // replaceable address;



    _PI->WriteCodePatch(0x408ff7, "%n", 9);

    _PI->WriteLoHook(0x408ff2, AdvManager_KeyPress);


}
