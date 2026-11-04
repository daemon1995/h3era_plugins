/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AI
{
// © Raistlin
// Исправляем косяк, из-за которого ИИ не мог использовать торговцев артефактами в Сопряжении
int __stdcall R_FixAI_ConfluxNerchant(LoHook *h, HookContext *c)
{
    if ((c->eax & 0xFF) == TOWN_CONFLUX) // al
    {
        *(int *)(c->ebp - 36) = c->ecx;
        c->return_address = 0x525EE0;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
// Фикс бесконечного цикла ИИ: когда он оценивает ценность получения стека обнуляем отрицательное значение, чтобы
// избежать переполнения.
int __stdcall Fix_AI_GetBestStackExchVal(HiHook *h, DWORD this_, int crId, __int16 qty, __int16 *slotPtr,
                                         char mustReplaceCreature)
{
    const int result = CALL_5(int, __thiscall, h->GetDefaultFunc(), this_, crId, qty, slotPtr, mustReplaceCreature);
    return result < 0 ? 0 : result;
}

// фикс вылета: АИ битва (просчёт)
// проверка на скорость монстра и когда он дойдет до защиты стрелка.
// Убираем из проверки существ с нулевой скоростью и боевые машины
_int_ __stdcall Y_AIMgr_Stack_MinRoundToReachHex(HiHook *hook, _dword_ this_, _BattleStack_ *stack, _int_ a3)
{
    if (stack->creature.flags == BCF_CANT_MOVE || stack->creature.speed < 1)
    {
        return 99; // 99 раундов необходимо, чтобы добраться до стрелка
    }

    return CALL_3(_int_, __thiscall, hook->GetDefaultFunc(), this_, stack, a3);
}

// © daemon_n
// фикс бага WoG -- для ИИ не был добавлен рассчёт кавалерийского бонуса для новых существ и SE:
_LHF_(FixAI_CavalryBonus) // 0x0436200
{
    const _BattleStack_ *atkStack =
        reinterpret_cast<_BattleStack_ *>(c->ecx); // поместим нападающий стек в ebx для ф-ции ниже
    if (atkStack->creature.flyer)
    {
        return EXEC_DEFAULT;
    }
    const _BattleStack_ *defStack =
        *reinterpret_cast<_BattleStack_ **>(c->esi + 8); // поместим стек защитника в edi для ф-ции ниже

    int result = atkStack->creature_id;
    __asm {
        mov ebx, atkStack // attacker
        mov edi, defStack // defender
        mov eax, 0x075D7F5 // вызов ф-ции вог, на случай, если кто хочет её хукнуть
        call eax
        mov result, eax
    }
    c->ecx = result;               // во время возврата должен быть уже id существа, а не stack*
    c->return_address = 0x0436208; // возвращаем "Чемпиона" для последующей проверки
    return NO_EXEC_DEFAULT;
}
// У героя есть артефакты, позволяющие игнорировать препятствия при стрельбе.
_bool_ hero_has_shoot_obst_arts;

// Герой врага.
_Hero_ *AI_VB_SetStacks_EnemyHero;

// Коэффициент ценности.
double AI_VB_SetStacks_val_coeff;

// При расчёте ИИ-ИИ битвы при нападении защищающегося исправляем параметры вычисления урона.
int __stdcall LoHook_AICalcBattle_DefAtt_CalcDamageVal(LoHook *h, HookContext *c)
{
    // Защищающийся бьёт не всеми, но его стрелки не заблокированны.
    c->eax = CALL_3(_int32_, __thiscall, 0x426390, c->edi, IntAt(c->ebp - 8), FALSE);

    // Пропускаем стандартное вычисление.
    c->return_address = 0x426D22;

    return NO_EXEC_DEFAULT;
}
// При расчёте ИИ-ИИ битвы при нападении защищающегося исправляем параметры нанесения урона.
int __stdcall LoHook_AICalcBattle_DefAtt_MakeDamage(LoHook *h, HookContext *c)
{
    // Защищающийся получает урон только по добежавшим.
    c->eax = CALL_4(_int32_, __thiscall, 0x426170, c->edi, c->eax, 1, IntAt(c->ebp - 8));

    // Пропускаем стандартное нанесение.
    c->return_address = 0x426D57;

    return NO_EXEC_DEFAULT;
}
// Перед настройкой стеков при расчёте битвы инициализируем некоторые значения.
void __stdcall HiHook_AICalcBattle_SetStacks(HiHook *h, _Struct_ *this_, double val_coeff, _Hero_ *enemy_hero)
{
    // Артефакты, позволяющие игнорировать препятствия при стрельбе.
    hero_has_shoot_obst_arts =
        this_->Field<_Hero_ *>(36) && (this_->Field<_Hero_ *>(36)->DoesWearArtifact(AID_GOLDEN_BOW) ||
                                       this_->Field<_Hero_ *>(36)->DoesWearArtifact(AID_BOW_OF_THE_SHARPSHOOTER));

    AI_VB_SetStacks_EnemyHero = enemy_hero;
    AI_VB_SetStacks_val_coeff = val_coeff;

    CALL_4(void, __thiscall, h->GetDefaultFunc(), this_, *(_dword_ *)&val_coeff, *((_dword_ *)&val_coeff + 1),
           enemy_hero);
}
// При расчёте ИИ-ИИ битвы при взятии модификатора стрелковой атаки учитываем и собственные 100% урона стека (баг SoD).
_float_ __stdcall HiHook_AICalcBattle_GetHeroShootingModif(HiHook *h, _Hero_ *this_)
{
    // Добавляем 1.0.
    return 1.0f + CALL_1(_float_, __thiscall, h->GetDefaultFunc(), this_);
}

// Учитываем всех стреляющих сквозь препятствия стрелков (а не только архимагов), а так же соответствующие артефакты.
int __stdcall LoHook_AICalcBattle_Check_ObstacklesShooters(LoHook *h, HookContext *c)
{
    const int monId = IntAt(c->ebp - 4);
    if (hero_has_shoot_obst_arts)
    {
        c->return_address = 0x424429;
        return NO_EXEC_DEFAULT;
    }
    else
    {
        switch (monId)
        {
        case CID_MAGE:
        case CID_ENCHANTER:
        case CID_SHARPSHOOTER:
        case CID_ARROW_TOWER:
        case CID_ARCTIC_SHARPSHOOTER:
        case CID_LAVA_SHARPSHOOTER:
            c->return_address = 0x424429;
            return NO_EXEC_DEFAULT;
        default:
            break;
        }
    }

    return EXEC_DEFAULT;
}

// Исправляем плохой учёт ИИ нейтралов при рассчёте боевого духа с учётом Альянса Ангелов (1).
int __stdcall LoHook_FixAngelicAllianceAI1(LoHook *h, HookContext *c)
{
    // Если существо - нейтрал, пропускаем взятие бита из маски альянса.
    if (c->esi == -1)
    {
        // Восстанавливаем затёртую команду.
        c->edi = c->eax;

        // Стек не подвержен альянсу.
        c->return_address = 0x42C7C3;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Исправляем плохой учёт ИИ нейтралов при рассчёте боевого духа с учётом Альянса Ангелов (2).
int __stdcall LoHook_FixAngelicAllianceAI2(LoHook *h, HookContext *c)
{
    // Если существо - нейтрал, пропускаем взятие бита из маски альянса.
    if (c->edi == -1)
    {
        // Восстанавливаем затёртую команду.
        c->ebx = c->eax;

        // Стек не подвержен альянсу.
        c->return_address = 0x42C8F9;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void AIFixes(PatcherInstance *_PI)
{
    // © Raistlin
    // Исправляем косяк, из-за которого ИИ не мог использовать торговцев артефактами в Сопряжении
    _PI->WriteLoHook(0x525ED1, R_FixAI_ConfluxNerchant);
    // Фикс бесконечного цикла ИИ: когда он оценивает ценность получения стека обнуляем отрицательное значение, чтобы
    // избежать переполнения.
    _PI->WriteHiHook(0x42c830, SPLICE_, EXTENDED_, THISCALL_, Fix_AI_GetBestStackExchVal);

    // AI битва (просчёт)
    // проверка на скорость монстра и когда он дойдет до защиты стрелка.
    // Убираем из проверки существ с нулевой скоростью и боевые машины
    _PI->WriteHiHook(0x4B3C80, SPLICE_, EXTENDED_, THISCALL_, Y_AIMgr_Stack_MinRoundToReachHex);

    // AI БИТВА
    // AI всегда будет получать артефакты и опыт за побеждённого врага
    _PI->WriteByte(0x426F41, 0);

    // Исправление бага SoD с передачей артефактов побеждённому компьютеру.
    _PI->WriteHexPatch(0x426FA1 + 1, "CA"); // (mov ecx,) edx
    _PI->WriteHexPatch(0x426FA3, "53");     // push ebx

    // © JackSlater
    // Фикс бага WoG - Драколичи не имели флаг сплеша для ИИ
    o_CreatureInfo[CID_DRACOLICH].fireballAttack = true; // !#DC(MON_FLAG_SPLASH_SHOOTER) = 1048576;
    // Исправляем баг SoD: ИИ считал, что облако личей не задевает только нежить.
    _PI->WriteByte(0x41EFA6 + 2, 4);
    _PI->WriteHexPatch(0x41EFAC, "75 16"); // jnz 0x41EFC4

    // Снимаем ограничение для ИИ в 8 героев на команду
    // _PI->WriteHexPatch(0x431392, "EB");

    // © daemon_n
    // фикс бага WoG -- для ИИ не был добавлен рассчёт кавалерийского бонуса для новых существ и SE:
    _PI->WriteLoHook(0x0436200, FixAI_CavalryBonus);

    // Исправление багов ИИ-ИИ битвы.

    // При расчёте ИИ-ИИ битвы при нападении защищающегося исправляем параметры вычисления урона.
    _PI->WriteLoHook(0x426D17, LoHook_AICalcBattle_DefAtt_CalcDamageVal);

    // При расчёте ИИ-ИИ битвы при нападении защищающегося исправляем параметры нанесения урона.
    _PI->WriteLoHook(0x426D4B, LoHook_AICalcBattle_DefAtt_MakeDamage);

    // Перед настройкой стеков при расчёте битвы инициализируем некоторые значения.
    _PI->WriteHiHook(0x424120, SPLICE_, EXTENDED_, THISCALL_, HiHook_AICalcBattle_SetStacks);

    // При расчёте ИИ-ИИ битвы при взятии модификатора стрелковой атаки учитываем и собственные 100% урона стека (баг
    // SoD).
    _PI->WriteHiHook(0x42426D, CALL_, EXTENDED_, THISCALL_, HiHook_AICalcBattle_GetHeroShootingModif);

    // Учитываем всех стреляющих сквозь препятствия стрелков (а не только архимагов).
    _PI->WriteLoHook(0x424412, LoHook_AICalcBattle_Check_ObstacklesShooters);

    // временно отключаем проверку на каст заклинаний для ИИ в теоретических битвах
    _PI->WriteByte(0x0423F47 + 3, 0);

    return;
    //// + Исправляем плохой учёт ИИ нейтралов при рассчёте боевого духа с учётом Альянса Ангелов (баг SoD).
    //_PI->WriteLoHook(0x42C778, LoHook_FixAngelicAllianceAI1);
    //_PI->WriteLoHook(0x42C8AD, LoHook_FixAngelicAllianceAI2);
}

} // namespace AI
