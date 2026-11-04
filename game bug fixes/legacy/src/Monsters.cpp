/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

int __stdcall setActStack(LoHook *h, HookContext *c)
{
    _BattleStack_ *mon = (_BattleStack_ *)(int)(c->esi - 656);

    // если выстрелов у катапульты или балисты больше нет
    // то делаем пропуски передачи хода этим машинам
    if (CALL_1(_bool8_, __fastcall, 0x47AAB0, mon->creature_id)) // isWarMachine
    {
        if (mon->creature.shots < 1)
        {
            c->return_address = 0x464DAD;
            return NO_EXEC_DEFAULT;
        }
    }
    // если выстрелов у атакующих стену больше нет
    else if (mon->creature.destroyWalls && mon->creature.shots < 1)
    {
        // то забираем флаг "катапульта"
        mon->creature.destroyWalls = false;
    }
    return EXEC_DEFAULT;
}

// создание инкремента боезапаса при атаке осадных стен "катапульта"
int __stdcall catapultaShoot(LoHook *h, HookContext *c)
{
    _BattleStack_ *mon = (_BattleStack_ *)c->edi;
    mon->creature.shots -= 1;

    return EXEC_DEFAULT;
}

// запрет на второй выстрел, если боезапас закончился
// для баллисты и стрелков
int __stdcall monstreShoot(LoHook *h, HookContext *c)
{
    _BattleStack_ *mon = (_BattleStack_ *)c->esi;

    if (mon->creature.shots < 1)
    {
        c->return_address = 0x43FFFC;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

_int_ __stdcall Y_FixBagCreatureGredeOfNeutrals(HiHook *hook, _Army_ *army, _int_ creature_id)
{
    _int_ count = 0;
    _int_ i = 0;
    _int_ crGrade_id = GetCreatureUprade(creature_id);
    do
    {
        if (army->type[i] == creature_id || army->type[i] == crGrade_id)
        {
            count += army->count[i];
        }
        i++;
    } while (i < 7);

    return count;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

int __stdcall fixHarpyBinds(LoHook *h, HookContext *c)
{
    if (*(int *)(c->ebx + 696))
    {
        c->return_address = 0x478365;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// фикс возвращения Темных Драконов после атаки врага сзади
_byte_ __stdcall Y_FixMoveDarkDragonsAfterAttack(HiHook *hook, _BattleStack_ *stack, _int_ gex, _byte_ a3)
{
    // проверяем стек на двухклеточность (только у них есть этот баг)
    if (stack->creature.flags & BCF_2HEX_WIDE)
    {
        // проверяем повернут ли стек после атаки
        if (stack->orientation == stack->side)
        {
            if (stack->side)
                gex--;
            else
                gex++;
        }
    }

    return CALL_3(_byte_, __thiscall, hook->GetDefaultFunc(), stack, gex, a3);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// не считать кавалерийский бонус при полете
_int_ __stdcall Y_AntiKavalierAndFly(LoHook *h, HookContext *c)
{
    if (*(_dword_ *)(c->ebx + 132) >> 1 & 1)
    {                                 // проверить флаг атакующего на полет
        c->return_address = 0x4430A3; // обходим расчет кавалерийского бонуса (он всё равно не работает)
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

int __cdecl ERM_Fix_EA_E(HiHook *hook, _BattleStack_ *stack)
{
    int ret = 0;
    _int32_ spell_duration[81]; // для сохранения длительности заклинаний
    _int32_ spells_power[81];   // для сохранения силы действия заклинания

    if (stack)
    {
        for (int i = 0; i < 80; i++)
        {
            spell_duration[i] = stack->active_spell_duration[i];
            spells_power[i] = stack->active_spells_power[i];

            if (spell_duration[i] > 0) // если заклинание наложено на стек, то сбрасываем его эффект
                CALL_2(int, __thiscall, 0x444230, stack, i); // ResetSpellFromStack 0x444230
        }
    }

    ret = CALL_1(int, __cdecl, hook->GetDefaultFunc(), stack);

    for (int i = 0; i < 80; i++)
    {
        if (spell_duration[i] > 0)
        { // если заклинание ранее было наложено, то восстанавливаем его
            CALL_5(int, __thiscall, 0x444610, stack, i, spell_duration[i], spells_power[i], 0); // ApplySpell 0x444610
        }
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include <set>
// Решаем проблему когда бонусы специалистов не считаются Супер существам
// обновлено: теперь поддерживается бесконечная цепочка улучшений существ, а не только юниты 7-го уровня
// обновлено: добавлен учёт апгрейда при расчёте максмального запаса хода героя
inline int FindUpgradedCreature(HookContext *c, const int returnAddress, const int creatureId)
{
    int heroCreatureSpec = c->ecx;

    std::set<int> usedCreatures;
    while (heroCreatureSpec >= 0 && usedCreatures.insert(heroCreatureSpec).second)
    {
        if (heroCreatureSpec == creatureId)
        {
            c->eax = heroCreatureSpec;
            c->return_address = returnAddress;
            return NO_EXEC_DEFAULT;
        }

        heroCreatureSpec = CALL_1(int, _cdecl, 0x0724A5F, heroCreatureSpec); // wog function to get creature grade
    }

    return EXEC_DEFAULT;
}
_LHF_(Hero_SpecCreatureInfo_GetCreatureUprade)
{
    const int creatureId = IntAt(c->ebp + 8);
    return FindUpgradedCreature(c, 0x04E64FF, creatureId);
}
_LHF_(Hero_GetMaxMovementPoints_GetCreatureUprade)
{
    const int creatureId = c->esi;
    return FindUpgradedCreature(c, 0x04E4EAB, creatureId);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// корректировка WoG ненависти существ
// добавляем и существ 8-го уровня
_int_ __stdcall Y_SetWogHates(LoHook *h, HookContext *c)
{
    int mult = 0; // бонус урона ненависти в процентах
    int amtype = *(int *)(c->ebp - 4);
    int dmtype = *(int *)(c->ebp - 8);

    switch (amtype)
    {
    case 12:  // Ангел
    case 13:  // Архангел
    case 150: // Верховный Архангел
        if (dmtype == 54 || dmtype == 55 || dmtype == 153)
            mult = 50;
        break;

    case 36: // Джин
    case 37: // Мастар Джин
        if (dmtype == 52 || dmtype == 53)
            mult = 50;
        break;

    case 41:  // Титан
    case 152: // Громовержец
        if (dmtype == 83 || dmtype == 155)
            mult = 50;
        break;

    case 52: // Ифрит
    case 53: // Ифрит Султан
        if (dmtype == 36 || dmtype == 37)
            mult = 50;
        break;

    case 54:  // Дьявол
    case 55:  // Архидьявол
    case 153: // Барон Ада
        if (dmtype == 12 || dmtype == 13 || dmtype == 150)
            mult = 50;
        break;

    case 83:  // Черный Дракон
    case 155: // Тёмный Дракон
        if (dmtype == 41 || dmtype == 152)
            mult = 50;
        break;

    default:
        break;
    }
    // теперь бонус урона нужно занести в EDX
    c->edx = mult;
    c->return_address = 0x766EEB;
    return NO_EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Решение бага ERM: триггер MA:U#/-2 приводил к тому, что любое существо при установке такой команды улучшалось в
// копейщика
int __stdcall Fix_WoG_GetCreatureGrade_Expo(LoHook *h, HookContext *c)
{
    if (*(int *)(c->ebp - 4) < -1)
    {
        *(int *)(c->ebp - 4) = -1;
    }
    return EXEC_DEFAULT;
}

int __stdcall Fix_WoG_GetCreatureGrade_Town(LoHook *h, HookContext *c)
{
    if (*(int *)0x27F93B0 < -1)
    {
        *(int *)0x27F93B0 = -1;
    }
    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

#define WOG_HeroEntered2Object (*(_Hero_ **)0x803300)

// показ лычек опыта в диалоге присоедиения монстров (он же диалог гарнизона)
int __stdcall Y_Dlg_AddCreatures_Init_Add(HiHook *hook, _Hero_ *hero, int creatureType, int creatureCount)
{
    if (hero)
        WOG_HeroEntered2Object = hero;

    return CALL_3(int, __fastcall, hook->GetDefaultFunc(), hero, creatureType, creatureCount);
}

// показ лычек опыта в диалоге оставления монстров на карте (он же диалог гарнизона)
int __stdcall Y_Dlg_AddCreatures_Init_Leave(HiHook *hook, _Hero_ *hero, _Army_ *army, int creatureType)
{
    if (hero)
        WOG_HeroEntered2Object = hero;

    return CALL_3(int, __fastcall, hook->GetDefaultFunc(), hero, army, creatureType);
}

// фикс кол-ва артов (опыт существ) при осаде города, когда армия охраны соединяется с армией героя гостя
_LHF_(Y_Fix_CrExpo_ArtsNum_OnArmy_AddCreatures_BeforeBattleInTown)
{
    DwordAt(c->ebp - 0x4) = 0;
    DwordAt(c->ebp - 0x24) = 0;
    return NO_EXEC_DEFAULT;
}

// баг, из-за которого палатки могут атаковать вблизи.
// Причина в построении вектора пути для боевых машин (© daemon_n)
_LHF_(Gem_OnBattleStackCheckReachability)
{
    _BattleStack_ *stack = *(_BattleStack_ **)(c->ebp + 8);

    if (stack && (stack->creature.flags & BCF_CANT_MOVE || CALL_1(_bool8_, __fastcall, 0x47AAB0, stack->creature_id)))
    {
        c->return_address = 0x4B332A;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
// При настройке путей к гексам учитываем, что невозможно атаковать стек, встав на препятствие.
int __stdcall LoHook_SetHexWays_SkipAttackIfObstackle(LoHook *h, HookContext *c)
{
    // Не надо становиться на препятствие - выполняем.
    if ((((_Struct_ *)(c->ebp - 80))->Field<_dword_>(4) & 0xFC000000) == 0)
    {
        return EXEC_DEFAULT;
    }
    // Пропускаем проверку стеков, если надо встать на препятствие.
    else
    {
        c->return_address = 0x4B3724;

        return NO_EXEC_DEFAULT;
    }
}
// исправление диалога присоединения монстров
// в Воге нет проверки на присоединение монстров с кол-вом меньше одного
_LHF_(Y_Fix_CrChangeDialog)
{
    for (int i = 0; i < 7; ++i)
    {
        _int_ adress = (0x83F354 + i * 4);
        _int_ type = IntAt(adress);
        _int_ count = IntAt(adress + 28);

        if (type != ID_NONE && count < 1)
        {
            IntAt(adress) = ID_NONE; // type = -1
            IntAt(adress + 28) = 0;  // count = 0
        }
    }

    return EXEC_DEFAULT;
}
// © JackSlater
// Фикс бага SoD - При проверке возможности наложения заклинаний
// Чародеи теперь учитывают все отряды, а не только первый с каждой стороны
_LHF_(gem_EnchantersFindTargetsForSpell)
{
    c->edx += c->edi * sizeof(_BattleStack_);
    return EXEC_DEFAULT;
}
_LHF_(gem_EnchantersTryToCastSelectSpell)
{
    c->edx += c->esi * sizeof(_BattleStack_);
    return EXEC_DEFAULT;
}

// © Archer30
// Rebalance Hill Forts - the cost of upgrade is calculated based on the level of the upgraded monster instead of the
// pre-upgraded monster
_LHF_(gem_OnGetHillFortMonLevel)
{
    const int monToUpgrade = c->eax;
    if (monToUpgrade > -1)
    {
        const int monUpgrade = GetCreatureUpradeWOG(monToUpgrade);
        if (monUpgrade > -1)
        {
            c->eax = monUpgrade;
        }
    }
    return EXEC_DEFAULT;
}

// © JackSlater
// Фикс бага SoD - двойной урон абилки рыцаря смерти работал только на базовый урон
_LHF_(LoHook_FixDoubleDamage_DreadKnight)
{
    c->edx <<= 1;
    return EXEC_DEFAULT;
}
// © JackSlater
// Фикс бага SoD - двойной урон баллисты работал только на базовый урон
_LHF_(LoHook_FixDoubleDamage_Ballista)
{
    c->edi <<= 1;
    return EXEC_DEFAULT;
}

// © daemon_n
// Fix bug with cloning monsters when the monster limit is reached
_LHF_(gem_AI_GetCloneCastValue)
{
    if (o_BattleMgr->countMonsters[o_BattleMgr->currentActiveSide] >= 20)
    {
        c->return_address = 0x43AFAC;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
typedef _BattleStack_ H3CombatCreature;
// © JackSlater
// Существа, из которых были подняты демоны, считаются пожертвованными.
_LHF_(LoHook_DemonResurrect_DeadForever)
{
    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    // Пожертвован.
    reinterpret_cast<H3CombatCreature *>(c->esi)->creature.sacrificed = true;

    return EXEC_DEFAULT;
}

// © JackSlater
// Исправление бага с воскрешением фениксов после исчезновения.
_LHF_(LoHook_Fix_Fenix_Resurrect)
{
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    if (stack->creature.sacrificed || stack->creature.summon)
    {
        c->return_address = 0x469192;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
// © JackSlater
// Вампиризм не работает на клонов
_LHF_(LoHook_BattleStackVampirism)
{
    if ((*reinterpret_cast<H3CombatCreature **>(c->ebp + 0x8))->creature.clone)
    {
        c->return_address = 0x4412AB;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
// © daemon_n
// исправление проверки id существ к иммунитета к превращению в оборотня
_LHF_(WoG__WereWolfImmunityCheck)
{
    const int targetCreatureID = IntAt(c->ebp - 0x8);
    // командиры и боги теперь корректно получают иммунитет
    if (Era::IsCommanderId(targetCreatureID) ||
        targetCreatureID >= CID_EMISSARY_OF_WAR && targetCreatureID <= CID_EMISSARY_OF_LORE)
    {
        c->return_address = 0x076778B;
    }
    else
    {
        c->return_address = 0x0767516;
    }

    return NO_EXEC_DEFAULT;
}
// © daemon_n
// удаляем лишнюю логику при скрытой битве
_LHF_(WoG__WereWolfAction)
{
    if (o_BattleMgr->IsHiddenBattle())
    {
        c->return_address = 0x07676F6;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
// © daemon_n
// отключение сообщения с нанесением урона при его блокировании Драколичами или прочими абилками
const _BattleStack_ *blockingStack = nullptr;
_LHF_(CrExpBon_StackBlock_DracolichBlock)
{
    blockingStack = *reinterpret_cast<const _BattleStack_ **>(c->ebp + 0x8);
    return EXEC_DEFAULT;
}
void __stdcall BattleMgr_AttackingLogMessage(HiHook *h, _BattleMgr_ *_this, char *AttackerName, int attackerCount,
                                             int damage, _BattleStack_ *target, int killedCount)
{

    if (blockingStack && target == blockingStack)
    {
        blockingStack = nullptr;
        return;
    }
    CALL_6(void, __thiscall, h->GetDefaultFunc(), _this, AttackerName, attackerCount, damage, target, killedCount);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Monsters(PatcherInstance *_PI)
{
    // исправления стрельбы при отрицат.боезапасе
    // при передаче хода стеку
    _PI->WriteLoHook(0x464D75, setActStack);
    // при стрельбе по стенам (катапульта, циклопы)
    _PI->WriteLoHook(0x445CF9, catapultaShoot);
    // второй выстрел монстрами
    _PI->WriteLoHook(0x43FF92, monstreShoot);
    // второй выстрел баллистой
    _PI->WriteLoHook(0x43FFF4, monstreShoot);

    // исправление диалога присоединения монстров
    // в Воге нет проверки на присоединение монстров с кол-вом меньше одного
    _PI->WriteLoHook(0x714BE5, Y_Fix_CrChangeDialog);

    // исправление бага палатки, когда на её ходу невозможно убежать или сделать другие действия
    _PI->WriteByte(0x75C82C, 0xEB);

    // фикс бага блока существ (от опыта мностров), заключается в том
    // что существа бесконечно получают ход в текущем раунде и игра виснет
    // _PI->WriteJmp(0x71C685, 0x71C69D); // убираем установку флага "в защите" (старый способ - без анимации)
    _PI->WriteHexPatch(0x71C685, "8B4D 08 C601 01 C641 02 04 EB0C 90 90 90");

    // Решение бага (еще с SoD) исчезновения улучшенного стека при битве с даунгрейдом нейтралов
    _PI->WriteHiHook(0x4AC5F5, CALL_, EXTENDED_, THISCALL_, Y_FixBagCreatureGredeOfNeutrals);

    // фикс отлета гарпий, когда после удара они связаны корнями дендроидов
    _PI->WriteLoHook(0x47835B, fixHarpyBinds);

    // фикс возвращения Темных Драконов после атаки врага сзади
    _PI->WriteHiHook(0x478360, CALL_, EXTENDED_, THISCALL_, Y_FixMoveDarkDragonsAfterAttack);

    // не считать кавалерийский бонус при полете
    _PI->WriteLoHook(0x44307A, Y_AntiKavalierAndFly);

    // Решение бага Вога, когда в бою накладывается опыт через EA:E и атака, защита, уроны, скорость, боезапасы и т.п.
    // заново пересчитываются. Из-за этого теряются бонусы наложенных заклинаний (например бонус скорости от ускорения)
    _PI->WriteHiHook(0x726DE4, CALL_, EXTENDED_, CDECL_, ERM_Fix_EA_E);

    // вызовы драконов от артефакта сердце дракона
    // меняем местами номера гексов,
    // чтобы в банках существ не перекрывался стек №3
    _PI->WriteByte(0x767A05 + 1, 55);
    _PI->WriteByte(0x767A2A + 1, 89);

    // Решение бага ERM: триггер MA:U#/-2 приводил к тому, что любое существо при установке такой команды улучшалось в
    // копейщика
    _PI->WriteDword(0x724A9F, -2);
    _PI->WriteLoHook(0x724AC5, Fix_WoG_GetCreatureGrade_Expo);
    _PI->WriteDword(0x74ED27, -2);
    _PI->WriteLoHook(0x74ED5C, Fix_WoG_GetCreatureGrade_Town);

    // Решаем проблему когда бонусы специалистов не считаются Супер существам
    // _PI->WriteHiHook(0x4E64FA, CALL_, EXTENDED_, FASTCALL_, Y_FixWoG_GetCreatureGrade);
    _PI->WriteLoHook(0x4E64D1, Hero_SpecCreatureInfo_GetCreatureUprade); // бонусы специалиста существ
    _PI->WriteLoHook(0x4E4E7E, Hero_GetMaxMovementPoints_GetCreatureUprade);

    // показ лычек опыта в диалоге присоединения и оставления монстров на карте
    _PI->WriteHiHook(0x5D15D0, SPLICE_, EXTENDED_, FASTCALL_, Y_Dlg_AddCreatures_Init_Add);
    _PI->WriteHiHook(0x5D16B0, SPLICE_, EXTENDED_, FASTCALL_, Y_Dlg_AddCreatures_Init_Leave);

    // фикс Вога: при нападении на город армия в городе соединяется с армией героя
    // но Вог, когда не находит опыт существа, обнуляет опыт, но не обнуляет кол-во артов
    // и поэтому вместо количества артов суёт мусор. Мы - обнулим количество артов
    _PI->WriteLoHook(0x759C78, Y_Fix_CrExpo_ArtsNum_OnArmy_AddCreatures_BeforeBattleInTown);
    _PI->WriteLoHook(0x759E7D, Y_Fix_CrExpo_ArtsNum_OnArmy_AddCreatures_BeforeBattleInTown);

    // убираем отображение двух ошибок от опыта существ
    _PI->WriteCodePatch(0x717B37, "%n", 20); // 20 nops
    _PI->WriteCodePatch(0x717FAB, "%n", 20); // 20 nops

    // баг, из-за которого палатки могут атаковать вблизи.
    // Причина в построении вектора гексов целей
    // © daemon_n
    _PI->WriteLoHook(0x4B3309, Gem_OnBattleStackCheckReachability);
    // При настройке путей к гексам учитываем, что невозможно атаковать стек, встав на препятствие.
    // (+ это исправление бага с пропуском хода летающим при нажатии).
    _PI->WriteLoHook(0x4B352F, LoHook_SetHexWays_SkipAttackIfObstackle);
    // Не перезаписываем направление для атакуемого гекса.
    _PI->WriteCodePatch(0x4B37A5, "%n", 20);
    _PI->WriteCodePatch(0x4B37BC, "%n", 3);

    // меняем картинки оставления монстров и для эстетики
    // и для того, чтобы визуально понимать, что мы вносили правку
    _PI->WriteByte(0x757365 + 1, 35); // 35 - Меч Правосудия
    _PI->WriteByte(0x757369 + 1, 13); // 13 - Архангел

    // © JackSlater
    // Фикс бага SoD - При проверке возможности наложения заклинаний
    // Чародеи теперь учитывают все отряды, а не только первый с каждой стороны
    _PI->WriteLoHook(0x0447D9C, gem_EnchantersFindTargetsForSpell);
    _PI->WriteLoHook(0x0447E4A, gem_EnchantersTryToCastSelectSpell);

    // Исправления недочёта SoD с отобржением атаки с бонусами стека.
    _PI->WriteCodePatch(0x5F37EA, "%n", 11); // 11 nop
    _PI->WriteCodePatch(0x5F37F8, "%n", 5);  // 5 nop
    _PI->WriteCodePatch(0x5F3800, "%n", 5);  // 5 nop

    // © Archer30
    // Rebalance Hill Forts - the cost of upgrade is calculated based on the level of the upgraded monster instead of
    // the pre-upgraded monster ; This is considered a bug fix as in the original H3, every upgrade has the same
    // creature level before and after
    // In WoG with Extended Upgrade option, it's possible to upgrade Gremlins (level 1) to Santa Gremlins (leve 3)
    //    for free (since the fee is calculated based on Gremlins)
    // This script fixes this so that such upgrade would costs a little amount of fee(but still with discount as
    //     Hill Forts always do to low level creatures)
    _PI->WriteLoHook(0x4E800F, gem_OnGetHillFortMonLevel);

    // © JackSlater
    // Фикс бага SoD - двойной урон баллисты и абилки рыцаря смерти работал только на базовый урон
    _PI->WriteLoHook(0x4436EA, LoHook_FixDoubleDamage_DreadKnight);
    _PI->WriteLoHook(0x443615, LoHook_FixDoubleDamage_Ballista);

    // © daemon_n
    // Fix bug with cloning monsters when the monster limit is reached
    _PI->WriteLoHook(0x43AF42, gem_AI_GetCloneCastValue);

    // Существа, из которых были подняты демоны, считаются пожертвованными.
    _PI->WriteLoHook(0x5A76C6, LoHook_DemonResurrect_DeadForever);
    // Исправление бага с воскрешением фениксов после исчезновения.
    _PI->WriteLoHook(0x46907B, LoHook_Fix_Fenix_Resurrect);
    // Вампиризм не работает на клонов
    _PI->WriteLoHook(0x440935, LoHook_BattleStackVampirism);

    // © daemon_n
    // исправление проверки id существ к иммунитета к превращению в оборотня
    _PI->WriteLoHook(0x07674FA, WoG__WereWolfImmunityCheck);
    // © daemon_n
    // удаляем лишнюю логику при скрытой битве
    _PI->WriteLoHook(0x0767644, WoG__WereWolfAction);
    // © daemon_n
    // отключение сообщения с нанесением урона при его блокировании Драколичами или прочими абилками
    _PI->WriteLoHook(0x071C6CF, CrExpBon_StackBlock_DracolichBlock);
    _PI->WriteHiHook(0x0469670, SPLICE_, EXTENDED_, THISCALL_, BattleMgr_AttackingLogMessage);
    // патчи без Tiphon.dll
    if (!TIPHON)
    {
        // корректировка WoG ненависти существ
        // добавляем и существ 8-го уровня
        _PI->WriteLoHook(0x766E4E, Y_SetWogHates);

        // восстанавливаем двойную атаку Фанатикам Войны
        o_CreatureInfo[CID_WAR_ZEALOT].doubleAttack = true;
        // установка флага "король1" для горынычей, так как они чудовища 7-го уровня
        o_CreatureInfo[CID_GORYNYCH].king1 = true;

        // удаляем иммунитеты к огню у призраков
        o_CreatureInfo[CID_GHOST].fireImmunity = false;

        // удаляем иммунитеты к огню у посланников
        o_CreatureInfo[CID_FIRE_MESSENGER].fireImmunity = false;
        o_CreatureInfo[CID_EARTH_MESSENGER].fireImmunity = false;
        o_CreatureInfo[CID_AIR_MESSENGER].fireImmunity = false;
        o_CreatureInfo[CID_WATER_MESSENGER].fireImmunity = false;
    }
}
