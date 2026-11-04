// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include <array>

namespace switcher
{
namespace aoe
{

// Строки из JSON
constexpr LPCSTR SINGULAR_HINT_FORMAT_NAME_JSON_KEY_FORMAT = "jsMod.shotAtArea.hintFormat.singular";
constexpr LPCSTR PLURAL_HINT_FORMAT_NAME_JSON_KEY_FORMAT = "jsMod.shotAtArea.hintFormat.plural";
constexpr LPCSTR DESC_FORMAT_NAME_JSON_KEY_FORMAT = "jsMod.shotAtArea.hintFormat.desc";
constexpr LPCSTR DESC_ON_TARGET_FORMAT_NAME_JSON_KEY_FORMAT = "jsMod.shotAtArea.hintFormat.descOnTarget";
constexpr LPCSTR OFF_DESC_ON_TARGET_FORMAT_NAME_JSON_KEY_FORMAT = "jsMod.shotAtArea.hintFormat.offDescOnTarget";
constexpr LPCSTR CANNOT_SHOOT_HINT_FORMAT_NAME_JSON_KEY_FORMAT = "jsMod.shotAtArea.hintFormat.cannotShootHint";

#define o_NetworkGame *(bool *)0x69959C

// Является ли атака атакой по стене.
// Если является, второй параметр функции отрисовки выстрела будет считаться элементом замка.
// Если по гексу - будет считаться гексом.
// Обычный выстрел.
#define ST_USUAL 0
// Выстрел по стене.
#define ST_WALL 1
// Выстрел по гексу.
#define ST_HEX 2
int ShotType = ST_USUAL;
bool areaShotEnabled = true;
// Стороны в бою.
#define ATTACKER 0 // Атакующий
#define DEFENDER 1 // Защищающийся

// Атакующий - для выбора цели площадного выстрела.
H3CombatCreature *AreaShotDlg_Attacker;
// Есть ли лук снайпера у атакующего - для выбора цели площадного выстрела.
bool AreaShotDlg_Has_Bow_S = false;
// Последний выбранный гекс выбора цели площадного выстрела.
int AreaShotDlg_LastSelectedHexIx = -1;
// Cache the actual hover separately: an invalid hex has no selectable target.
int AreaShotDlg_LastHoveredHexIx = -2;
const char *ReadTextOrDefault(const char *key, const char *fallback)
{
    bool translated = false;
    const char *value = EraJS::read(key, translated);
    return translated && value && value[0] ? value : fallback;
}

// Заполнение памяти с автоматическим приведением типов.
#define MemSet(ptr, value, size) memset((void *)(ptr), (char)(value), (size_t)(size))
// Обнуление памяти с автоматическим приведением типов.
#define MemZero(ptr, size) MemSet(ptr, 0, size)

// #define o_GetIngameCursorPos(px, py) STDCALL_2(void, 0x50D700, px, py)
//  Получение имени существа по его номеру и количеству.
#define GetCreatureName(Type, Count) FASTCALL_2(char *, 0x43FE20, (int)Type, (int)Count)

bool CanShoot(H3CombatCreature *stack, int /*aim*/)
{
    return ActionSwitcher::CanShootNormally(stack) != FALSE;
}

// Рядом ли стоят гексы.
// TODO: добавить inline к H3CombatManager
BOOL8 HexesAreNear(H3CombatManager *bm, int hex1_id, int hex2_id)
{
    return THISCALL_3(BOOL8, 0x4670A0, bm, hex1_id, hex2_id);
}
// Расстояние между гексами.
int HexesDist(int hex1_id, int hex2_id)
{
    return FASTCALL_2(int, 0x469250, hex1_id, hex2_id);
}
void GetCombatCursorPosition(int &x, int &y)
{
    H3POINT::GetCursorPosition(x, y);
    x += P_CombatManager->dlg->GetX();
    y += P_CombatManager->dlg->GetY();
}

////////////////////////////////////////////////////////////// HOOKS

// Устанавливаем подсказку площадного выстрела при наступлении хода существа
_LHF_(LoHook_PrepareAction_LichMagogCast)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // 10103D10

    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->ebx);

    // Бой отрисовывается и это человек за этим компьютером.
    if (!P_CombatManager->IsHiddenBattle() && P_CombatManager->isNotAI[P_CombatManager->currentActiveSide] &&
        (!o_NetworkGame ||
         THISCALL_2(BOOL8, 0x4CE630, P_Game->Get(), P_CombatManager->heroOwner[P_CombatManager->currentActiveSide])))
    {
        // Стек может стрелять по площади.
        if (IsAvailable(stack))
        {
            // Стек может стрелять.
            if (CanShoot(stack, 0))
            {
                // Читаем строку из json
                const bool isSingle = stack->numberAlive == 1;
                const char *hintFormat = ReadTextOrDefault(isSingle ? SINGULAR_HINT_FORMAT_NAME_JSON_KEY_FORMAT
                                                                    : PLURAL_HINT_FORMAT_NAME_JSON_KEY_FORMAT,
                                                           "%s can fire an area shot.");
                // Пишем подсказку в лог.
                sprintf(h3_TextBuffer, hintFormat, isSingle ? stack->info.nameSingular : stack->info.namePlural);
                P_CombatManager->AddStatusMessage(h3_TextBuffer);
            }
        }
    }

    return EXEC_DEFAULT;
}

// Убираем невозможность стрельбы в произвольный гекс для площадного выстрела.
_LHF_(LoHook_MakeAttack_LichMagogCast)
{
    // 10103A10

    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    // Стек может стрелять площадным выстрелом по пустым гексам и по себе.
    if (areaShotEnabled && stack->info.fireballAttack)
    {
        c->return_address = 0x4457D4;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Убираем невозможность стрельбы не в стек для площадного выстрела.
_LHF_(LoHook_Shot_LichMagogCast_Allow)
{
    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    // Стек может стрелять площадным выстрелом по пустым гексам и по себе.
    if (areaShotEnabled && stack->info.fireballAttack)
    {
        // Нет целевого стека
        if (c->ecx < 0 || stack->_f_14 < 0) // targetStackIndex
        {
            // Нет целевого стека.
            PtrAt(c->ebp - 4) = 0;

            // Берём целевой гекс для вычисления необходимости поворота.
            c->eax = stack->battlefieldDestination;

            // Восстанавливаем затёртую команду.
            c->ecx = (_ptr_)P_CombatManager->Get();

            // Пропускаем станадртное взятие стека и его гекса.
            c->return_address = 0x43FEF1;

            return NO_EXEC_DEFAULT;
        }
        else
        {
            // Восстанавливаем затёртую команду.
            c->edx = stack->_f_14;

            // Пропускаем стандартную проверку.
            c->return_address = 0x43FEAA;

            return NO_EXEC_DEFAULT;
        }
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Убираем двойную стрельбу для площадного выстрела.
_LHF_(LoHook_DoubleShot_LichMagogCast)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    // Запрещаем двойной выстрел.
    if (!c->ebx || stack->info.fireballAttack)
    {
        c->flags.ZF = true;
        c->flags.SF = false;
        c->flags.CF = false;
        c->flags.OF = false;

        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// Отрисовка полёта снаряда - перенаправляем на гекс.
void __stdcall HiHook_Shot_DrawBullet_ToHex(HiHook *h, H3CombatCreature *this_, H3CombatCreature *enemy)
{
    if (!areaShotEnabled)
    {
        THISCALL_2(void, h->GetDefaultFunc(), this_, enemy);
        return;
    }
    // Aim at the selected hex even when it is occupied: the rear half of a
    // double-wide creature must not move the impact to its front/center.
    if (this_->info.fireballAttack && this_->battlefieldDestination >= 0 && this_->battlefieldDestination < 187)
    {
        const int previousShotType = ShotType;
        ShotType = ST_HEX;
        THISCALL_2(void, h->GetDefaultFunc(), this_, &P_CombatManager->squares[this_->battlefieldDestination]);
        ShotType = previousShotType;
    }
    // Обычная стрельба.
    else
    {
        THISCALL_2(void, h->GetDefaultFunc(), this_, enemy);
    }
}

// Координаты анимации выстрела магога.
_LHF_(LoHook_Coords_MagogCast)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    // The splash center is the clicked hex, including an occupied rear hex.
    if (stack->info.fireballAttack && stack->battlefieldDestination >= 0 && stack->battlefieldDestination < 187)
    {
        // Целевой гекс.
        H3CombatSquare *tar_hex = &P_CombatManager->squares[stack->battlefieldDestination];

        // Def анимации.
        H3LoadedDef *def = reinterpret_cast<H3LoadedDef *>(c->ebx);

        // X-координата.
        IntAt(c->ebp - 0x10) = tar_hex->x - def->widthDEF / 2;

        // Y-координата.
        c->ecx = tar_hex->y - def->heightDEF / 2 - 37; // Центр гекса вместо низа

        // Пропускаем стандартное взятие координат.
        c->return_address = 0x43F7EF;

        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// Координаты анимации выстрела личей.
_LHF_(LoHook_Coords_LichCast)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);

    // The splash center is the clicked hex, including an occupied rear hex.
    if (stack->info.fireballAttack && stack->battlefieldDestination >= 0 && stack->battlefieldDestination < 187)
    {
        // Целевой гекс.
        H3CombatSquare *tar_hex = &P_CombatManager->squares[stack->battlefieldDestination];

        // Def анимации.
        H3LoadedDef *def = reinterpret_cast<H3LoadedDef *>(c->ebx);

        // X-координата.
        IntAt(c->ebp - 0x20) = tar_hex->x - def->widthDEF / 2;

        // Y-координата.
        c->ecx = tar_hex->y - def->heightDEF / 2 - 37; // Центр гекса вместо низа

        // Пропускаем стандартное взятие координат.
        c->return_address = 0x43FBDB;

        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// Typed AI selection and damage dispatch are in AreaShotTypes.cpp.
int AreaShotDlg_SelectHex(int hex_ix, bool updateHint = true);

// Магоги не задевают иммунных к огню: выстрел.
_LHF_(LoHook_Magog_FireImmun_Shot)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Атакующий стек.
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->esi);
    // Целевой стек.
    H3CombatCreature *target = reinterpret_cast<H3CombatCreature *>(c->eax);

    // Магоги не задевают краями иммунных к огню, кроме среднего гекса.
    if (c->edi != 6 && target && target->info.fireImmunity &&
        GetAreaShotType(stack) == AreaShotType::FIRE) // !CanDealAreaDamage(target, AreaShotType::FIRE))
    {
        c->return_address = 0x43F985;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

void ClearAreaShotHighlights(H3CombatManager *manager = P_CombatManager->Get())
{
    if (!manager)
        return;
    for (int side = ATTACKER; side <= DEFENDER; ++side)
        for (int i = 0; i < manager->heroMonCount[side]; ++i)
        {
            H3CombatCreature *stack = &manager->stacks[side][i];
            if (stack->numberAlive > 0)
                THISCALL_2(BOOL8, 0x43ED00, stack, false);
        }
    MemZero(manager->massSpellTarget, sizeof(manager->massSpellTarget));
    AreaShotDlg_LastSelectedHexIx = -1;
}

// Функция обработки диалога выбора цели площадного выстрела.
int __fastcall AreaShotDlgMainFunc(H3Msg *msg)
{
    if (SwitcherDlgPanel::instance && SwitcherDlgPanel::instance->HandleAreaShotSwitch(msg))
    {
        P_CombatManager->action = eCombatAction::CANCEL;
        msg->command = eMsgCommand::MOUSE_BUTTON;
        msg->subtype = eMsgSubtype::END_DIALOG;
        ClearAreaShotHighlights();
        return 2;
    }

    // Проигрываем шаг анимации ожидания.
    THISCALL_1(void, 0x473970, P_CombatManager->Get());

    // Последнее сообщение.
    H3Msg last_msg;

    // Тип пришедшего сообщения.
    switch (msg->command)
    {

    // Нажата клавиша.
    case 1:
        if (msg->subtype != 1) // Не ESC - не реагируем
        {
            return 1;
        }
        P_CombatManager->action = eCombatAction::CANCEL;
        msg->command = eMsgCommand::MOUSE_BUTTON;
        msg->subtype = eMsgSubtype::END_DIALOG;
        ClearAreaShotHighlights();
        return 2;

    // Изменились координаты курсора.
    case 4:
        if (THISCALL_2(H3Msg *, 0x4EC710, *reinterpret_cast<H3InputManager **>(0x699530), &last_msg)->command != 4)
        {
            // Текущий гекс.
            int hex_ix = P_CombatManager->SquareAtCoordinates(msg->subtype - P_CombatManager->dlg->GetX(),
                                                              msg->itemId - P_CombatManager->dlg->GetY());

            // Гекс изменился - перевыделяем стеки и гексы.
            if (hex_ix != AreaShotDlg_LastHoveredHexIx)
            {
                AreaShotDlg_LastSelectedHexIx = AreaShotDlg_SelectHex(hex_ix);
            }
        }
        return 1;
        break;

    // Нажата ЛКМ.
    case 8:
        // Нет выбранного гекса - нет реакции.
        if (AreaShotDlg_LastSelectedHexIx == -1)
        {
            return 1;
        }
        else
        {
            // Выбран целевой гекс.
            P_CombatManager->actionTarget = AreaShotDlg_LastSelectedHexIx;
            msg->command = eMsgCommand::MOUSE_BUTTON;
            msg->subtype = eMsgSubtype::END_DIALOG;

            ClearAreaShotHighlights();

            // Завершение диалога.
            return 2;
        }
        break;

    // Нажата ПКМ.
    case 32:
        // Не выбрано действие.
        P_CombatManager->action = eCombatAction::CANCEL;
        msg->command = eMsgCommand::MOUSE_BUTTON;
        msg->subtype = eMsgSubtype::END_DIALOG;

        ClearAreaShotHighlights();

        // Завершение диалога.
        return 2;
        break;

    default:
        return 1;
        break;
    }
}

int AreaShotDlg_CanCastAtHex(int hex_ix)
{
    // 100FCFC0

    if (AreaShotDlg_Attacker && hex_ix >= 0 && hex_ix < 187 && hex_ix % 17 != 0 && hex_ix % 17 != 16 &&
        hex_ix != AreaShotDlg_Attacker->position &&
        (!AreaShotDlg_Attacker->info.doubleWide || hex_ix != AreaShotDlg_Attacker->GetSecondSquare()) &&
        (AreaShotDlg_Has_Bow_S ||
         (!HexesAreNear(P_CombatManager->Get(), AreaShotDlg_Attacker->position, hex_ix) &&
          (!(AreaShotDlg_Attacker->info.doubleWide) ||
           !HexesAreNear(P_CombatManager->Get(), AreaShotDlg_Attacker->GetSecondSquare(), hex_ix)))))
    {
        return 1;
    }
    return 0;
}

// Выбора гекса при выборе цели для площадного выстрела.
int AreaShotDlg_SelectHex(int hex_ix, bool updateHint)
{
    AreaShotDlg_LastHoveredHexIx = hex_ix;
    // Результирующий целевой гекс.
    int res_hex = AreaShotDlg_CanCastAtHex(hex_ix) ? hex_ix : -1;
    // Тип стрелка
    const AreaShotType shotType = GetAreaShotType(AreaShotDlg_Attacker);

    //// Для луча джагернаутов - доп проверки - 100FC728
    // if (shooterType == 184 || shooterType == 185)
    //{
    //     //hex_ix = sub_100FE2E0(AreaShotDlg_Attacker, hex_ix); // см функцию по адресу
    // }

    // Leaving the battlefield clears its footprint without replacing UI hints.
    if (updateHint)
    {
        // Можно колдовать сюда.
        if (AreaShotDlg_CanCastAtHex(hex_ix))
        {
            // Атакующий герой.
            // H3Hero* att_hero = P_CombatManager->hero[AreaShotDlg_Attacker->GetSide()];

            // Целевой стек.
            H3CombatCreature *tar_stack = P_CombatManager->squares[hex_ix].GetCreature();

            //// Для луча джагернаутов - 100FC773
            // if (shooterType == 184 || shooterType == 185)
            //{

            //}

            // Если нет стека.
            // else if (!tar_stack || sub_10100290(tar_stack))
            if (!tar_stack)
            {
                P_MouseManager->SetCursor(20, 2);
                //// Магог.
                // if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
                //{
                //     P_MouseManager->SetCursor(20, 2);
                // }
                //// Лич.
                // else
                //{
                //     P_MouseManager->SetCursor(21, 2);
                // }
            }
            // Есть стек и кривая стрела.
            else if (THISCALL_4(bool, 0x4670F0, P_CombatManager->Get(), AreaShotDlg_Attacker,
                                AreaShotDlg_Attacker->position, hex_ix) ||
                     THISCALL_3(bool, 0x4671E0, P_CombatManager->Get(), AreaShotDlg_Attacker, tar_stack))
            {
                P_MouseManager->SetCursor(15, 2);
            }
            // Прямая стрела.
            else
            {
                P_MouseManager->SetCursor(3, 2);
            }

            // Нет целевого стека (или джагернаут).
            if (!tar_stack) // || (shooterType == 182 || shooterType == 183) && *(_BYTE
                            // *)(*H3CombatCreature::GetExt(tar_stack) + 65) )
            {
                const char *desc = ReadTextOrDefault(DESC_FORMAT_NAME_JSON_KEY_FORMAT, "Select a target hex.");
                strcpy(h3_TextBuffer, desc);
            }
            // Есть целевой стек.
            else
            {
                char *tar_name = GetCreatureName(tar_stack->type, tar_stack->numberAlive);

                // Включена доп. информация.
                if (DwordAt(0x698818))
                {
                    bool shoot = true;
                    //// Для джагернаутов - не стрельба (для лога)
                    // if (shooterType == 182 || shooterType == 183)
                    //{
                    //     shoot = false;
                    // }

                    // Получаем урон.
                    H3String str;
                    //_HStringF_ str;
                    // Дистанция между стрелком и целью.
                    int distance = HexesDist(AreaShotDlg_Attacker->position, tar_stack->position);
                    // Получаем лог-строку про урон
                    H3String *logDmg =
                        FASTCALL_5(H3String *, 0x492F50, &str, AreaShotDlg_Attacker, tar_stack, shoot, distance);
                    // Получаем текст лога.
                    const char *desc =
                        ReadTextOrDefault(DESC_ON_TARGET_FORMAT_NAME_JSON_KEY_FORMAT, "%s: %d shots, damage %s");
                    sprintf(h3_TextBuffer, desc, tar_name, AreaShotDlg_Attacker->info.numberShots, logDmg->String());

                    // 100FC984
                    // if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
                    //{
                    //    sprintf(h3_TextBuffer, desc, tar_name, AreaShotDlg_Attacker->creature.shots,
                    //    logDmg->String());
                    //}
                    // else
                    //{
                    //    sprintf(h3_TextBuffer, desc, tar_name, AreaShotDlg_Attacker->creature.shots,
                    //    logDmg->String());
                    //}
                }
                // Не включена доп. информация.
                else
                {
                    const char *desc = ReadTextOrDefault(OFF_DESC_ON_TARGET_FORMAT_NAME_JSON_KEY_FORMAT, "%s");
                    sprintf(h3_TextBuffer, desc, tar_name);
                    // if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
                    //{
                    //     sprintf(h3_TextBuffer, misk_text[11], tar_name);
                    // }
                    // else
                    //{
                    //     sprintf(h3_TextBuffer, misk_text[17], tar_name);
                    // }
                }
            }

            // Отправляем сообщение в лог.
            THISCALL_4(void, 0x4729D0, P_CombatManager->dlg, h3_TextBuffer, 0, true);
        }
        // Нельзя колдовать сюда.
        else
        {
            // Курсор - недоступное действие.
            P_MouseManager->SetCursor(0, 2);

            // Сообщение в лог.
            const char *desc = ReadTextOrDefault(CANNOT_SHOOT_HINT_FORMAT_NAME_JSON_KEY_FORMAT,
                                                 "This stack cannot fire at that target.");
            strcpy(h3_TextBuffer, desc);

            // if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
            //{
            //     strcpy(h3_TextBuffer, misk_text[9]);
            // }
            // else
            //{
            //     strcpy(h3_TextBuffer, misk_text[15]);
            // }

            THISCALL_4(void, 0x4729D0, P_CombatManager->dlg, h3_TextBuffer, 0, true);

            // Результирующий гекс - пустой.
            res_hex = -1;
        }
    }
    // Меняем выделенность стеков.

    // Строим список задетых стеков.
    H3Vector<H3CombatCreature *> sel_stacks;

    if (AreaShotDlg_CanCastAtHex(hex_ix))
    {
        //// Для луча джагернаутов - доп проверки
        // if (shooterType == 184 || shooterType == 185)
        //{
        //     sub_10100490(AreaShotDlg_Attacker, hex_ix, P_CombatManager->Get()); // см функцию по адресу
        // }
        // else
        //{
        THISCALL_5(void, 0x5A4A00, P_CombatManager->Get(), hex_ix, 1, true, &sel_stacks);
        //}
    }

    // Нужно ли что-то перерисовывать.
    int need_redraw = false;

    // Отмечаем стеки, которые надо выделить.
    for (int side = ATTACKER; side <= DEFENDER; side++)
    {
        for (int i = 0; i < P_CombatManager->heroMonCount[side]; i++)
        {
            // Текущий стек.
            H3CombatCreature *curr_stack = &P_CombatManager->stacks[side][i];

            if (curr_stack->numberAlive > 0)
            {
                const bool inArea = std::find(sel_stacks.begin(), sel_stacks.end(), curr_stack) != sel_stacks.end();
                const bool atCenter = curr_stack->position == hex_ix ||
                                      (curr_stack->info.doubleWide && curr_stack->GetSecondSquare() == hex_ix);
                const bool affected = res_hex >= 0 && inArea && (atCenter || CanDealAreaDamage(curr_stack, shotType));
                if (affected)
                {
                    if (THISCALL_2(BOOL8, 0x43ED00, curr_stack, true))
                    {
                        // Нужна перерисовка.
                        need_redraw = true;
                    }
                }
                else
                {
                    if (THISCALL_2(BOOL8, 0x43ED00, curr_stack, false))
                    {
                        // Нужна перерисовка.
                        need_redraw = true;
                    }
                }
            }
        }
    }

    // Выделяем гексы.
    if (DwordAt(0x698810))
    {
        // Строим список задетых гексов.
        H3Vector<int> sel_hexes;

        if (AreaShotDlg_CanCastAtHex(hex_ix))
        {
            // 100FCDE8
            // if (shooterType == 184 || shooterType == 185)
            //{

            //}
            // else
            //{
            THISCALL_5(void, 0x5A4480, P_CombatManager->Get(), hex_ix, 1, true, &sel_hexes);
            //}
        }

        // Убираем иммунных существ из выделения.
        for (INT i = static_cast<INT>(sel_hexes.Size()) - 1; i >= 0; --i)
        {
            // Гекс корректен.
            if (sel_hexes[i] >= 0 && sel_hexes[i] < 187)
            {
                // Стек на гексе.
                H3CombatCreature *stack = P_CombatManager->squares[sel_hexes[i]].GetCreature();

                // Не выделяем гекс с иммунным стеком.
                if (stack && !stack->highlightContour)
                {
                    // Указатель на текущий элемент списка.
                    // int* p = &sel_hexes[i];

                    //// Удаляем элемент из списка.
                    // while (p < sel_hexes.EndData - 1)
                    //{
                    //     *p = *(p + 1);
                    //     p++;
                    // }
                    // sel_hexes.EndData--;

                    // continue;
                    sel_hexes.Remove(i);
                }
            }
        }

        // The native hover layer restores its previous footprint before drawing
        // this one. Leave movement shading and its cached background untouched.
        THISCALL_4(void, 0x493A20, P_CombatManager->Get(), res_hex, &sel_hexes, true);
    }

    // Убираем текущее выделение стеков.
    MemZero(P_CombatManager->massSpellTarget, sizeof(P_CombatManager->massSpellTarget));

    // 0x493A20 already restores the previous shade, draws creatures and flips
    // the affected region. Do not draw all creatures a second time over it.
    // With mouse shading disabled, only contour changes need a normal redraw.
    if (need_redraw && !DwordAt(0x698810))
        P_CombatManager->Refresh();

    return res_hex;
}

// Выбор цели площадного выстрела.
void AreaShotDlg(H3CombatManager *b_mgr, H3CombatCreature *attacker)
{
    // Только для способных к площадному выстрелу существ.
    if (!attacker->info.fireballAttack)
    {
        return;
    }
    if (!CanShoot(attacker, 0))
    {
        return;
    }

    // Нет действия.
    b_mgr->action = eCombatAction::CANCEL;
    // Нет дополнительной информации.
    b_mgr->actionParameter = -1;
    // Нет выбранного гекса.
    b_mgr->actionTarget = -1;
    // Нет второго выбранного гекса.
    b_mgr->actionParameter2 = -1;

    // Запоминаем атакующего.
    AreaShotDlg_Attacker = attacker;

    // G and the action button enter this same modal picker.

    // Тип действия - стрельба.
    b_mgr->action = eCombatAction::SHOOT;

    // Получаем координаты мыши.
    int mouse_x;
    int mouse_y;
    H3POINT::GetCursorPosition(mouse_x, mouse_y);

    // Выбранный гекс.
    int hex_ix = b_mgr->SquareAtCoordinates(mouse_x - b_mgr->dlg->GetX(), mouse_y - b_mgr->dlg->GetY());

    // Сбрасываем выделенный стек.
    THISCALL_2(void, 0x477740, b_mgr, 0);

    // Выделяем выбранный гекс.
    AreaShotDlg_LastSelectedHexIx = AreaShotDlg_SelectHex(hex_ix);

    // Вызываем диалог.
    THISCALL_4(int, 0x602AE0, P_WindowManager->Get(), 0, AreaShotDlgMainFunc, false);

    // Получаем координаты мыши.
    H3POINT::GetCursorPosition(mouse_x, mouse_y);
    // Выбранный гекс.
    hex_ix = b_mgr->SquareAtCoordinates(mouse_x - b_mgr->dlg->GetX(), mouse_y - b_mgr->dlg->GetY());

    // Выделяем стек на выбранном гексе.
    THISCALL_2(void, 0x477550, b_mgr, hex_ix);

    // Курсор - стрелка.
    P_MouseManager->SetCursor(6, 2);
}

bool IsAvailable(const H3CombatCreature *stack)
{
    return areaShotEnabled && stack && stack->numberAlive > 0 && stack->info.fireballAttack &&
           GetAreaShotType(stack) != AreaShotType::NONE && ActionSwitcher::CanShootNormally(stack);
}

void ClearHoverShade(H3CombatManager *manager)
{
    if (DwordAt(0x698810))
    {
        H3Vector<INT> empty;
        THISCALL_4(void, 0x493A20, manager, -1, &empty, true);
    }
}

void ResetTargeting(H3CombatManager *manager)
{
    if (!AreaShotDlg_Attacker)
        return;
    if (manager)
    {
        ClearAreaShotHighlights(manager);
        ClearHoverShade(manager);
    }
    AreaShotDlg_Attacker = nullptr;
    AreaShotDlg_Has_Bow_S = false;
    AreaShotDlg_LastSelectedHexIx = -1;
    AreaShotDlg_LastHoveredHexIx = -2;
}

void SetEnabled(bool enabled)
{
    areaShotEnabled = enabled;
    if (!enabled)
    {
        ResetTargeting(P_CombatManager->Get());
        ShotType = ST_USUAL;
    }
}

void OpenForActiveStack()
{
    H3CombatManager *manager = P_CombatManager->Get();
    if (!manager || manager->action != eCombatAction::CANCEL || !IsAvailable(manager->activeStack) ||
        AreaShotDlg_Attacker)
        return;
    H3CombatCreature *stack = manager->activeStack;
    AreaShotDlg_Attacker = stack;
    const H3Hero *hero = stack->GetOwner();
    AreaShotDlg_Has_Bow_S = hero && hero->WearsArtifact(eArtifact::BOW_OF_THE_SHARPSHOOTER);

    // Only the modal picker owns this snapshot. Hide movement range like a spell
    // picker, update the existing background, then restore it before returning.
    std::array<INT8, 187> movementShade;
    memcpy(movementShade.data(), manager->accessibleSquares, movementShade.size());
    ClearHoverShade(manager);
    memset(manager->accessibleSquares, 0, movementShade.size());
    // Rebuild from the native terrain/obstacle background once, not from pixels
    // that already contain movement or creature shadows. Hovering reuses it.
    manager->doNotDrawShade = FALSE;
    manager->Refresh();

    AreaShotDlg(manager, stack);
    const INT targetHex = manager->actionTarget;
    const bool confirmed = manager->action == eCombatAction::SHOOT && targetHex >= 0 && targetHex < 187;
    ResetTargeting(manager);
    memcpy(manager->accessibleSquares, movementShade.data(), movementShade.size());
    // The outer selector redraws once on cancel; action processing redraws after
    // confirmation. In both cases the native background starts clean again.
    manager->doNotDrawShade = FALSE;
    if (confirmed)
    {
        // Send only the native SHOOT command and clicked hex to the other peer.
        H3CombatCreature *target = manager->squares[targetHex].GetCreature();
        stack->_f_10 = target ? target->side : -1;
        stack->_f_14 = target ? target->sideIndex : -1;
        stack->battlefieldDestination = targetHex;
        manager->actionParameter = manager->actionParameter2 = -1;
    }
}

//// Тип текущего диалога стека (0 - обычный, 1 - сказочный дракон, 2 - магоги, 3 - личи).
// int curr_cr_dlg_type;
//
//// Площадный выстрел по кнопке в окне стека.
//_LHF_(LoHook_AreaShot_Button)
//{
//    // Магоги, личи.
//    if (curr_cr_dlg_type == 2 || curr_cr_dlg_type == 3)
//    {
//        // Ходит игрок за этим компьютером.
//        if (P_CombatManager->Field<_bool32_>(78516))
//        {
//            AreaShotDlg(P_CombatManager->Get(),
//            &P_CombatManager->stacks[P_CombatManager->currentMonSide][P_CombatManager->currentMonIndex]);
//        }
//
//        // Пропускаем стандартную обработку.
//        c->return_address = 0x468546;
//
//        return NO_EXEC_DEFAULT;
//    }
//    else
//    {
//        return EXEC_DEFAULT;
//    }
//}

//// ПКМ по кнопке активации способности.
//_LHF_(LoHook_ViewArmy_ActButtonRcl)
//{
//    // Кнопка активации способности.
//    if (c->esi == 15)
//    {
//        // Сказочный дракон.
//        if (curr_cr_dlg_type == 1)
//        {
//            c->esi = (_ptr_)misk_text[19]; // "Выбрать цель заклинания"
//        }
//        // Магог.
//        else if (curr_cr_dlg_type == 2)
//        {
//            c->esi = (_ptr_)misk_text[20]; // "Выбрать цель огненного шара"
//        }
//        // Лич.
//        else
//        {
//            c->esi = (_ptr_)misk_text[21]; // "Выбрать цель облака смерти"
//        }
//
//        // Пропускаем стандартную обработку.
//        c->return_address = 0x5F4CA0;
//
//        return NO_EXEC_DEFAULT;
//    }
//    else
//    {
//        return EXEC_DEFAULT;
//    }
//}

//// Наведение на кнопку активации способности.
// int __stdcall LoHook_ViewArmy_ActButtonCursor(LoHook* h, HookContext* c)
//{
//     // Кнопка активации способности.
//     if (c->eax == 15)
//     {
//         // Сказочный дракон.
//         if (curr_cr_dlg_type == 1)
//         {
//             c->edi = (_ptr_)misk_text[19]; // "Выбрать цель заклинания"
//         }
//         // Магог.
//         else if (curr_cr_dlg_type == 2)
//         {
//             c->edi = (_ptr_)misk_text[20]; // "Выбрать цель огненного шара"
//         }
//         // Лич.
//         else
//         {
//             c->edi = (_ptr_)misk_text[21]; // "Выбрать цель облака смерти"
//         }
//
//         // Пропускаем стандартную обработку.
//         c->return_address = 0x5F5336;
//
//         return NO_EXEC_DEFAULT;
//     }
//     else
//     {
//         return EXEC_DEFAULT;
//     }
// }

// Подменяем стек на элемент замка или гекс в функции отрисовки выстрела.
int __stdcall Hook_OnShotDrawEnemyPosGet(LoHook *h, HookContext *c)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;

    H3CombatSquare *hex = *reinterpret_cast<H3CombatSquare **>(c->ebp + 0x8);

    if (ShotType == ST_WALL)
    {
        // TODO: тут нужна другая структура! hex подставлен с подходящими оффсетами наобум
        // H3ValidCatapultTargets

        H3CombatSquare *hex = *reinterpret_cast<H3CombatSquare **>(c->ebp + 0x8);

        // Берём координаты атакуемого элемента.
        IntAt(c->ebp - 0x14) = hex->left;

        c->edi = hex->top;
        IntAt(c->ebp - 0x34) = hex->top;

        // Пропускаем стандартный расчёт.
        c->return_address = 0x43F06F;

        // Восстанавливаем затёртую команду.
        c->ecx = (int)P_CombatManager->Get();

        return NO_EXEC_DEFAULT;
    }
    else if (ShotType == ST_HEX)
    {
        // Атакуемый гекс.
        H3CombatSquare *hex = *reinterpret_cast<H3CombatSquare **>(c->ebp + 0x8);
        // Берём координаты атакуемого гекса.
        IntAt(c->ebp - 0x14) = hex->x;
        c->edi = hex->y - 37;               // Центр гекса вместо низа
        IntAt(c->ebp - 0x34) = hex->y - 37; // Центр гекса вместо низа

        // Пропускаем стандартный расчёт.
        c->return_address = 0x43F06F;

        // Восстанавливаем затёртую команду.
        c->ecx = (int)P_CombatManager->Get();

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Подменяем стек на элемент замка или гекс в функции отрисовки магического выстрела (вертикаль).
int __stdcall Hook_OnMagicShotDrawEnemy_Y_PosGet(HiHook *h, DWORD *Tar)
{
    if (!areaShotEnabled)
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);

    if (ShotType == ST_WALL)
    {
        // TODO: тут нужна другая структура! hex подставлен с подходящими оффсетами наобум
        // H3ValidCatapultTargets

        // Берём координаты атакуемого элемента.
        return ((H3CombatSquare *)Tar)->top;
    }
    else if (ShotType == ST_HEX)
    {
        // Берём координаты атакуемого гекса.
        return ((H3CombatSquare *)Tar)->y;
    }
    else
    {
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);
    }
}

// Подменяем стек на элемент замка или гекс в функции отрисовки магического выстрела (горизонталь).
int __stdcall Hook_OnMagicShotDrawEnemy_X_PosGet(HiHook *h, DWORD *Tar)
{
    if (!areaShotEnabled)
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);
    if (ShotType == ST_WALL)
    {
        // TODO: тут нужна другая структура! hex подставлен с подходящими оффсетами наобум
        // H3ValidCatapultTargets

        // Берём координаты атакуемого элемента.
        return ((H3CombatSquare *)Tar)->left;
    }
    else if (ShotType == ST_HEX)
    {
        // Берём координаты атакуемого гекса.
        return ((H3CombatSquare *)Tar)->x;
    }
    else
    {
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);
    }
}

void Install(PatcherInstance *patcher)
{
    static bool installed = false;
    if (installed || !patcher)
        return;
    installed = true;

    // Show the area attack hint when an eligible stack's turn begins.
    patcher->WriteLoHook(0x4650D0, LoHook_PrepareAction_LichMagogCast);

    // Allow area shots to target empty hexes and creature stacks.
    patcher->WriteLoHook(0x4457CE, LoHook_MakeAttack_LichMagogCast);
    patcher->WriteLoHook(0x43FE97, LoHook_Shot_LichMagogCast_Allow);

    // Prevent the double-shot code from consuming a second shot.
    patcher->WriteLoHook(0x43FF8B, LoHook_DoubleShot_LichMagogCast);
    patcher->WriteLoHook(0x43FFA3, LoHook_DoubleShot_LichMagogCast);

    // Redirect projectile and animation coordinates to the selected hex.
    patcher->WriteHiHook(0x43F6FC, CALL_, EXTENDED_, THISCALL_, HiHook_Shot_DrawBullet_ToHex);
    patcher->WriteLoHook(0x43F789, LoHook_Coords_MagogCast);
    patcher->WriteLoHook(0x43FB75, LoHook_Coords_LichCast);

    // Keep the return animation when a shot deals no damage.
    patcher->WriteCodePatch(0x43F997, (char *)"0F 8E ~d", 0x43FA05);
    patcher->WriteCodePatch(0x43FD91, (char *)"7E 6E");

    // AI target selection and fire-immunity handling.
    InstallTypes(patcher);
    patcher->WriteLoHook(0x43F8E7, LoHook_Magog_FireImmun_Shot);

    // G and ALT/button selection share SelectAction -> OpenForActiveStack.

    // Draw projectile impacts at a wall or a battlefield hex.
    patcher->WriteLoHook(0x43F016, Hook_OnShotDrawEnemyPosGet);
    patcher->WriteHiHook(0x5A0111, CALL_, EXTENDED_, THISCALL_, Hook_OnMagicShotDrawEnemy_Y_PosGet);
    patcher->WriteHiHook(0x5A0119, CALL_, EXTENDED_, THISCALL_, Hook_OnMagicShotDrawEnemy_X_PosGet);
}

} // namespace aoe
} // namespace switcher
