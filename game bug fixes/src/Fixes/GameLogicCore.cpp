#include "../../include/Plugin.h"
#include <cstddef>

namespace gbf
{
namespace game_logic_core
{
namespace
{
static_assert(sizeof(h3::H3CombatCreature) == 0x548, "Combat creature layout");
static_assert(offsetof(h3::H3CombatCreature, numberAlive) == 0x4C, "Alive count offset");

// Match the original GameLogic's signed ten-bit coordinates and four-bit z.
DWORD __cdecl PackWoGCoordinates(int x, int y, int z)
{
    return (DWORD(x) & 0x3FFu) | ((DWORD(y) & 0x3FFu) << 16) | ((DWORD(z) & 0xFu) << 26);
}

int SignExtend10(DWORD value)
{
    const int coordinate = static_cast<int>(value & 0x3FFu);
    return coordinate >= 0x200 ? coordinate - 0x400 : coordinate;
}

void __cdecl UnpackWoGCoordinates(int *x, int *y, int *z, DWORD packed)
{
    *x = SignExtend10(packed);
    *y = SignExtend10(packed >> 16);
    const int level = static_cast<int>((packed >> 26) & 0xFu);
    *z = level >= 8 ? level - 16 : level;
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

bool __cdecl WoG_OnMapRestartCrtraitsReset(const BOOL isWogMap)
{
    return 1;
}

// © daemon_n
// Добавить мифрил игроку при вводе чит-кода на ресурсы
int __stdcall AdvMgr_ApplyResourceCheat(LoHook *, HookContext *c)
{
    // H3API: ownerID at +0 is the original _Player_::id.
    const auto *player = h3::H3ActivePlayer::Get();
    if (player && player->ownerID >= 0 && player->ownerID < 8)
        IntAt(0x27F9A00 + static_cast<int>(player->ownerID) * 4) += 100;
    return EXEC_DEFAULT;
}

// убираем клонов из показа в диалоге результатов битвы
int __stdcall Y_Dlg_BattleResults_IgnoreClones(LoHook *, HookContext *c)
{
    if (c->eax < 96)
        return EXEC_DEFAULT;
    h3::H3CombatCreature *stack = reinterpret_cast<h3::H3CombatCreature *>(c->eax - 96);

    if (stack->info.clone)
    {
        c->return_address = 0x470958;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// фикс бага расчёта защиты с возвратом 0 (чаще всего при атаке медведями существ с низкой защитой)
int __stdcall WoG_BattleStack_GetDefenceAgainst(LoHook *, HookContext *c)
{
    // поместить сразу в переменную возврата результат выполнения оригинальной ф-ции
    // вот если бы не Ghost Behemoth, проблема бы решалась в 2 счёта (байта, кек)

    IntAt(0x2832700) = c->eax;
    return EXEC_DEFAULT;
}

// Ошибка WoG/ERM -- помещение банка существ не типа объекта 16 не инициализировала его как банк существ
int __stdcall WoG_PlaceObject(LoHook *, HookContext *c)
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

// фикс SoD бага, который не отмечает посещённосто университа игроком
int __stdcall AdvMgr_EnterToUniversity(LoHook *, HookContext *c)
{
    const auto hero = *reinterpret_cast<h3::H3Hero **>(c->ebp + 0x8);
    if (hero && hero->owner >= 0 && hero->owner < 8)
    {
        THISCALL_2(void, 0x4FC620, c->edi, hero->owner);
    }
    return EXEC_DEFAULT;
}

// © JackSlater
// Прыжок через ProcessMessagesForTime, если скрытая битва
int __stdcall LoHook_00441B25(LoHook *, HookContext *c)
{
    if (!h3::H3CombatManager::Get()->IsHiddenBattle())
    {
        return EXEC_DEFAULT;
    }

    c->return_address = 0x441B43;
    return NO_EXEC_DEFAULT;
}

int __stdcall LoHook_00441BD6(LoHook *, HookContext *c)
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
int __stdcall LoHook_00441982(LoHook *, HookContext *c)
{
    if (reinterpret_cast<h3::H3CombatCreature *>(c->esi)->numberAlive > 0)
    {
        return EXEC_DEFAULT;
    }
    c->return_address = 0x4419A5;
    return NO_EXEC_DEFAULT;
}

// Баг: перед контратакой - проверка на count_current цели
int __stdcall LoHook_00441AFF(LoHook *, HookContext *c)
{

    if (reinterpret_cast<h3::H3CombatCreature *>(c->edi)->numberAlive > 0)
    {
        return EXEC_DEFAULT;
    }
    c->return_address = 0x441B85;
    return NO_EXEC_DEFAULT;
}

// Исправление копейщиков в лагерях беженцев (только для клетки с триггером).
int __stdcall LoHook_FixRefugeeCamp_dx(LoHook *, HookContext *c)
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

int __stdcall AdvManager_KeyPress(LoHook *, HookContext *c)
{
    THISCALL_2(void, 0x47F0F0, h3::H3AdventureManager::Get(), 1); // H3AdventureManager::StopCursor
    return EXEC_DEFAULT;
}
} // namespace
void Install(PatcherInstance *pi)
{
    // исправление созданий WoG'ом корявых пакованых координат
    pi->WriteHiHook(0x711E7F, SPLICE_, DIRECT_, CDECL_, PackWoGCoordinates);
    pi->WriteHiHook(0x711F49, SPLICE_, DIRECT_, CDECL_, UnpackWoGCoordinates);
    // фикс вылета: нет проверки на на наличие стуктуры целевого стека
    pi->WriteLoHook(0x5A1C17, Y_FixCrash_CastSpell_38);
    // фикс вылета: при удалении препятствия в битве, когда его стуктура таблицы равна нуля
    pi->WriteLoHook(0x46681B, Y_FixCrash_RemoveObstacle);
    // фикс WoG'a
    pi->WriteHiHook(0x070561A, CALL_, DIRECT_, CDECL_, WoG_OnMapRestartCrtraitsReset);
    pi->WriteLoHook(0x04027FE, AdvMgr_ApplyResourceCheat);
    // WoG баг, при котором, если защита отряда из оригинальной ф-ции = 0
    // и нет нападающего отряда при расчёте, игра возвращала мусорное значение;
    pi->WriteLoHook(0x075D42C, WoG_BattleStack_GetDefenceAgainst);
    pi->WriteLoHook(0x0713429, WoG_PlaceObject);
    pi->WriteLoHook(0x04AA153, AdvMgr_EnterToUniversity);
    pi->WriteLoHook(0x4708FC, Y_Dlg_BattleResults_IgnoreClones);
    pi->WriteLoHook(0x441B25, LoHook_00441B25);
    pi->WriteLoHook(0x441BD6, LoHook_00441BD6);
    pi->WriteLoHook(0x41E74A, LoHook_ShacklesRest_AI);
    pi->WriteLoHook(0x478DA2, LoHook_ShacklesRest_Surrend);
    pi->WriteLoHook(0x478EBC, LoHook_ShacklesRest_GU);
    pi->WriteLoHook(0x441982, LoHook_00441982);
    pi->WriteLoHook(0x441AFF, LoHook_00441AFF);
    pi->WriteCodePatch(0x505E15, "%n", 4);
    pi->WriteLoHook(0x505E15, LoHook_FixRefugeeCamp_dx);
    pi->WriteCodePatch(0x408ff7, "%n", 9);
    pi->WriteLoHook(0x408ff2, AdvManager_KeyPress);
    // Byte/data patches from original GameLogic, independent of complex ABI hooks.
    // исправление бага с исчезновением стартового героя при переигрывании
    pi->WriteByte(0x5029C0, 0xEB);
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
    pi->WriteDword(0x4F4B57, 320);
    pi->WriteDword(0x4026FA, 360);
    // радус закрытия всей карты
    pi->WriteDword(0x402751, 360);
    // при доступе к рынку в окне союзника без своих собственных (возможно через торговца артефактов)
    // курс делится на 0, что приводит к крашу при клику на ресурсах и артефактах
    pi->WriteDword(0x678344, 0x3DCCCCCD);
    pi->WriteDword(0x678344 + 11 * 4, 0x3DCCCCCD);
    pi->WriteDword(0x678344 + 22 * 4, 0x3DCCCCCD);
    const int maxCreature = h3::IntAt(0x4A1657);
    pi->WriteDword(0x073A847 + 3, maxCreature + 1);
    pi->WriteDword(0x073A850 + 3, maxCreature);
    // Темница(5) на поверхности генерируется на родном типе земли(6), а не на грязи(0)
    pi->WriteByte(0x464044, 0xEB);
    // Исправляем баг SoD: сброс посещённости сирен после боя.
    pi->WriteCodePatch(0x4DA8FC, "%n", 24);
    // фикс: посещение мифрила обновляет экран
    pi->WriteCodePatch(0x07060AC, "%n", 7);
    pi->WriteJmp(0x0705F42, 0x0705F7D);
    // Теперь в городах игрока при наличии форта всегда будет отстроено 2 уровня существ.
    pi->WriteCodePatch(0x5C1051, "%n", 20);
    // Убираем случайность количества стеков в начальной армии.
    pi->WriteCodePatch(0x4C948F, "%n", 5);
    pi->WriteCodePatch(0x4C9497, "%n", 10);
    pi->WriteCodePatch(0x4C950F, "%n", 20);
    //// ЧИТ-Меню ////
    // Увеличиваем кол-во заклинаний 999 -> 9999
    pi->WriteWord(0x4F508F + 4, 9999);
    pi->WriteWord(0x4F50DE + 4, 9999);
    pi->WriteWord(0x4F511A + 4, 9999);
    // Увеличиваем кол-во существ 5 -> 100
    pi->WriteByte(0x4F4ED2 + 1, 100);
}
} // namespace game_logic_core
void InstallGameLogicCore(PatcherInstance *pi)
{
    game_logic_core::Install(pi);
}
} // namespace gbf
