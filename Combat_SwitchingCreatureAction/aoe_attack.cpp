// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

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
H3CombatCreature* AreaShotDlg_Attacker;
// Есть ли лук снайпера у атакующего - для выбора цели площадного выстрела.
bool AreaShotDlg_Has_Bow_S = false;
// Необходимость отрисовывать рамки вокруг стеков при анимации ожидания.
bool NeedRedrawBorders = false;
// Был ли диалог скрытым (как диалоги наложения заклинаний).
bool Dlg_WasHidden = false;
// Последний выбранный гекс выбора цели площадного выстрела.
int AreaShotDlg_LastSelectedHexIx = -1;

const char *ReadTextOrDefault(const char *key, const char *fallback)
{
    bool translated = false;
    const char *value = EraJS::read(key, translated);
    return translated && value && value[0] ? value : fallback;
}


// Заполнение памяти с автоматическим приведением типов.
#define MemSet(ptr, value, size) memset((void*)(ptr), (char)(value), (size_t)(size))
// Обнуление памяти с автоматическим приведением типов.
#define MemZero(ptr, size) MemSet(ptr, 0, size)

//#define o_GetIngameCursorPos(px, py) STDCALL_2(void, 0x50D700, px, py)
// Получение имени существа по его номеру и количеству.
#define GetCreatureName(Type, Count) FASTCALL_2(char*, 0x43FE20, (int)Type, (int)Count)

bool CanShoot(H3CombatCreature *stack, int /*aim*/)
{
    return ActionSwitcher::CanShootNormally(stack) != FALSE;
}

// Рядом ли стоят гексы.
// TODO: добавить inline к H3CombatManager
BOOL8 HexesAreNear(H3CombatManager* bm, int hex1_id, int hex2_id)
{
    return THISCALL_3(BOOL8, 0x4670A0, bm, hex1_id, hex2_id);
}
// Расстояние между гексами.
int HexesDist(int hex1_id, int hex2_id)
{
    return FASTCALL_2(int, 0x469250, hex1_id, hex2_id);
}
// Отрисовка поля боя.
// Flip - необходимость обновления экрана.
// SetBattleRedraws - необходимость настройки границ обновления экрана.
// UseBattleRedraws - необходимость использования границ обновления экрана (при SetBattleRedraws = TRUE игнорируется, считаясь TRUE).
// WaitingTime - время, на ожидание которого надо настроить следующую отрисовку (игнорируется при Wait = FALSE).
// RedrawBackground - необходимость перерисовки заднего плана (и стёрки старого изображения).
// Wait - необходимость ожидания. В случае FALSE время ожидания для следующей отрисовки не меняется.
void RedrawBattlefield(H3CombatManager* bm, BOOL8 Flip, BOOL8 SetBattleRedraws, BOOL8 UseBattleRedraws, int WaitingTime, BOOL8 RedrawBackground, BOOL8 Wait)
{
    return THISCALL_7(void, 0x493FC0, bm, Flip, SetBattleRedraws, UseBattleRedraws, WaitingTime, RedrawBackground, Wait);
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
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature*>(c->ebx);

    // Бой отрисовывается и это человек за этим компьютером.
    if (!P_CombatManager->IsHiddenBattle() && P_CombatManager->isNotAI[P_CombatManager->currentActiveSide] &&
        (!o_NetworkGame ||
         THISCALL_2(BOOL8, 0x4CE630, P_Game->Get(), P_CombatManager->heroOwner[P_CombatManager->currentActiveSide])))
    {
        // Стек может стрелять по площади.
        if (stack->info.fireballAttack)
        {
            // Стек может стрелять.
            if (CanShoot(stack, 0))
            {
                // Читаем строку из json
                const bool isSingle = stack->numberAlive == 1;
                const char *hintFormat = ReadTextOrDefault(
                    isSingle ? SINGULAR_HINT_FORMAT_NAME_JSON_KEY_FORMAT : PLURAL_HINT_FORMAT_NAME_JSON_KEY_FORMAT,
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
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);

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
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);

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
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);

    // Запрещаем двойной выстрел.
    if (!c->ebx || stack->info.fireballAttack)
    {
        c->flags.ZF = true;
        c->flags.SF = false;
        c->flags.CF = false;
        c->flags.OF = false;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Отрисовка полёта снаряда - перенаправляем на гекс.
void __stdcall HiHook_Shot_DrawBullet_ToHex(HiHook* h, H3CombatCreature* this_, H3CombatCreature* enemy)
{
    if (!areaShotEnabled)
    {
        THISCALL_2(void, h->GetDefaultFunc(), this_, enemy);
        return;
    }
    // Это стрельба по гексу.
    if (!enemy)
    {
        ShotType = ST_HEX;
        THISCALL_2(void, h->GetDefaultFunc(), this_, &P_CombatManager->squares[this_->battlefieldDestination]);
        ShotType = ST_USUAL;
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
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);

    // Нет целевого стека.
    if (!c->edi)
    {
        // Целевой гекс.
        H3CombatSquare* tar_hex = &P_CombatManager->squares[stack->battlefieldDestination];

        // Def анимации.
        H3LoadedDef* def = reinterpret_cast<H3LoadedDef*>(c->ebx);

        // X-координата.
        IntAt(c->ebp - 0x10) = tar_hex->x - def->widthDEF / 2;

        // Y-координата.
        c->ecx = tar_hex->y - def->heightDEF / 2 - 37; // Центр гекса вместо низа

        // Пропускаем стандартное взятие координат.
        c->return_address = 0x43F7EF;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Координаты анимации выстрела личей.
_LHF_(LoHook_Coords_LichCast)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Стек.
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);

    // Нет целевого стека.
    if (!c->edi)
    {
        // Целевой гекс.
        H3CombatSquare* tar_hex = &P_CombatManager->squares[stack->battlefieldDestination];

        // Def анимации.
        H3LoadedDef* def = reinterpret_cast<H3LoadedDef*>(c->ebx);

        // X-координата.
        IntAt(c->ebp - 0x20) = tar_hex->x - def->widthDEF / 2;

        // Y-координата.
        c->ecx = tar_hex->y - def->heightDEF / 2 - 37; // Центр гекса вместо низа

        // Пропускаем стандартное взятие координат.
        c->return_address = 0x43FBDB;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Выбор ИИ гекса для площадного выстрела.
_LHF_(LoHook_AISelectHex_LichMagogCast)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Не площадный выстрел.
    if (!ByteAt(c->ebp - 0xD))
    {
        return EXEC_DEFAULT;
    }

    // Лучший целевой гекс среди пустых.
    int tar_hex_empty = -1;
    // Максимальная ценность среди пустых гексов.
    int max_val_empty = 0;
    // Путь до лучшего целевого гекса среди пустых.
    int way_len_empty = -1;

    // Лучший целевой гекс среди задних гексов существ.
    int tar_hex_2hex = -1;
    // Максимальная ценность среди задних гексов существ.
    int max_val_2hex = 0;
    // Путь до лучшего целевого гекса среди задних гексов существ.
    int way_len_2hex = -1;

    // Лучший целевой гекс среди передних гексов существ.
    int tar_hex_1hex = -1;
    // Максимальная ценность среди передних гексов существ.
    int max_val_1hex = 0;
    // Путь до лучшего целевого гекса среди передних гексов существ.
    int way_len_1hex = -1;

    // Атакующий стек.
    H3CombatCreature* att_stack = *reinterpret_cast<H3CombatCreature**>(c->ebp + 8);

    // Первый гекс атакующего (стека).
    int att_hex1 = att_stack->position;
    // Второй гекс атакующего.
    int att_hex2 = (att_stack->info.doubleWide) ? att_stack->GetSecondSquare() : -1;

    // Атакующий герой.
    H3Hero* att_hero = P_CombatManager->hero[att_stack->GetSide()];

    // Есть ли лук снайпера у атакующего.
    bool has_bow_s = att_hero && att_hero->WearsArtifact(eArtifact::BOW_OF_THE_SHARPSHOOTER);


    // По всем гексам.
    for (int i = 0; i < 187; i++)
    {
        // Не крайние колонки.
        if (i % 17 != 0 && i % 17 != 16)
        {
            // Сам в себя - нельзя.
            if (i == att_stack->position || (att_stack->info.doubleWide) && i == att_stack->GetSecondSquare())
            {
                continue;
            }

            // Нет лука снайпера.
            if (!has_bow_s)
            {
                // Нельзя бить в гексы рядом с собой.
                if (HexesAreNear(P_CombatManager->Get(), att_stack->position, i) || (att_stack->info.doubleWide) && HexesAreNear(P_CombatManager->Get(), att_stack->GetSecondSquare(), i))
                {
                    continue;
                }
            }

            // Стек на гексе.
            H3CombatCreature* stack = P_CombatManager->squares[i].GetCreature();

            // Не пустой гекс.
            if (stack)
            {
                // Задний гекс.
                if ((stack->info.doubleWide) && stack->position == i)
                {
                    // Ценность для этого гекса.
                    int val = FASTCALL_4(int, 0x41EF20, att_stack, i, IntAt(c->ebp - 0x18), PtrAt(c->ebp + 0xC));

                    // Путь до 1 гекса.
                    int curr_way = HexesDist(att_hex1, i);
                    // Путь до 2 гекса.
                    if (att_hex2 >= 0)
                    {
                        int curr_way2 = HexesDist(att_hex2, i);
                        if (curr_way2 < curr_way) curr_way = curr_way2;
                    }

                    // Новая максимальная ценность.
                    if (val > 0 && (val > max_val_2hex || (val == max_val_2hex && curr_way < way_len_2hex)))
                    {
                        max_val_2hex = val;
                        tar_hex_2hex = i;
                        way_len_2hex = curr_way;
                    }
                }
                // Передний гекс.
                else
                {
                    // Ценность для этого гекса.
                    int val = FASTCALL_4(int, 0x41EF20, att_stack, i, IntAt(c->ebp - 0x18), PtrAt(c->ebp + 0xC));

                    // Путь до 1 гекса.
                    int curr_way = HexesDist(att_hex1, i);
                    // Путь до 2 гекса.
                    if (att_hex2 >= 0)
                    {
                        int curr_way2 = HexesDist(att_hex2, i);
                        if (curr_way2 < curr_way) curr_way = curr_way2;
                    }

                    // Новая максимальная ценность.
                    if (val > 0 && (val > max_val_1hex || (val == max_val_1hex && curr_way < way_len_1hex)))
                    {
                        max_val_1hex = val;
                        tar_hex_1hex = i;
                        way_len_1hex = curr_way;
                    }
                }
            }
            // Пустой гекс.
            else
            {
                // Ценность для этого гекса.
                int val = FASTCALL_4(int, 0x41EF20, att_stack, i, IntAt(c->ebp - 0x18), PtrAt(c->ebp + 0xC));

                // Путь до 1 гекса.
                int curr_way = HexesDist(att_hex1, i);
                // Путь до 2 гекса.
                if (att_hex2 >= 0)
                {
                    int curr_way2 = HexesDist(att_hex2, i);
                    if (curr_way2 < curr_way) curr_way = curr_way2;
                }

                // Новая максимальная ценность.
                if (val > 0 && (val > max_val_empty || (val == max_val_empty && curr_way < way_len_empty)))
                {
                    max_val_empty = val;
                    tar_hex_empty = i;
                    way_len_empty = curr_way;
                }
            }
        }
    }


    // Ценность пустого гекса - наивысшая.
    if (max_val_empty > max_val_1hex && max_val_empty > max_val_2hex)
    {
        IntAt(c->ebp - 0x28) = tar_hex_empty;
        IntAt(PtrAt(c->ebp + 0x10)) = max_val_empty;
    }
    // Ценность заднего гекса - наивысшая (или равна ценности пустого гекса).
    else if (max_val_2hex > max_val_1hex)
    {
        IntAt(c->ebp - 0x28) = tar_hex_2hex;
        IntAt(PtrAt(c->ebp + 0x10)) = max_val_2hex;
    }
    // Ценность заднего гекса - наивысшая (или равна ценности другого гекса).
    else if (tar_hex_1hex > 0)
    {
        IntAt(c->ebp - 0x28) = tar_hex_1hex;
        IntAt(PtrAt(c->ebp + 0x10)) = max_val_1hex;
    }

    // Пропускаем стандарнтую обработку.
    c->return_address = 0x41EF01;

    return NO_EXEC_DEFAULT;
}

// Магоги не задевают иммунных к огню: ИИ.
_LHF_(LoHook_Magog_FireImmun_AI)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Атакующий стек.
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->edi);
    // Целевой стек.
    H3CombatCreature* target = reinterpret_cast<H3CombatCreature*>(c->esi);

    // Магоги не задевают краями иммунных к огню.
    if (stack->type == eCreature::MAGOG && target->info.fireImmunity)
    {
        c->return_address = 0x41EFAE;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

int AreaShotDlg_SelectHex(int hex_ix);

// Магоги не задевают иммунных к огню: выстрел.
_LHF_(LoHook_Magog_FireImmun_Shot)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;
    // Атакующий стек.
    H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);
    // Целевой стек.
    H3CombatCreature* target = reinterpret_cast<H3CombatCreature*>(c->eax);

    //if (stack->side == target->side && stack->sideIndex == target->sideIndex)
    //{
    //    *(_BYTE*)(*(_DWORD*)sub_10081180(MEMORY[0x699420]) + 0x28) = 1;
    //    c->return_address = 0x43F985;
    //    return NO_EXEC_DEFAULT;
    //}

    // Магоги не задевают краями иммунных к огню, кроме среднего гекса.
    if (c->edi != 6 && target->info.fireImmunity)
    {
        c->return_address = 0x43F985;
        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

void ClearAreaShotHighlights()
{
    for (int side = ATTACKER; side <= DEFENDER; ++side)
        for (int i = 0; i < P_CombatManager->heroMonCount[side]; ++i)
        {
            H3CombatCreature *stack = &P_CombatManager->stacks[side][i];
            if (stack->numberAlive > 0)
                THISCALL_2(BOOL8, 0x43ED00, stack, false);
        }
    AreaShotDlg_LastSelectedHexIx = -1;
}

// Функция обработки диалога выбора цели площадного выстрела.
int __fastcall AreaShotDlgMainFunc(H3Msg* msg)
{
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
        if (THISCALL_2(H3Msg*, 0x4EC710, P_InputManager->Get(), &last_msg)->command != 4)
        {
            // Текущий гекс.
            int hex_ix = P_CombatManager->SquareAtCoordinates(msg->subtype - P_CombatManager->dlg->GetX(), msg->itemId - P_CombatManager->dlg->GetY());

            // Гекс изменился - перевыделяем стеки и гексы.
            if (hex_ix != AreaShotDlg_LastSelectedHexIx)
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

    if (hex_ix >= 0 && hex_ix < 187 && hex_ix % 17 != 0 && hex_ix % 17 != 16 &&
        hex_ix != AreaShotDlg_Attacker->position &&
        (!AreaShotDlg_Attacker->info.doubleWide || hex_ix != AreaShotDlg_Attacker->GetSecondSquare()) &&
        (AreaShotDlg_Has_Bow_S || (!HexesAreNear(P_CombatManager->Get(), AreaShotDlg_Attacker->position, hex_ix) &&
            (!(AreaShotDlg_Attacker->info.doubleWide) || !HexesAreNear(P_CombatManager->Get(), AreaShotDlg_Attacker->GetSecondSquare(), hex_ix)))))
    {
        return 1;
    }
    return 0;
}


// Выбора гекса при выборе цели для площадного выстрела.
int AreaShotDlg_SelectHex(int hex_ix)
{
    // Результирующий целевой гекс.
    int res_hex = hex_ix;
    // Тип стрелка
    int shooterType = AreaShotDlg_Attacker->type;

    //// Для луча джагернаутов - доп проверки - 100FC728
    //if (shooterType == 184 || shooterType == 185)
    //{
    //    //hex_ix = sub_100FE2E0(AreaShotDlg_Attacker, hex_ix); // см функцию по адресу
    //}

    // Можно колдовать сюда.
    if (AreaShotDlg_CanCastAtHex(hex_ix))
    {
        // Атакующий герой.
        //H3Hero* att_hero = P_CombatManager->hero[AreaShotDlg_Attacker->GetSide()];

        // Целевой стек.
        H3CombatCreature* tar_stack = P_CombatManager->squares[hex_ix].GetCreature();

        //// Для луча джагернаутов - 100FC773
        //if (shooterType == 184 || shooterType == 185)
        //{

        //}

        // Если нет стека.
        //else if (!tar_stack || sub_10100290(tar_stack))
        if (!tar_stack)
        {
            P_MouseManager->SetCursor(20, 2);
            //// Магог.
            //if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
            //{
            //    P_MouseManager->SetCursor(20, 2);
            //}
            //// Лич.
            //else
            //{
            //    P_MouseManager->SetCursor(21, 2);
            //}
        }
        // Есть стек и кривая стрела.
        else if (THISCALL_4(bool, 0x4670F0, P_CombatManager->Get(), AreaShotDlg_Attacker, AreaShotDlg_Attacker->position, hex_ix) ||
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
        if (!tar_stack) // || (shooterType == 182 || shooterType == 183) && *(_BYTE *)(*H3CombatCreature::GetExt(tar_stack) + 65) )
        {
            const char* desc = ReadTextOrDefault(DESC_FORMAT_NAME_JSON_KEY_FORMAT, "Select a target hex.");
            strcpy(h3_TextBuffer, desc);
        }
        // Есть целевой стек.
        else
        {
            char* tar_name = GetCreatureName(tar_stack->type, tar_stack->numberAlive);

            // Включена доп. информация.
            if (DwordAt(0x698818))
            {
                bool shoot = true;
                //// Для джагернаутов - не стрельба (для лога)
                //if (shooterType == 182 || shooterType == 183)
                //{
                //    shoot = false;
                //}

                // Получаем урон.
                H3String str;
                //_HStringF_ str;
                // Дистанция между стрелком и целью.
                int distance = HexesDist(AreaShotDlg_Attacker->position, tar_stack->position);
                // Получаем лог-строку про урон
                H3String* logDmg = FASTCALL_5(H3String*, 0x492F50, &str, AreaShotDlg_Attacker, tar_stack, shoot, distance);
                // Получаем текст лога.
                const char* desc = ReadTextOrDefault(DESC_ON_TARGET_FORMAT_NAME_JSON_KEY_FORMAT,
                                                     "%s: %d shots, damage %s");
                sprintf(h3_TextBuffer, desc, tar_name, AreaShotDlg_Attacker->info.numberShots, logDmg->String());

                // 100FC984
                //if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
                //{
                //    sprintf(h3_TextBuffer, desc, tar_name, AreaShotDlg_Attacker->creature.shots, logDmg->String());
                //}
                //else
                //{
                //    sprintf(h3_TextBuffer, desc, tar_name, AreaShotDlg_Attacker->creature.shots, logDmg->String());
                //}

            }
            // Не включена доп. информация.
            else
            {
                const char* desc = ReadTextOrDefault(OFF_DESC_ON_TARGET_FORMAT_NAME_JSON_KEY_FORMAT, "%s");
                sprintf(h3_TextBuffer, desc, tar_name);
                //if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
                //{
                //    sprintf(h3_TextBuffer, misk_text[11], tar_name);
                //}
                //else
                //{
                //    sprintf(h3_TextBuffer, misk_text[17], tar_name);
                //}
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
        const char* desc = ReadTextOrDefault(CANNOT_SHOOT_HINT_FORMAT_NAME_JSON_KEY_FORMAT,
                                             "This stack cannot fire at that target.");
        strcpy(h3_TextBuffer, desc);

        //if (AreaShotDlg_Attacker->type == eCreature::MAGOG)
        //{
        //    strcpy(h3_TextBuffer, misk_text[9]);
        //}
        //else
        //{
        //    strcpy(h3_TextBuffer, misk_text[15]);
        //}

        THISCALL_4(void, 0x4729D0, P_CombatManager->dlg, h3_TextBuffer, 0, true);

        // Результирующий гекс - пустой.
        res_hex = -1;
    }


    // Меняем выделенность стеков.

    // Строим список задетых стеков.
    H3Vector<H3CombatCreature*> sel_stacks;

    if (AreaShotDlg_CanCastAtHex(hex_ix))
    {
        //// Для луча джагернаутов - доп проверки
        //if (shooterType == 184 || shooterType == 185)
        //{
        //    sub_10100490(AreaShotDlg_Attacker, hex_ix, P_CombatManager->Get()); // см функцию по адресу
        //}
        //else
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
            H3CombatCreature* curr_stack = &P_CombatManager->stacks[side][i];

            // Это существо не мертво.
            if (!(curr_stack->info.cannotMove))
            {
                // Существо получит урон. - 100FCC8B
                if (curr_stack->position == hex_ix ||
                    ((curr_stack->info.doubleWide) && curr_stack->GetSecondSquare() == hex_ix) ||
                    ((shooterType == eCreature::MAGOG && !(curr_stack->info.fireImmunity) ||
                    (shooterType == eCreature::LICH || shooterType == eCreature::POWER_LICH || shooterType == 196) && curr_stack->info.alive)))
                {
                    // Выделяем стек.
                    //if (THISCALL_2(BOOL8, __thiscall, 0x43ED00, curr_stack, P_CombatManager->Field<_bool8_>(0x547C + 20 * side + i)))
                    if (THISCALL_2(BOOL8, 0x43ED00, curr_stack, P_CombatManager->massSpellTarget[side][i]))
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
            //if (shooterType == 184 || shooterType == 185)
            //{

            //}
            //else
            //{
                THISCALL_5(void, 0x5A4480, P_CombatManager->Get(), hex_ix, 1, true, &sel_hexes);
            //}

        }

        // Убираем иммунных существ из выделения.
        for (UINT32 i = 0; i < sel_hexes.Size(); ++i)
        {
            // Гекс корректен.
            if (sel_hexes[i] >= 0 && sel_hexes[i] < 187)
            {
                // Стек на гексе.
                H3CombatCreature* stack = P_CombatManager->squares[sel_hexes[i]].GetCreature();

                // Не выделяем гекс с иммунным стеком.
                if (stack && !stack->highlightContour)
                {
                    // Указатель на текущий элемент списка.
                    //int* p = &sel_hexes[i];

                    //// Удаляем элемент из списка.
                    //while (p < sel_hexes.EndData - 1)
                    //{
                    //    *p = *(p + 1);
                    //    p++;
                    //}
                    //sel_hexes.EndData--;

                    //continue;
                    sel_hexes.Remove(i);
                }
            }
        }

        // Подсвечиваем гексы.
        THISCALL_4(void, 0x493A20, P_CombatManager->Get(), hex_ix, &sel_hexes, true);

        // Нужна перерисовка.
        need_redraw = true;
    }

    // Убираем текущее выделение стеков.
    MemZero(P_CombatManager->massSpellTarget, 40); // 547C

    // Перерисовка.
    if (need_redraw)
    {
        bool BNRB = NeedRedrawBorders;
        NeedRedrawBorders = true;
        RedrawBattlefield(P_CombatManager->Get(), true, true, false, 0, true, false);
        NeedRedrawBorders = BNRB;
    }

    return res_hex;
}

// Выбор цели площадного выстрела.
void AreaShotDlg(H3CombatManager* b_mgr, H3CombatCreature* attacker)
{
    // Только для способных к площадному выстрелу существ.
    if (!attacker->info.fireballAttack) {
        return;
    }
    if (!CanShoot(attacker, 0)) {
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

    // Атакующий герой.
    H3Hero* att_hero = b_mgr->hero[attacker->GetSide()];

    // Есть ли лук снайпера у атакующего.
    AreaShotDlg_Has_Bow_S = att_hero && att_hero->WearsArtifact(eArtifact::BOW_OF_THE_SHARPSHOOTER);

    // Подсветка нужных гексов.

    // Если нужно подсвечивать гекс, сбрасываем подсветку.
    if (DwordAt(0x698810))
    {
        THISCALL_3(void, 0x493F10, b_mgr, -1, 0);
    }

    // Если есть тень перемещения - меняем её.
    if (DwordAt(0x698814))
    {
        // Надо ли затенять гексы.
        INT8* hex_msk = b_mgr->accessibleSquares;

        // Нет подсвеченых гексов.
        MemSet(hex_msk, 0, 187);

        // Магог.
        if (attacker->type == eCreature::MAGOG)
        {
            // Отмечаем доступные гексы-цели.
            for (int i = 0; i < 187; i++)
            {
                // Не крайние колонки.
                if (i % 17 != 0 && i % 17 != 16)
                {
                    // Сам в себя - нельзя.
                    if (i == attacker->position || attacker->info.doubleWide && i == attacker->GetSecondSquare())
                    {
                        continue;
                    }

                    // Нет лука снайпера.
                    if (!AreaShotDlg_Has_Bow_S)
                    {
                        // Нельзя бить в гексы рядом с собой.
                        if (HexesAreNear(b_mgr, attacker->position, i) || attacker->info.doubleWide && HexesAreNear(b_mgr, attacker->GetSecondSquare(), i))
                        {
                            continue;
                        }
                    }

                    // Там есть стек и можно колдовать на него.
                    H3CombatCreature* stack = b_mgr->squares[i].GetCreature();
                    if (stack)
                    {
                        if (!(stack->info.fireImmunity))
                        {
                            hex_msk[i] = 1;
                        }
                    }
                }
            }
        }
        // Лич.
        else
        {
            // Отмечаем доступные гексы-цели.
            for (int i = 0; i < 187; i++)
            {
                // Не крайние колонки.
                if (i % 17 != 0 && i % 17 != 16)
                {
                    // Сам в себя - нельзя.
                    if (i == attacker->position || attacker->info.doubleWide && i == attacker->GetSecondSquare())
                    {
                        continue;
                    }

                    // Нет лука снайпера.
                    if (!AreaShotDlg_Has_Bow_S)
                    {
                        // Нельзя бить в гексы рядом с собой.
                        if (HexesAreNear(b_mgr, attacker->position, i) || attacker->info.doubleWide && HexesAreNear(b_mgr, attacker->GetSecondSquare(), i))
                        {
                            continue;
                        }
                    }

                    // Там есть стек и можно колдовать на него.
                    H3CombatCreature* stack = b_mgr->squares[i].GetCreature();
                    if (stack)
                    {
                        if (stack->info.alive)
                        {
                            hex_msk[i] = 1;
                        }
                    }
                }
            }
        }
    }

    // Перерисовка поля боя.
    if ((DwordAt(0x698810) || DwordAt(0x698814)))
    {
        b_mgr->doNotDrawShade = 0;
        bool BNRB = NeedRedrawBorders;
        NeedRedrawBorders = true;
        RedrawBattlefield(b_mgr, true, false, false, 0, true, false);
        NeedRedrawBorders = BNRB;
    }

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

    // Диалог скрыт (как диалоги наложения заклинаний).
    Dlg_WasHidden = true;

    // Вызываем диалог.
    THISCALL_4(int, 0x602AE0, P_WindowManager->Get(), 0, AreaShotDlgMainFunc, false);
    Dlg_WasHidden = false;

    // Получаем координаты мыши.
    H3POINT::GetCursorPosition(mouse_x, mouse_y);
    // Выбранный гекс.
    hex_ix = b_mgr->SquareAtCoordinates(mouse_x - b_mgr->dlg->GetX(), mouse_y - b_mgr->dlg->GetY());

    // Выделяем стек на выбранном гексе.
    THISCALL_2(void, 0x477550, b_mgr, hex_ix);


    // Если была установлена новая подсветка - возвращаем обычную.

    // Если нужно подсвечивать гекс, сбрасываем подсветку.
    if (DwordAt(0x698810))
    {
        THISCALL_3(void, 0x493F10, b_mgr, -1, 0);
    }

    // Если есть тень перемещения - возвращаем её.ggg
    if (DwordAt(0x698814))
    {
        THISCALL_2(void, 0x493350, b_mgr, &b_mgr->stacks[b_mgr->currentMonSide][b_mgr->currentMonIndex]);
    }

    // Перерисовка поля боя.
    if ((DwordAt(0x698810) || DwordAt(0x698814)))
    {
        b_mgr->doNotDrawShade = false; // 53B8
        if (b_mgr->action == 0)
        {
            bool BNRB = NeedRedrawBorders;
            NeedRedrawBorders = true;
            RedrawBattlefield(b_mgr, true, false, false, 0, true, false);
            NeedRedrawBorders = BNRB;
        }
    }

    // Курсор - стрелка.
    P_MouseManager->SetCursor(6, 2);

}

bool IsAvailable(const H3CombatCreature *stack)
{
    return areaShotEnabled && stack && stack->info.fireballAttack &&
           ActionSwitcher::CanShootNormally(stack);
}

void SetEnabled(bool enabled)
{
    areaShotEnabled = enabled;
    if (!enabled)
        ShotType = ST_USUAL;
}

void OpenForActiveStack()
{
    H3CombatManager *manager = P_CombatManager->Get();
    if (manager && manager->activeStack && IsAvailable(manager->activeStack))
        AreaShotDlg(manager, manager->activeStack);
}

// Площадный выстрел по G.
_LHF_(LoHook_AreaShot_F)
{
    H3CombatManager* bm = reinterpret_cast<H3CombatManager*>(c->ebx);
    H3CombatCreature* stack = &bm->stacks[bm->currentMonSide][bm->currentMonIndex];

    // Only replace the native action when this stack can actually fire now.
    if (IsAvailable(stack))
    {
        // Ходит игрок за этим компьютером.

        //if (bm->Field<_bool32_>(0x132B4)) // 78516
        if (ValueAt<BOOL32>(int(bm) + 0x132B4))
        {
            AreaShotDlg(bm, stack);

            // Обработка после диалога.
            c->return_address = 0x474ACA;
        }
        else
        {
            // Пропускаем стандартную обработку.
            c->return_address = 0x475764;
        }

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

//// Тип текущего диалога стека (0 - обычный, 1 - сказочный дракон, 2 - магоги, 3 - личи).
//int curr_cr_dlg_type;
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
//            AreaShotDlg(P_CombatManager->Get(), &P_CombatManager->stacks[P_CombatManager->currentMonSide][P_CombatManager->currentMonIndex]);
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
//int __stdcall LoHook_ViewArmy_ActButtonCursor(LoHook* h, HookContext* c)
//{
//    // Кнопка активации способности.
//    if (c->eax == 15)
//    {
//        // Сказочный дракон.
//        if (curr_cr_dlg_type == 1)
//        {
//            c->edi = (_ptr_)misk_text[19]; // "Выбрать цель заклинания"
//        }
//        // Магог.
//        else if (curr_cr_dlg_type == 2)
//        {
//            c->edi = (_ptr_)misk_text[20]; // "Выбрать цель огненного шара"
//        }
//        // Лич.
//        else
//        {
//            c->edi = (_ptr_)misk_text[21]; // "Выбрать цель облака смерти"
//        }
//
//        // Пропускаем стандартную обработку.
//        c->return_address = 0x5F5336;
//
//        return NO_EXEC_DEFAULT;
//    }
//    else
//    {
//        return EXEC_DEFAULT;
//    }
//}

// Подменяем стек на элемент замка или гекс в функции отрисовки выстрела.
int __stdcall Hook_OnShotDrawEnemyPosGet(LoHook* h, HookContext* c)
{
    if (!areaShotEnabled)
        return EXEC_DEFAULT;

    H3CombatSquare* hex = *reinterpret_cast<H3CombatSquare**>(c->ebp + 0x8);

    if (ShotType == ST_WALL)
    {
        // TODO: тут нужна другая структура! hex подставлен с подходящими оффсетами наобум
        //H3ValidCatapultTargets

        H3CombatSquare* hex = *reinterpret_cast<H3CombatSquare**>(c->ebp + 0x8);

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
        H3CombatSquare* hex = *reinterpret_cast<H3CombatSquare**>(c->ebp + 0x8);
        // Берём координаты атакуемого гекса.
        IntAt(c->ebp - 0x14) = hex->x;
        c->edi = hex->y - 37; // Центр гекса вместо низа
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
int __stdcall Hook_OnMagicShotDrawEnemy_Y_PosGet(HiHook* h, DWORD* Tar)
{
    if (!areaShotEnabled)
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);

    if (ShotType == ST_WALL)
    {
        // TODO: тут нужна другая структура! hex подставлен с подходящими оффсетами наобум
        //H3ValidCatapultTargets

        // Берём координаты атакуемого элемента.
        return ((H3CombatSquare*)Tar)->top;
    }
    else if (ShotType == ST_HEX)
    {
        // Берём координаты атакуемого гекса.
        return ((H3CombatSquare*)Tar)->y;
    }
    else
    {
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);
    }
}

// Подменяем стек на элемент замка или гекс в функции отрисовки магического выстрела (горизонталь).
int __stdcall Hook_OnMagicShotDrawEnemy_X_PosGet(HiHook* h, DWORD* Tar)
{
    if (!areaShotEnabled)
        return THISCALL_1(int, h->GetDefaultFunc(), Tar);
    if (ShotType == ST_WALL)
    {
        // TODO: тут нужна другая структура! hex подставлен с подходящими оффсетами наобум
        //H3ValidCatapultTargets

        // Берём координаты атакуемого элемента.
        return ((H3CombatSquare*)Tar)->left;
    }
    else if (ShotType == ST_HEX)
    {
        // Берём координаты атакуемого гекса.
        return ((H3CombatSquare*)Tar)->x;
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
    patcher->WriteCodePatch(0x43F997, (char*)"0F 8E ~d", 0x43FA05);
    patcher->WriteCodePatch(0x43FD91, (char*)"7E 6E");

    // AI target selection and fire-immunity handling.
    patcher->WriteLoHook(0x41ED6D, LoHook_AISelectHex_LichMagogCast);
    patcher->WriteLoHook(0x41EF93, LoHook_Magog_FireImmun_AI);
    patcher->WriteLoHook(0x43F8E7, LoHook_Magog_FireImmun_Shot);

    // Retain the source hotkey and display its remapped G hint.
    patcher->WriteLoHook(0x4755F4, LoHook_AreaShot_F);
    patcher->WriteByte(0x4757C0 + 13, 8);
    patcher->WriteByte(0x4757C0 + 14, 1);

    // Draw projectile impacts at a wall or a battlefield hex.
    patcher->WriteLoHook(0x43F016, Hook_OnShotDrawEnemyPosGet);
    patcher->WriteHiHook(0x5A0111, CALL_, EXTENDED_, THISCALL_, Hook_OnMagicShotDrawEnemy_Y_PosGet);
    patcher->WriteHiHook(0x5A0119, CALL_, EXTENDED_, THISCALL_, Hook_OnMagicShotDrawEnemy_X_PosGet);
}

} // namespace aoe
} // namespace switcher
