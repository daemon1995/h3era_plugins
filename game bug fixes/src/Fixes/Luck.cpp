#include "../../include/Plugin.h"
#include <cstdio>
// WOG's luck-modifier buffer is NOT an H3String, and it is not substituted
// with H3String; this layout is defined only for the original WoG ABI.
#pragma pack(push, 1)
struct WogLuckString
{
    BYTE owned;
    BYTE reserved[3];
    char *data;
    int length;
    int capacity;
    void Clear(bool reset)
    {
        THISCALL_2(void, 0x404130, this, reset);
    }
};
#pragma pack(pop)
static_assert(alignof(WogLuckString) == 1, "WoG string packed layout");
static_assert(offsetof(WogLuckString, data) == 4, "WoG string data offset");
static_assert(offsetof(WogLuckString, length) == 8, "WoG string length offset");
static_assert(offsetof(WogLuckString, capacity) == 12, "WoG string capacity offset");
static_assert(sizeof(WogLuckString) == 16, "WoG luck string ABI mismatch");
namespace gbf
{
namespace luck
{
// Перерасчёт морали и удачи отрядов сразу после каста массовых заклинаний артефактами в начале битвы
_LHF_(Gem_OnAfterArtSpellCasting)
{
    THISCALL_1(int, 0x469330, h3::H3CombatManager::Get());
    return EXEC_DEFAULT;
}
char __stdcall HiHook_00441330(HiHook *h, h3::H3CombatCreature *attacker, h3::H3CombatCreature *target, int direction)
{
    char result = THISCALL_3(char, h->GetDefaultFunc(), attacker, target, direction);
    attacker->isLucky = 0;
    return result;
}

void PlayLuckOrUnluckAttack(h3::H3CombatCreature *attacker)
{
    int luck = attacker->luck;
    if (!luck)
        return;

    if (luck < 0)
    {
        const int min_value = h3::IntAt(0x44B145);
        if (luck < min_value)
        {
            luck = min_value;
        }

        if (FASTCALL_2(int, 0x50C7C0, 1, 12) <= -luck)
        {
            attacker->isLucky = -1;
            // Проигрываем анимацию неудачи
            if (!h3::H3CombatManager::Get()->IsHiddenBattle())
            {
                FASTCALL_3(BYTE, 0x59A890, "badluck.82m", -1, 3);
                char *crName = FASTCALL_2(char *, 0x43FE20, attacker->type, attacker->numberAlive);
                sprintf_s(reinterpret_cast<char *>(0x697428), 0x300u, h3::H3GeneralText::Get()->GetText(46), crName);
                h3::H3CombatManager::Get()->AddStatusMessage(reinterpret_cast<const char *>(0x697428));
                h3::H3CombatManager::Get()->PlayMagicAnimation(48, attacker, 100, false);
            }
        }
    }
    else
    {
        const int max_value = h3::IntAt(0x44B13B);
        if (luck > max_value)
        {
            luck = max_value;
        }

        if (FASTCALL_2(int, 0x50C7C0, 1, 24) <= luck)
        {
            attacker->isLucky = 1;
            // Проигрываем анимацию удачи
            if (!h3::H3CombatManager::Get()->IsHiddenBattle())
            {
                FASTCALL_3(BYTE, 0x59A890, "goodluck.82m", -1, 3);
                char *crName = FASTCALL_2(char *, 0x43FE20, attacker->type, attacker->numberAlive);

                sprintf_s(reinterpret_cast<char *>(0x697428), 0x300u, h3::H3GeneralText::Get()->GetText(47), crName);
                h3::H3CombatManager::Get()->AddStatusMessage(reinterpret_cast<const char *>(0x697428));
                h3::H3CombatManager::Get()->PlayMagicAnimation(18, attacker, 100, false);
            }
        }
    }
}

_LHF_(LoHook_0044151D)
{
    PlayLuckOrUnluckAttack(reinterpret_cast<h3::H3CombatCreature *>(c->esi));
    c->return_address = 0x4415DE;
    return NO_EXEC_DEFAULT;
}

void __stdcall HiHook_0043F620(HiHook *h, h3::H3CombatCreature *shooter, h3::H3CombatCreature *target)
{
    THISCALL_2(void, h->GetDefaultFunc(), shooter, target);
    shooter->isLucky = 0;
}

_LHF_(LoHook_0043F63B)
{
    PlayLuckOrUnluckAttack(reinterpret_cast<h3::H3CombatCreature *>(c->esi));
    c->return_address = 0x43F6F6;
    return NO_EXEC_DEFAULT;
}

// удача увеличивает весь урон в 2 раза
_LHF_(LoHook_00443CA3)
{
    if (reinterpret_cast<h3::H3CombatCreature *>(c->ebx)->isLucky > 0)
    {
        c->eax <<= 1;
    }

    return EXEC_DEFAULT;
}

// делаем так, чтобы при неудаче урона было в 2 раза меньше
double __stdcall HiHook_004438B0(HiHook *h, h3::H3CombatCreature *attacker, h3::H3CombatCreature *target, char shoot)
{
    double result = THISCALL_3(double, h->GetDefaultFunc(), attacker, target, shoot);
    if (attacker->isLucky == -1)
    {
        result /= 2;
    }

    return result;
}

// Есть ли у кого-то песочные часы недоброго часа.
bool HasHourglass;

// Перед получением списка модификаторов удачи определяем наличие песочных часов недброго часа.
WogLuckString *__stdcall HiHook_Get_CreatureLuckMod_List(HiHook *h, h3::H3Army *own_army, WogLuckString *out_str,
                                                         int creature_id, int luck_level, h3::H3Hero *hero,
                                                         h3::H3Town *town, h3::H3Hero *enemy_hero,
                                                         h3::H3Army *enemy_army, int subterr_id)
{
    // Определяем, есть ли песочные часы недоброго часа.
    HasHourglass = ((hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR)) ||
                    (enemy_hero && enemy_hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR)));

    // Вызываем функцию.
    return THISCALL_9(WogLuckString *, h->GetDefaultFunc(), own_army, out_str, creature_id, luck_level, hero, town,
                      enemy_hero, enemy_army, subterr_id);
}
// Учитываем песочные часы недоброго часа как модификатор удачи существа в списке модификаторов.
int __stdcall LoHook_CreatureLuckModifsList_Hourglass(LoHook *h, HookContext *c)
{

    // Если удача положительна и есть песочные часы недоброго часа.
    if (h3::IntAt(c->ebp + 20) > 0 && HasHourglass)
    {
        // Удаляем созданную предыдущими модификаторами строку.
        ((WogLuckString *)(c->ebp - 32))->Clear(TRUE);

        // Обнуляем удачу.
        h3::IntAt(c->ebp + 20) = 0;

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
    h3::H3Hero *hero = *(h3::H3Hero **)(c->ebp + 12);

    // Если удача положительна и есть песочные часы недоброго часа.
    if (c->eax > 0 && hero && hero->WearsArtifact(85))
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
    h3::H3Hero *hero = *(h3::H3Hero **)(c->ebp + 8);

    // Вражеский герой.
    h3::H3Hero *enemy_hero = *(h3::H3Hero **)(c->ebp + 20);

    // Учитываем только положительную удачу хоббитов.
    if (((h3::H3CombatCreature *)c->edi)->type == 138)
    {
        if (c->esi < 1)
            c->esi = 1;
    }

    // Если удача положительна и есть песочные часы недоброго часа.
    if (c->esi > 0 && (hero && hero->WearsArtifact(85) || enemy_hero && enemy_hero->WearsArtifact(85)))
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

void Install(PatcherInstance *_PI)
{

    // Перерасчёт морали и удачи отрядов сразу после каста массовых заклинаний артефактами в начале битвы
    _PI->WriteLoHook(0x4650BF, Gem_OnAfterArtSpellCasting);
    // Перерасчёт морали и удачи при открытии диалога существа
    _PI->WriteLoHook(0x5F37BE, Gem_OnAfterArtSpellCasting);
    // h3::H3CombatCreature::MeleeAtack (00441330)

    // Обнуляем удачу атакующего после атаки
    // _BattleStack_::MeleeAtack (00441330)
    _PI->WriteHiHook(0x441330, SPLICE_, EXTENDED_, THISCALL_, HiHook_00441330);

    // Рассчитываем и проигрываем удачное/неудачное милли
    _PI->WriteLoHook(0x44151D, LoHook_0044151D);

    // Нопим обнуление удачи атакующего
    _PI->WriteCodePatch(0x441792, "%n", 7);

    // h3::H3CombatCreature::Shoot (0043F620)

    // Обнуляем удачу стрелка после выстрела
    // _BattleStack_::Shoot (0043F620)
    _PI->WriteHiHook(0x43F620, SPLICE_, EXTENDED_, THISCALL_, HiHook_0043F620);

    // Рассчитываем и проигрываем удачный/неудачный выстрел
    _PI->WriteLoHook(0x43F63B, LoHook_0043F63B);

    // Выстрел магога - нопим обнуление удачи стрелка
    _PI->WriteCodePatch(0x43F963, "%n", 7);

    // Обычный выстрел - нопим обнуление удачи стрелка
    _PI->WriteCodePatch(0x43FA66, "%n", 7);

    // Выстрел лича - нопим обнуление удачи стрелка
    _PI->WriteCodePatch(0x43FD45, "%n", 7);

    // h3::H3CombatCreature::DoMeleeAtack (004419D0)

    // h3::H3CombatCreature::AoeMeleeAtack (00440030)

    // Затираем атакующему стеку isLucky = 0 - удача/неудача при AoE атаке работает на все цели
    // _BattleStack_::DoMeleeAtack (004419D0)
    // _BattleStack_::AoeMeleeAtack (00440030)
    _PI->WriteCodePatch(0x4400E7, "%n", 7);

    // h3::H3CombatCreature::CalcDamageBonuses (00443040)

    // Нопим + базовый урон, если стек isLucky
    // _BattleStack_::CalcDamageBonuses (00443040)
    _PI->WriteCodePatch(0x4430A3, "%n", 18);

    // h3::H3CombatCreature::ApplyDamageModifiers (00443C60)

    // Double dmg при удаче на все модификаторы
    // _BattleStack_::ApplyDamageModifiers (00443C60)
    _PI->WriteLoHook(0x443CA3, LoHook_00443CA3);

    // h3::H3CombatCreature::CalcDamagePenalties (004438B0)

    // Снижаем общий урон в 2 раза если attacker->isLucky = -1
    // _BattleStack_::CalcDamagePenalties (004438B0)
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
    static float f0_0244 = -0.0244f;
    _PI->WriteDword(0x4355B0 + 2, reinterpret_cast<DWORD>(&f0_0244));
    _PI->WriteDword(0x438C26 + 2, reinterpret_cast<DWORD>(&f0_0244));

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
void InstallLuck(PatcherInstance *pi)
{
    luck::Install(pi);
}
} // namespace gbf
