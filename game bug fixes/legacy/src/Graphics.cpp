// исправление отображения графики тени Силового Поля
int __stdcall Fix_ForceFieldShadow(LoHook *h, HookContext *c)
{
    if (o_BattleMgr->actionParam == SPL_FORCE_FIELD)
    {                                    // если заклинание Силовое Поле
        IntAt(c->ebp - 0x14) = 0x63AC6C; // активная сторона тут всегда должна быть 0 (0x63AC6C всегда равна нулю!)
        c->return_address = 0x5A3699;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

// восстановление параметра тени курсора в бою
// при автобитве этот параметр обнуляется, но не восстанавливается
// значит восстановим вручную
int __stdcall Y_RestoreBattleShadow(LoHook *h, HookContext *c)
{
    o_Battle_CursorShadow = DwordAt(c->ebp - 0x1C);
    return EXEC_DEFAULT;
}

void ChangeStackValue(HookContext *c, int newValue)
{
    if (newValue < 1)
    {
        newValue *= -1;
    }
    const int stackData[3] = {c->Pop(), c->Pop(), c->Pop()};
    // удаляем потдип картинки из стека (0)
    c->Pop();
    // помещаем новое значение удачи в стек
    c->Push(newValue);

    // восстанавливаем стек
    c->Push(stackData[2]);
    c->Push(stackData[1]);
    c->Push(stackData[0]);
}
_LHF_(DlgCreatureInfo_AtRightClick)
{

    if (const int picType = c->edx)
    {
        INT bonusValue = 0;

        switch (picType)
        {
        case 11: // если клик по удаче (которая +/-)
        case 13:
            bonusValue = IntAt(c->ebx + 0x7C);
            break;
        case 14: // если клик по морали (которая +/-)
        case 16:
            bonusValue = IntAt(c->ebx + 0x68);
            break;
        default:
            break;
        }

        // если надо менять картинку
        if (bonusValue > 1 || bonusValue < -1)
        {
            ChangeStackValue(c, bonusValue);
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(Hero_LuckClick)
{
    // если клик по удаче (которая +/-)
    if (c->ebx == 11 || c->ebx == 13)
    {
        const int luckBonus =
            CALL_4(int, __thiscall, 0x04E3930, DwordAt(c->ebp + 0x8), 0, 0, 1); // получаем бонус удачи

        if (luckBonus > 1 || luckBonus < -1)
        {
            ChangeStackValue(c, luckBonus);
        }
    }

    return EXEC_DEFAULT;
}
int moraleValue = 0;
_LHF_(Hero_MoraleClickStart)
{
    moraleValue = c->edi;
    return EXEC_DEFAULT;
}

_LHF_(Hero_MoraleClick)
{
    // если клик по морали (которая +/-)
    if (c->eax == 14 || c->eax == 16)
    {
        if (moraleValue > 1 || moraleValue < -1)
        {
            ChangeStackValue(c, moraleValue);
        }
    }

    return EXEC_DEFAULT;
}

// © daemon_n
// Исправление отображения текста с количеством существ в портрете существа в диалоге гарнизона
int __cdecl CreaturePortraitText_sprintf(HiHook *h, char *buffer, const char *format, const int creaturesNum)
{
    if (creaturesNum < 100000)
    {
        return CALL_3(int, __cdecl, h->GetDefaultFunc(), buffer, format, creaturesNum);
    }
    return Era::FormatQuantity(creaturesNum, buffer, 32, 5, 3);
}

// @ JackSlater
// исправление индекса слота в диалоге продажи артефактов
// hd mod ранее ставил слот 9 вместе 18 ( что дублировало 9-й слот)

// Исправляем попытку работать с диалогом рынка при недопустимых координатах.
_int32_ __stdcall HiHook_MarketDlgDefProc(HiHook *h, _Dlg_ *this_, _EventMsg_ *msg)
{
    // DefProc
    _int32_ res = CALL_2(_int32_, __thiscall, h->GetDefaultFunc(), this_, msg);

    // Будет работать основная функция.
    if (!res)
    {
        // Не даём работать с недопустимыми координатами.
        if ((msg->type == 4) && (this_->FindItemID(msg->x_abs, msg->y_abs) < 0))
        {
            return 1;
        }
        else
        {
            return res;
        }
    }
    else
    {
        return res;
    }
}

// Исправляем отсутствие кнопки покупки артефактов в сопряжении - рынок.
int __stdcall LoHook_ConfluxArtMer_Res(LoHook *h, HookContext *c)
{
    // Город.
    _Town_ *town = (_Town_ *)c->ecx;

    // Сопряжение.
    if (town->type == 8)
    {
        c->return_address = 0x5EAEEC;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Исправляем отсутствие кнопки покупки артефактов в сопряжении - отправка.
int __stdcall LoHook_ConfluxArtMer_Send(LoHook *h, HookContext *c)
{
    // Город.
    _Town_ *town = (_Town_ *)c->ecx;

    // Сопряжение.
    if (town->type == 8)
    {
        c->return_address = 0x5EB6DE;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

constexpr int ARTSELL_SLOT5_ID = 1006;
constexpr int ARTSELL_SLOT5_FRAME_ID = 1007;
// Окна рынка.
// Рамка выделения кнопок переключения окон рынка (0-3 элемента: 1000, 1001, 1002)
#define MARKET_CHANGEWNDBUTTON_FRAME_ID 1000
// Рамка выделения кнопки ОК.
#define MARKET_OKBUTTON_FRAME_ID 1003
// Рамка выделения кнопки максимум.
#define MARKET_MAXBUTTON_FRAME_ID 1004
// Рамка выделения кнопки торговли.
#define MARKET_TRADEBUTTON_FRAME_ID 1005
// 5-й слот разного в продаже артефактов.
#define ARTSELL_SLOT5_ID 1006
// Обводка 5-го слота разного в продаже артефактов.
#define ARTSELL_SLOT5_FRAME_ID 1007
#define o_Market_BackpackIndexOfFirstSlot (*(_int8_ *)0x6AAAD8)
#define o_Market_SelectedSlotIndex *(_int32_ *)0x6AAAF8
#define o_Market_SelectedBackpackSlotIndex (o_Market_SelectedSlotIndex - 19) // Для HotA -19, для оригинала -18

// Добавление 5 слота разного в окно продажи артефактов.
int __stdcall LoHook_ArtSell_Add5Slot(LoHook *h, HookContext *c)
{
    // Список элементов диалога.
    _List_<_DlgItem_ *> *lst = (_List_<_DlgItem_ *> *)c->esi;

    // Добавляем элемент.
    lst->Append(_DlgStaticDef_::Create(20, 314, 44, 44, ARTSELL_SLOT5_ID, "Artifact.def", 0, 0, 0));

    return EXEC_DEFAULT;
}

// Добавление рамки 5 слота разного в окно продажи артефактов.
int __stdcall LoHook_ArtSell_Add5SlotFrame(LoHook *h, HookContext *c)
{
    // Список элементов диалога.
    _List_<_DlgItem_ *> *lst = (_List_<_DlgItem_ *> *)c->esi;

    // Добавляем элемент.
    lst->Append(_DlgStaticPcx8_::Create(17, 311, 48, 48, ARTSELL_SLOT5_FRAME_ID, "TPMrkSe3.pcx"));

    return EXEC_DEFAULT;
}

// Добавление 5 слота разного в окно продажи артефактов - подсказка.
int __stdcall LoHook_ArtSell_Add5Slot_Command(LoHook *h, HookContext *c)
{
    if (c->eax == ARTSELL_SLOT5_FRAME_ID)
    {
        // Берём имя артефакта.
        c->edi = (_ptr_)o_ArtInfo[o_Market_Hero->doll_art[AS_MISC_5].id].name;

        c->return_address = 0x5EEC19;

        return EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Добавление 5 слота разного в окно продажи артефактов - описание.
int __stdcall LoHook_ArtSell_Add5Slot_Descr(LoHook *h, HookContext *c)
{
    if (c->eax == ARTSELL_SLOT5_FRAME_ID)
    {
        // Берём слот.
        c->esi = AS_MISC_5;

        c->return_address = 0x5EE3D2;

        return NO_EXEC_DEFAULT;
    }
    else if (c->eax - 107 >= AS_MISC_5 && c->eax - 107 < 23)
    {
        // Берём слот.
        c->esi = c->eax - 107 + 1;

        c->return_address = 0x5EE3D2;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}
// Добавление 5 слота разного в окно продажи артефактов - выбор.
int __stdcall LoHook_ArtSell_Add5Slot_Select(LoHook *h, HookContext *c)
{
    if (c->ecx == ARTSELL_SLOT5_FRAME_ID)
    {
        // Берём слот.
        c->eax = AS_MISC_5;

        c->return_address = 0x5EE880;

        return NO_EXEC_DEFAULT;
    }
    else if (c->ecx - 107 >= AS_MISC_5 && c->ecx - 107 < 23)
    {
        // Берём слот.
        c->eax = c->ecx - 107 + 1;

        c->return_address = 0x5EE880;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}

// Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (1).
int __stdcall LoHook_ArtSell_Add5Slot_ArtUpd1(LoHook *h, HookContext *c)
{
    if (c->edi == AS_MISC_5)
        c->ecx = ARTSELL_SLOT5_ID;
    else if (c->edi > AS_MISC_5)
        c->ecx--;

    return EXEC_DEFAULT;
}

// Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (2).
int __stdcall LoHook_ArtSell_Add5Slot_ArtUpd2(LoHook *h, HookContext *c)
{
    if (c->edi == AS_MISC_5)
        c->edx = ARTSELL_SLOT5_ID;
    else if (c->edi > AS_MISC_5)
        c->edx--;

    return EXEC_DEFAULT;
}

// Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (3).
int __stdcall LoHook_ArtSell_Add5Slot_ArtUpd3(LoHook *h, HookContext *c)
{
    if (c->edi == AS_MISC_5 + 107)
        c->edi = ARTSELL_SLOT5_FRAME_ID;
    else if (c->edi > AS_MISC_5 + 107)
        c->edi--;

    return EXEC_DEFAULT;
}

// Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (4).
int __stdcall LoHook_ArtSell_Add5Slot_ArtUpd4(LoHook *h, HookContext *c)
{
    if (c->esi == AS_MISC_5)
        c->edx = ARTSELL_SLOT5_FRAME_ID;
    else if (c->esi > AS_MISC_5)
        c->edx--;

    return EXEC_DEFAULT;
}

// Выделение следующего артефакта при продаже.
int __stdcall LoHook_ArtSell_Add5Slot_ArtSellSelect(LoHook *h, HookContext *c)
{
    // Арт в рюкзаке.
    if (o_Market_SelectedSlotIndex >= 19)
    {

        // Новый артефакт на этом месте.
        _int_ art_id = o_Market_Hero
                           ->backpack_art[(o_Market_BackpackIndexOfFirstSlot + o_Market_SelectedBackpackSlotIndex) %
                                          max(o_Market_Hero->BackpackArtsCount(TRUE), 5)]
                           .id;
        if (art_id > 0)
        {
            // Цена артефакта.
            _float_ cost = static_cast<_float_>(o_ArtInfo[art_id].cost);

            // Цена ресурса.
            _int16_ res_cost = ((_int16_ *)0x68C4D2)[IntAt(0x6AAB34)];

            cost = cost * (((_float_ *)0x678370)[IntAt(0x6AAB00)]) / ((_float_)(double)res_cost);
            if (cost < 1.0f)
            {
                cost = 1.0f;
            }

            IntAt(0x6AAAE8) = 1;
            IntAt(0x6AAB28) = ((_int_)(_int64_)(cost + 0.5f));
            IntAt(0x6AAAFC) = 1;
            IntAt(0x6AAB18) = 1;

            // Переходим к обновлению экрана.
            c->return_address = 0x5EE522;

            return NO_EXEC_DEFAULT;
        }
        else
        {
            return EXEC_DEFAULT;
        }
    }
    else
    {
        return EXEC_DEFAULT;
    }
}
// Исправление бага с продажей артефактов не по своей цене.
int __stdcall LoHook_FixArtMerchantBackpack(LoHook *hook, HookContext *c)
{
    // Если артефактов меньше 6, учитываем, что могут быть и пустые слоты.
    _int_ i = (o_Market_BackpackIndexOfFirstSlot + o_Market_SelectedBackpackSlotIndex) %
              max(o_Market_Hero->BackpackArtsCount(TRUE), 5);

    // Берём информацию о нужном артефакте.
    c->ecx = o_Market_Hero->backpack_art[i].id;
    c->eax = c->edx = o_Market_Hero->backpack_art[i].mod;

    // Пропускаем стандартное взятие.
    c->return_address += 7;
    return NO_EXEC_DEFAULT;
}
// Исправление бага с продажей артефактов не по своей цене (2).
int __stdcall LoHook_FixArtMerchantBackpack2(LoHook *hook, HookContext *c)
{
    // Если артефактов меньше 6, учитываем, что могут быть и пустые слоты.
    _int_ i = (o_Market_BackpackIndexOfFirstSlot + c->edi - 19) % max(o_Market_Hero->BackpackArtsCount(TRUE), 5);

    // Берём информацию о нужном артефакте.
    c->ebx = o_Market_Hero->backpack_art[i].id;
    IntAt(c->ebp - 8) = o_Market_Hero->backpack_art[i].mod;

    // Пропускаем стандартное взятие.
    c->return_address = 0x5EAA4E;
    return NO_EXEC_DEFAULT;
}

// Исправление багов продажи артефактов при неполных 6 видимых слотах рюкзака.
_int_ __stdcall HiHook_ArtSell_BackpackArtsCount(HiHook *h, _Hero_ *this_, _bool8_ calc_war_mashines)
{
    // Стандартный подсчёт.
    _int_ count = CALL_2(_int_, __thiscall, h->GetDefaultFunc(), this_, calc_war_mashines);

    // Не меньше кол-ва открытых слотов в рюкзаке - 5.
    return max(count, 5);
}
// Подсчёт количества артефактов + свободных слотов в рюкзаке - для корректного отключения стрелок.
_int_ __stdcall HiHook_ArtSell_BackpackArtsCount_DisableArrows(HiHook *h, _Hero_ *this_, _bool8_ calc_war_mashines)
{
    // Стандартный подсчёт.
    _int_ count = CALL_2(_int_, __thiscall, h->GetDefaultFunc(), this_, calc_war_mashines);

    // Добавляем пустые слоты рюкзака.
    for (_int_ i = 0; i < 5; i++)
    {
        if (this_->backpack_art[i].id < 0)
            count++;
    }

    return count;
}
// Заменяем способ прокрутки рюкзака влево в продаже артефактов.
int __stdcall LoHook_ArtSell_BackpackLeft(LoHook *hook, HookContext *c)
{
    // Двигаем артефакты.
    c->ecx = (_ptr_)o_Market_Hero;
    CALL_1(void, __thiscall, 0x4DC020, o_Market_Hero);

    // Пропускаем стандартную прокрутку.
    c->return_address = 0x5EE58F;

    return NO_EXEC_DEFAULT;
}

// Заменяем способ прокрутки рюкзака вправо в продаже артефактов.
int __stdcall LoHook_ArtSell_BackpackRight(LoHook *hook, HookContext *c)
{
    // Двигаем артефакты.
    c->ecx = (_ptr_)o_Market_Hero;
    CALL_1(void, __thiscall, 0x4DC080, o_Market_Hero);

    // Пропускаем стандартную прокрутку.
    c->return_address = 0x5EE6CC;

    return NO_EXEC_DEFAULT;
}
// Прокрутка рюкзака влево в продаже артефактов - сбрасываем выделение пустого слота.
int __stdcall LoHook_ArtSell_Backpack_ResetEmptySlotSelection(LoHook *hook, HookContext *c)
{
    // Нет арта в выбранном слоте - сбрасываем выделение.
    if (c->ecx < 0)
    {
        // Нет выбранного ресурса.
        IntAt(0x6AAB34) = -1;

        // Нет выбранного артефакта.
        o_Market_SelectedSlotIndex = -1;

        // Пропускаем обновление цен.
        c->return_address = 0x5EE522;

        return NO_EXEC_DEFAULT;
    }
    // Есть арт в выбранном слоте - обновляем цены.
    else
    {
        return EXEC_DEFAULT;
    }
}
// Исправляем баг ИИ с неспособностью пользоваться торговцами артефактов в Сопряжении.
int __stdcall LoHook_FixAIConflArtMercant(LoHook *h, HookContext *c)
{
    // У сопряжения есть торговцы артефактами.
    if ((c->eax & 0xFF) == 8) // al
    {
        // Восстанавливаем затёртую команду.
        PtrAt(c->ebp - 36) = c->ecx;

        c->return_address = 0x525EE0;

        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}
// также отменяем патчи HD Mod'а
_LHF_(HookAfterHDModInit)
{
    // UNDO патчи на 5 слот HD-мода

    //  auto globalPatcher = GetPatcher();

    auto hdWog = _P->GetInstance("HD.WoG");
    if (hdWog)
    {

        hdWog->UndoAllAt(0x5E5B30);
        hdWog->UndoAllAt(0x5EA9D0);
        hdWog->UndoAllAt(0x5EC280);
        hdWog->UndoAllAt(0x5EE360);
    }
    auto wog = _P->GetInstance("WoG");
    if (wog)
    {
        wog->UndoAllAt(0x5EE9E0);
    }

    // + Исправляем попытку работать с диалогом рынка при недопустимых координатах.
    _PI->WriteHiHook(0x5ED3EF, CALL_, EXTENDED_, THISCALL_, HiHook_MarketDlgDefProc); // Ресурсы
    _PI->WriteHiHook(0x5ED95F, CALL_, EXTENDED_, THISCALL_, HiHook_MarketDlgDefProc); // Отправка ресурсов
    _PI->WriteHiHook(0x5EDDF2, CALL_, EXTENDED_, THISCALL_, HiHook_MarketDlgDefProc); // Покупка артеактов
    _PI->WriteHiHook(0x5EE372, CALL_, EXTENDED_, THISCALL_, HiHook_MarketDlgDefProc); // Продажа артефактов
    _PI->WriteHiHook(0x5EEC9C, CALL_, EXTENDED_, THISCALL_, HiHook_MarketDlgDefProc); // Продажа существ

    // Исправляем отсутствие кнопки покупки артефактов в сопряжении.
    _PI->WriteLoHook(0x5EAEE1, LoHook_ConfluxArtMer_Res);  // Рынок
    _PI->WriteLoHook(0x5EB6D3, LoHook_ConfluxArtMer_Send); // Передача

    // Добавляем 5 слот разного в окно продажи артефактов.

    // Добавление рамки 5 слота разного в окно продажи артефактов.
    _PI->WriteLoHook(0x5E6F1F, LoHook_ArtSell_Add5Slot);
    // Добавление 5 слота разного в окно продажи артефактов.
    _PI->WriteLoHook(0x5E7E82, LoHook_ArtSell_Add5SlotFrame);

    // 5-й слот - выделенный артефакт.
    _PI->WriteByte(0x5EE4C5 + 2, 19); // Продажа артефакта
    _PI->WriteByte(0x5EE4F8 + 3, -19);
    _PI->WriteByte(0x5EE601 + 2, 19); // Прокрутка рюкзака назад
    _PI->WriteByte(0x5EE73E + 2, 19); // Прокрутка рюкзака вперёд
    _PI->WriteByte(0x5EE3D5 + 2, 19); // Описание по ПКМ
    _PI->WriteByte(0x5EE430 + 3, -19);
    _PI->WriteByte(0x5ED1CF + 2, 19); // Цена артефакта
    _PI->WriteByte(0x5EE8A8 + 2, 19); // Выбор артефакта
    _PI->WriteByte(0x5EC2DA + 2, 19); // Обновление диалога - название артефакта
    _PI->WriteByte(0x5EC30D + 3, -19);
    _PI->WriteByte(0x5EC656 + 2, 19);
    _PI->WriteByte(0x5EC694 + 3, -19); // Обновление диалога - выделение
    _PI->WriteByte(0x5EC7DD + 2, 19);  // Обновление диалога - цена
    _PI->WriteByte(0x5EA9DC + 2, 19);  // Обновление артефакта
    _PI->WriteByte(0x5EC916 + 2, 24);  // Обновление артефактов - общее количество слотов

    // Добавление 5 слота разного в окно продажи артефактов - подсказка.
    _PI->WriteLoHook(0x5EEB48, LoHook_ArtSell_Add5Slot_Command);

    // Добавление 5 слота разного в окно продажи артефактов - описание.
    _PI->WriteLoHook(0x5EE3BA, LoHook_ArtSell_Add5Slot_Descr);

    // Добавление 5 слота разного в окно продажи артефактов - выбор.
    _PI->WriteLoHook(0x5EE81A, LoHook_ArtSell_Add5Slot_Select);

    // Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (1).
    _PI->WriteLoHook(0x5EAA59, LoHook_ArtSell_Add5Slot_ArtUpd1);
    // Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (2).
    _PI->WriteLoHook(0x5EAA7B, LoHook_ArtSell_Add5Slot_ArtUpd2);
    // Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (3).
    _PI->WriteLoHook(0x5EAAC0, LoHook_ArtSell_Add5Slot_ArtUpd3);
    // Добавление 5 слота разного в окно продажи артефактов - обновление артефакта (4).
    _PI->WriteLoHook(0x5EC8EA, LoHook_ArtSell_Add5Slot_ArtUpd4);

    // Выделение следующего артефакта при продаже.
    _PI->WriteLoHook(0x5EE50B, LoHook_ArtSell_Add5Slot_ArtSellSelect);

    // Исправление бага с продажей артефактов не по своей цене.
    _PI->WriteLoHook(0x5EE619, LoHook_FixArtMerchantBackpack);
    _PI->WriteLoHook(0x5EE756, LoHook_FixArtMerchantBackpack);
    _PI->WriteLoHook(0x5EE8C0, LoHook_FixArtMerchantBackpack);
    _PI->WriteLoHook(0x5EC7F5, LoHook_FixArtMerchantBackpack);
    _PI->WriteLoHook(0x5ED1E7, LoHook_FixArtMerchantBackpack);
    _PI->WriteLoHook(0x5EA9F9, LoHook_FixArtMerchantBackpack2);

    // Исправление багов продажи артефактов при неполных 6 видимых слотах рюкзака.
    _PI->WriteHiHook(0x5EC2F5, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);
    _PI->WriteHiHook(0x5EC67C, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);
    _PI->WriteHiHook(0x5EE41B, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);
    _PI->WriteHiHook(0x5EE4E0, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);
    _PI->WriteHiHook(0x5EEBAB, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);
    _PI->WriteHiHook(0x5EE58F, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);
    _PI->WriteHiHook(0x5EE6CC, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount);

    // Подсчёт количества артефактов + свободных слотов в рюкзаке - для корректного отключения стрелок.
    _PI->WriteHiHook(0x5EE55D, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount_DisableArrows);
    _PI->WriteHiHook(0x5EE69F, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount_DisableArrows);
    _PI->WriteHiHook(0x5EC5E8, CALL_, EXTENDED_, THISCALL_, HiHook_ArtSell_BackpackArtsCount_DisableArrows);

    // Заменяем способ прокрутки рюкзака влево в продаже артефактов.
    _PI->WriteLoHook(0x5EE57C, LoHook_ArtSell_BackpackLeft);
    // Заменяем способ прокрутки рюкзака вправо в продаже артефактов.
    _PI->WriteLoHook(0x5EE6BC, LoHook_ArtSell_BackpackRight);

    // Прокрутка рюкзака влево в продаже артефактов - сбрасываем выделение пустого слота.
    _PI->WriteLoHook(0x5EE62A, LoHook_ArtSell_Backpack_ResetEmptySlotSelection);
    // Прокрутка рюкзака вправо в продаже артефактов - сбрасываем выделение пустого слота.
    _PI->WriteLoHook(0x5EE767, LoHook_ArtSell_Backpack_ResetEmptySlotSelection);
    // Исправляем баг ИИ с неспособностью пользоваться торговцами артефактов в Сопряжении.
    _PI->WriteLoHook(0x525ED1, LoHook_FixAIConflArtMercant);

    return EXEC_DEFAULT;
}
// При показе списка карт обновляем выбранную карту (баг SoD с пропаданием выбора игрока).
void __stdcall HiHook_MapList_UpdateMap(HiHook *h, _Dlg_ *this_, _bool8_ rand_maps)
{
    // Показываем список карт.
    CALL_2(void, __thiscall, h->GetDefaultFunc(), this_, rand_maps);

    // Обновляем карту.
    CALL_3(void, __thiscall, 0x5857D0, this_, this_->Field<_int32_>(884), FALSE);
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void Graphics(PatcherInstance *_PI)
{
    // центрирование изображения по ПКМ в городе на иконке найма войск (ранее уходило сильно влево)
    _PI->WriteHexPatch(0x5D47B3, "0F BF 57 18 8B 4F 24 B8 FF FF FF FF 90");

    // [центрируем текст названия города по вертикали в окне города(id 149)]
    _PI->WriteByte(0x5C5C1B, 4);

    // отключить окно-сообщение тактики в начале боя (просто пропускаем его показ)
    _PI->WriteHexPatch(0x462D98, "EB 30 90 90 90 90"); // 0x462D98 JMP SHORT 0x462DCA

    // фикс невлезающего кол-ва существ (100-249 и т.п.) в маленьком окне героя ПКМ
    _PI->WriteByte(0x52F7CE + 1, 34); // герой
    _PI->WriteByte(0x5310F1 + 1, 34); // замок, гарнизон

    // исправить координаты кнопки Сказочных Драконов
    _PI->WriteDword(0x5F3D9F, 235); // подложка поз.Y
    _PI->WriteByte(0x5F3DA4, 21);   // подложка поз.X
    _PI->WriteDword(0x5F3DF5, 235); // кнопка   поз.Y
    _PI->WriteByte(0x5F3DFA, 21);   // кнопка   поз.X

    // исправление неправильных иконок героев Инферно (Ксерафакс и Ксерон)
    _PI->WriteDword(0x79984C, 63);
    _PI->WriteDword(0x799850, 57);

    // исправление неправильных кнопок
    // в диалоге таверны
    _PI->WriteDword(0x5D7ACA, 0x682A24); // iCN6432.def
    // в диалоге резделения отрядов
    _PI->WriteDword(0x449A41, 0x682A24); // iCN6432.def
    // в диалоге преобразователя скелетов
    _PI->WriteDword(0x565E4A, 0x682A24); // iCN6432.def

    // смещение портрета героя в диалоге повышения уровня героя
    _PI->WriteDword(0x4F90CB + 1, 0xAA);

    // исправление ошибки ERM в командре IF:N1, теперь командра работает
    // со всеми локальными, глобальными и отрицательными переменными z, а не только с z1
    _PI->WriteByte(0x749093, 0xB0);
    _PI->WriteByte(0x74909C, 0xB0);
    _PI->WriteByte(0x7490B0, 0xB0);
    _PI->WriteByte(0x7490B6, 0xB0);
    _PI->WriteByte(0x7490CD, 0xB0);

    // Решение проблемы отображения некоторых строк (в русской локализации) в диалоге экспы монстров.
    // Суть в подмене типа копирования символов со знакового на беззнаковое ( MOVSX -> MOVZX )
    _PI->WriteByte(0x71F3FC, 0xB6);
    _PI->WriteByte(0x71F5BA, 0xB6);
    _PI->WriteByte(0x71F5D3, 0xB6);
    _PI->WriteByte(0x7217BB, 0xB6);
    _PI->WriteByte(0x721B03, 0xB6);
    _PI->WriteByte(0x722792, 0xB6);
    _PI->WriteByte(0x723219, 0xB6);
    _PI->WriteByte(0x723657, 0xB6);
    _PI->WriteByte(0x7238D8, 0xB6);
    _PI->WriteByte(0x723ACB, 0xB6);
    _PI->WriteByte(0x723CBD, 0xB6);
    _PI->WriteByte(0x723F1C, 0xB6);

    // исправление включения тени, которое не выполняется при автобитве
    _PI->WriteLoHook(0x462C6C, Y_RestoreBattleShadow);

    // исправление отображения графики тени Силового Поля
    _PI->WriteLoHook(0x5A368C, Fix_ForceFieldShadow);
    // включить показ тени для Силового Поля (by RoseKavalier)
    _PI->WriteJmp(0x5A365D, 0x5A3666);
    _PI->WriteJmp(0x5A37B9, 0x5A37C2);

    // восстановить описание по ПКМ на правую кнопку перелистывания артефактов в рюкзаке героя (© daemon_n)
    _PI->WriteDword(0x5641A2 + 2, 0x6A673C);

    // Фикс размера и положения текстового виджета с отображением цены для ПКМ на жилище существ
    // (ранее влезало только 4 символа) (© daemon_n)
    _PI->WriteByte(0x55205E + 1, 40);
    _PI->WriteByte(0x552060 + 2, 36);

    // Исправление описания текста морали для Ангелов (© Hawaiing)
    _PI->WriteCodePatch(0x760A4F, "%n", 5); // 5 nops


    _PI->WriteByte(0x4A8BBE + 1, 2);

    // ПКМ в диалоге существа (работает на удачу и мораль)
    _PI->WriteLoHook(0x05F4E7F, DlgCreatureInfo_AtRightClick);

    // Пкм/ЛКМ на иконках героя из разных диалогов
    _PI->WriteLoHook(0x04F3CD2, Hero_LuckClick);
    _PI->WriteLoHook(0x04F396A, Hero_MoraleClickStart);
    _PI->WriteLoHook(0x04F3B58, Hero_MoraleClick);

    // В святынях и пирамидах выделяем заклинание жёлтым вместо кавычек.
    _PI->WriteByte(0x677750 + 2, '{'); // 100B1DEF
    _PI->WriteByte(0x677750 + 5, '}');

    // © daemon_n
    // Рисуем тени для всех объектов на Карте Приключений (даже тех, у которых стоит флаг "is_flat")
    _PI->WriteHexPatch(0x041175B, "EB 09 90"); // jump short + nop

    // © daemon_n
    // Исправление отображения текста с количеством существ в портрете существа в диалоге гарнизона (ранее
    // отображалось только 5 символов)

    // окно героя (большой портрет)
    _PI->WriteHiHook(0x04D9B44, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);

    // окно встречи героев
    _PI->WriteHiHook(0x05B136C, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);

    // любое окно гарнизона
    _PI->WriteHiHook(0x05AA140, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf); // CreaturePortraitText_sprintf

    // окно форта на холме
    _PI->WriteHiHook(0x04E8031, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf); // CreaturePortraitText_sprintf

    // битва
    _PI->WriteHiHook(0x046D966, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf); // предпросмотр юнита в битве
    _PI->WriteHiHook(0x043E482, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf); // ярлык с количеством юнитов

    // Окно обзора королевства
    // расширяем поле текста для отображения количества существ для юнитов в гарнизоне
    // _PI->WriteByte(0x051CB3F + 1, 0x1C);  // ширина
    // _PI->WriteByte(0x051CB41 + 1, 0x54);  // x-позиция
    // расширяем поле текста для отображения количества существ для юнитов у героя
    _PI->WriteDword(0x051DB0F + 1, 0x1F); // x-позиция для def и txt - потому что управляет обоими сразу
    _PI->WriteByte(0x051DC2D + 1, 0x1C);  // ширина
    _PI->WriteByte(0x051DB7B + 2, 0xFD);  // x-позиция для def

    _PI->WriteHiHook(0x051CAED, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051CD5E, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051D096, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051D1E5, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051D4EE, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051D810, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051DBE8, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);
    _PI->WriteHiHook(0x051DE00, CALL_, EXTENDED_, CDECL_, CreaturePortraitText_sprintf);

    // При показе списка карт обновляем выбранную карту (баг SoD с пропаданием выбора игрока).
    _PI->WriteHiHook(0x587272, CALL_, EXTENDED_, THISCALL_, HiHook_MapList_UpdateMap);

    // исправление индекса слота в диалоге продажи артефактов
    // hd mod ранее ставил слот 9 вместе 18 ( что дублировало 9-й слот)
    // также отменяем патчи HD Mod'а
    // ПОКА НЕ РАБОТАЕТ
    //   _PI->WriteLoHook(0x4EEAF2, HookAfterHDModInit);
}
