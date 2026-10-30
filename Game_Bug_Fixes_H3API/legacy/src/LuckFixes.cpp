namespace luck
{
// Перерасчёт морали и удачи отрядов сразу после каста массовых заклинаний артефактами в начале битвы
_LHF_(Gem_OnAfterArtSpellCasting)
{
    o_BattleMgr->RecalculateLuckAndMorale();
    return EXEC_DEFAULT;
}
char __stdcall HiHook_00441330(HiHook *h, _BattleStack_ *attacker, _BattleStack_ *target, int direction)
{
    char result = CALL_3(char, __thiscall, h->GetDefaultFunc(), attacker, target, direction);
    attacker->isLucky = 0;
    return result;
}

void PlayLuckOrUnluckAttack(_BattleStack_ *attacker)
{
    int luck = attacker->luck;
    if (!luck)
        return;

    if (luck < 0)
    {
        const int min_value = IntAt(0x44B145);
        if (luck < min_value)
        {
            luck = min_value;
        }

        if (Randint(1, 12) <= -luck)
        {
            attacker->isLucky = -1;
            // Проигрываем анимацию неудачи
            if (!o_BattleMgr->IsHiddenBattle())
            {
                Sound_Play_Wav("badluck.82m");
                char *crName = CALL_2(char *, __fastcall, 0x43FE20, attacker->creature_id, attacker->count_current);
                sprintf_s(o_TextBuffer, 0x300u, o_GENRLTXT_TXT->GetString(46), crName);
                o_BattleMgr->AddStatusMessage(o_TextBuffer);
                attacker->PlayMagicAnimation(48);
            }
        }
    }
    else
    {
        const int max_value = IntAt(0x44B13B);
        if (luck > max_value)
        {
            luck = max_value;
        }

        if (Randint(1, 24) <= luck)
        {
            attacker->isLucky = 1;
            // Проигрываем анимацию удачи
            if (!o_BattleMgr->IsHiddenBattle())
            {
                Sound_Play_Wav("goodluck.82m");
                char *crName = CALL_2(char *, __fastcall, 0x43FE20, attacker->creature_id, attacker->count_current);

                sprintf_s(o_TextBuffer, 0x300u, o_GENRLTXT_TXT->GetString(47), crName);
                o_BattleMgr->AddStatusMessage(o_TextBuffer);
                attacker->PlayMagicAnimation(18);
            }
        }
    }
}

_LHF_(LoHook_0044151D)
{
    PlayLuckOrUnluckAttack(reinterpret_cast<_BattleStack_ *>(c->esi));
    c->return_address = 0x4415DE;
    return NO_EXEC_DEFAULT;
}

void __stdcall HiHook_0043F620(HiHook *h, _BattleStack_ *shooter, _BattleStack_ *target)
{
    CALL_2(void, __thiscall, h->GetDefaultFunc(), shooter, target);
    shooter->isLucky = 0;
}

_LHF_(LoHook_0043F63B)
{
    PlayLuckOrUnluckAttack(reinterpret_cast<_BattleStack_ *>(c->esi));
    c->return_address = 0x43F6F6;
    return NO_EXEC_DEFAULT;
}

// удача увеличивает весь урон в 2 раза
_LHF_(LoHook_00443CA3)
{
    if (reinterpret_cast<_BattleStack_ *>(c->ebx)->isLucky > 0)
    {
        c->eax <<= 1;
    }

    return EXEC_DEFAULT;
}

// делаем так, чтобы при неудаче урона было в 2 раза меньше
double __stdcall HiHook_004438B0(HiHook *h, _BattleStack_ *attacker, _BattleStack_ *target, char shoot)
{
    double result = CALL_3(double, __thiscall, h->GetDefaultFunc(), attacker, target, shoot);
    if (attacker->isLucky == -1)
    {
        result /= 2;
    }

    return result;
}

// Есть ли у кого-то песочные часы недоброго часа.
_bool_ HasHourglass;

// Перед получением списка модификаторов удачи определяем наличие песочных часов недброго часа.
_HStringF_ *__stdcall HiHook_Get_CreatureLuckMod_List(HiHook *h, _Army_ *own_army, _HStringF_ *out_str,
                                                      _int32_ creature_id, _int32_ luck_level, _Hero_ *hero,
                                                      _Town_ *town, _Hero_ *enemy_hero, _Army_ *enemy_army,
                                                      _int32_ subterr_id)
{
    // Определяем, есть ли песочные часы недоброго часа.
    HasHourglass = ((hero && hero->DoesWearArtifact(AID_HOURGLASS_OF_THE_EVIL_HOUR)) ||
                    (enemy_hero && enemy_hero->DoesWearArtifact(AID_HOURGLASS_OF_THE_EVIL_HOUR)));

    // Вызываем функцию.
    return CALL_9(_HStringF_ *, __thiscall, h->GetDefaultFunc(), own_army, out_str, creature_id, luck_level, hero, town,
                  enemy_hero, enemy_army, subterr_id);
}
// Учитываем песочные часы недоброго часа как модификатор удачи существа в списке модификаторов.
int __stdcall LoHook_CreatureLuckModifsList_Hourglass(LoHook *h, HookContext *c)
{

    // Если удача положительна и есть песочные часы недоброго часа.
    if (IntAt(c->ebp + 20) > 0 && HasHourglass)
    {
        // Удаляем созданную предыдущими модификаторами строку.
        ((_HStringF_ *)(c->ebp - 32))->Clear(TRUE);

        // Обнуляем удачу.
        IntAt(c->ebp + 20) = 0;

        // Переходим к копированию текста часов.
        c->return_address = 0x44BF18;

        return NO_EXEC_DEFAULT;
    }
    // Часы не влияют - всё обычно.
    else
    {
        return EXEC_DEFAULT;
    }
}
// Учитываем песочные часы недоброго часа как модификатор удачи существа.
int __stdcall LoHook_CreatureLuck_Hourglass(LoHook *h, HookContext *c)
{
    // Герой.
    _Hero_ *hero = *(_Hero_ **)(c->ebp + 12);

    // Если удача положительна и есть песочные часы недоброго часа.
    if (c->eax > 0 && hero && hero->DoesWearArtifact(AID_HOURGLASS_OF_THE_EVIL_HOUR))
    {
        // Обнуляем удачу.
        c->eax = 0;
    }

    return EXEC_DEFAULT;
}
// Учитываем песочные часы недоброго часа как модификатор удачи существа при установке удачи стека.
int __stdcall LoHook_BattleStackSetLuck_Hourglass(LoHook *h, HookContext *c)
{
    // Герой.
    _Hero_ *hero = *(_Hero_ **)(c->ebp + 8);

    // Вражеский герой.
    _Hero_ *enemy_hero = *(_Hero_ **)(c->ebp + 20);

    // Учитываем только положительную удачу хоббитов.
    if (((_BattleStack_ *)c->edi)->creature_id == CID_HALFLING)
    {
        if (c->esi < 1)
            c->esi = 1;
    }

    // Если удача положительна и есть песочные часы недоброго часа.
    if (c->esi > 0 && (hero && hero->DoesWearArtifact(AID_HOURGLASS_OF_THE_EVIL_HOUR) ||
                       enemy_hero && enemy_hero->DoesWearArtifact(AID_HOURGLASS_OF_THE_EVIL_HOUR)))
    {
        // Обнуляем удачу.
        c->esi = 0;
    }

    // Переходим к установке удачи.
    if (c->esi == 1)
        c->return_address = 0x43DCED;
    else
        c->return_address = 0x43DCFF;

    return NO_EXEC_DEFAULT;
}

void LuckFixes(PatcherInstance *_PI)
{

    // Перерасчёт морали и удачи отрядов сразу после каста массовых заклинаний артефактами в начале битвы
    _PI->WriteLoHook(0x4650BF, Gem_OnAfterArtSpellCasting);
    // Перерасчёт морали и удачи при открытии диалога существа
    _PI->WriteLoHook(0x5F37BE, Gem_OnAfterArtSpellCasting);
    // _BattleStack_::MeleeAtack (00441330)

    // Обнуляем удачу атакующего после атаки
    _PI->WriteHiHook(0x441330, SPLICE_, EXTENDED_, THISCALL_, HiHook_00441330);

    // Рассчитываем и проигрываем удачное/неудачное милли
    _PI->WriteLoHook(0x44151D, LoHook_0044151D);

    // Нопим обнуление удачи атакующего
    _PI->WriteCodePatch(0x441792, "%n", 7);

    // _BattleStack_::Shoot (0043F620)

    // Обнуляем удачу стрелка после выстрела
    _PI->WriteHiHook(0x43F620, SPLICE_, EXTENDED_, THISCALL_, HiHook_0043F620);

    // Рассчитываем и проигрываем удачный/неудачный выстрел
    _PI->WriteLoHook(0x43F63B, LoHook_0043F63B);

    // Выстрел магога - нопим обнуление удачи стрелка
    _PI->WriteCodePatch(0x43F963, "%n", 7);

    // Обычный выстрел - нопим обнуление удачи стрелка
    _PI->WriteCodePatch(0x43FA66, "%n", 7);

    // Выстрел лича - нопим обнуление удачи стрелка
    _PI->WriteCodePatch(0x43FD45, "%n", 7);

    // _BattleStack_::DoMeleeAtack (004419D0)

    // _BattleStack_::AoeMeleeAtack (00440030)

    // Затираем атакующему стеку isLucky = 0 - удача/неудача при AoE атаке работает на все цели
    _PI->WriteCodePatch(0x4400E7, "%n", 7);

    // _BattleStack_::CalcDamageBonuses (00443040)

    // Нопим + базовый урон, если стек isLucky
    _PI->WriteCodePatch(0x4430A3, "%n", 18);

    // _BattleStack_::ApplyDamageModifiers (00443C60)

    // Double dmg при удаче на все модификаторы
    _PI->WriteLoHook(0x443CA3, LoHook_00443CA3);

    // _BattleStack_::CalcDamagePenalties (004438B0)

    // Снижаем общий урон в 2 раза если attacker->isLucky = -1
    _PI->WriteHiHook(0x4438B0, SPLICE_, EXTENDED_, THISCALL_, HiHook_004438B0);

    // Песочые часы недоброго часа влияют только на положительную на удачу

    // Песочные часы недоброго часа отнимают только положительную удачу.

    // Не проверяем песочные часы непосредственно у героя.
    _PI->WriteCodePatch(0x4E393B, "%n", 10); // 10 nop
    _PI->WriteHexPatch(0x4E3945, "90 E9");   // jmp ...

    // Песочные часы недоброго часа не влияют на удачу самого героя.
    _PI->WriteCodePatch(0x44AFB7, "%n", 17); // 17 nop
    _PI->WriteCodePatch(0x44AFCB, "%n", 15); // 15 nop
    _PI->WriteHexPatch(0x44AFDA, "EB");      // jmp ...

    // Учитываем песочные часы недоброго часа как модификатор удачи существа.
    _PI->WriteLoHook(0x44B12E, LoHook_CreatureLuck_Hourglass);

    // Стираем оригинальный учёт песочных часов недоброго часа при установке удачи стека.
    _PI->WriteCodePatch(0x43DC56, "%n", 22); // 22 nop
    _PI->WriteCodePatch(0x43DC6F, "%n", 17); // 17 nop

    // Учитываем песочные часы недоброго часа как модификатор удачи существа при установке удачи стека.
    _PI->WriteLoHook(0x43DCDF, LoHook_BattleStackSetLuck_Hourglass);

    // Стираем оригинальный учёт песочных часов недоброго часа при получении списка модификаторов удачи стека.
    _PI->WriteCodePatch(0x44BEF3, "%n", 17); // 17 nop
    _PI->WriteCodePatch(0x44BF07, "%n", 15); // 15 nop
    _PI->WriteHexPatch(0x44BF16, "EB");      // jmp ...

    // Список модификаторов удачи стека: вместо уменьшения предполагаемой удачи увеличиваем реальную.
    _PI->WriteByte(0x44BFF6 + 2, 20);
    _PI->WriteHexPatch(0x44BFFF + 1, "C1"); // add ecx, ...
    _PI->WriteByte(0x44C004 + 2, 20);

    // Перед получением списка модификаторов удачи определяем наличие песочных часов недброго часа.
    _PI->WriteHiHook(0x44BE90, SPLICE_, EXTENDED_, THISCALL_, HiHook_Get_CreatureLuckMod_List);

    // Учитываем песочные часы недоброго часа как модификатор удачи существа в списке модификаторов.
    _PI->WriteLoHook(0x44C18A, LoHook_CreatureLuckModifsList_Hourglass);

    //// Корректное описание модификатора удачи хоббитов.
    //_PI->WriteDword(0x44C156 + 1, misk_text[2]);

    // Недостающие фиксы удачи
    _PI->WriteByte(0x4382b0, 0x1);
    _PI->WriteCodePatch(0x4383e2, "%n", 3);
    _PI->WriteByte(0x5f4309, 0x0);

    // ИИ
    // Правка ценности ИИ неудачи (увеличиваем в 2 раза).
    static _float_ f0_0244 = -0.0244f;
    _PI->WriteDword(0x4355B0 + 2, (_ptr_)&f0_0244);
    _PI->WriteDword(0x438C26 + 2, (_ptr_)&f0_0244);

    // Исключаем неверный расчёт ИИ удачи героя при оценке ценности повышения удачи.

    // Лебединое озеро.
    _PI->WriteCodePatch(0x528A95, "%n", 17); // 17 nop

    // Домик фей.
    _PI->WriteCodePatch(0x528D93, "%n", 17); // 17 nop

    // Русалки, ящик пандоры, хижина провидца.
    _PI->WriteCodePatch(0x528277, "%n", 15); // 15 nop
    _PI->WriteCodePatch(0x528288, "%n", 2);  // 2 nop
}
} // namespace luck
