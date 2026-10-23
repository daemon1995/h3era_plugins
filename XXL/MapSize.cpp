// XXL map-size hooks, ported from JackSlater/XXL.

#include "pch.h"



//RunTemplatesListDlg 011533F0

//адрес для чека инфы 00586921, см EAX
//
//itemId справа :
//кадр дефа размера выбранной карты : 189
//текст "название миссии" : 100 (похоже что не так)
//текст имени карты : 100 - без открытых вкладок, 101 - сценарий, 102 - дополнительные опции, 103 - случайная
//0057BCCC
//
//доступные сценарии : 128
//дополнительные опции : 129
//случайная карта : 130
//hw опции: 131
//текст "описание миссии" : 105 (точно ли ? )
//текст описания миссии : FFFFFFFF
//сложность с пешки до короля : 107 108 109 110 111

//еще опции : ? ? ? (не триггерит LeftClickProc - в хуке хд - мода до этого следует своя логика)
//начать : 186
//выйти : 188
//
//
//itemId в random map :
//текст template слева от hd вкладки : 7003
//сам скролл вкладки с шаблонами??: 7001
//hd вкладка с шаблонами : 7002 (не триггерит LeftClickProc - в хуке хд - мода до этого следует своя логика)
//размер карты с S по Undergroung : 281 282 283 284 3003 3004 3005 285
//игроки(люди или ии) с 1 по рандом : 287 288 289 290 291 292 293 294 295
//игроки(только ии) с 0 по рандом : 307 308 309 310 311 312 313 314 315
//командные соглашения : ? ? ? (не триггерит LeftClickProc - в хуке хд - мода до этого следует своя логика)
//тип дорог : 7007 7008 7009 (hd id - шники ? ? ? )
//доля воды с без воды по рандом : 326 327 328 329
//сила монстров со слабые по рандом : 331 332 333 334
//сгенерированные карты : 335
//
//
//itemId в available scenarios :
//фильтры по размеру с S по все : 137 138 139 140 3000 3001 3002 141
//фильтры по игрокам до условий поражения : 190 191 192 193 194 195
//выбранная карта с верхней по нижнюю : 142 - 159
//слайдер карт вниз - вверх : 151 (в обе стороны)
//
//
//itemId в show advanced options :
//флаги с красного по розовый : 263 264 265 266 267 268 269 270
//человек или ии : 102 (? ? ? )
//стрелка влево смены города : 215 216 217 218 219 220 221 222
//стрелка вправо смены города : 223 224 225 226 227 228 229 230
//стрелка влево смены героя : 231 232 233 234 235 236 237 238
//стрелка вправо смены героя : 239 240 241 242 243 244 245 246
//стрелка влево смены бонуса : 247 248 249 250 251 252 253 254
//стрелка вправо смены бонуса : 255 256 257 258 259 260 261 262
//таймер(вкладка) : 2705 (не триггерит LeftClickProc - в хуке хд - мода до этого следует своя логика)
//слайдер таймер классический : 338 (в обе стороны)



//char byte_104A779C = 0;
//char byte_104A7440 = 0;

LPCSTR BTN_RMC_HINT_KEY = "XXL.rmcHint.%d";
LPCSTR COLUMN_MAP_SIZE_KEY = "XXL.mapSizeColumn.%d";


// Размеры карт (одна сторона)
#define MAPSIZE_S   36
#define MAPSIZE_M   72
#define MAPSIZE_L  108
#define MAPSIZE_XL 144
#define MAPSIZE_H  180
#define MAPSIZE_XH 216
#define MAPSIZE_G  252


// itemId для сценариев

// Выбор карты.
// Номер элемента диалоге - просмотр H-карт.
#define HMAP_DLGITEM_ID 3000
// Номер элемента диалоге - просмотр XH-карт.
#define XHMAP_DLGITEM_ID 3001
// Номер элемента диалоге - просмотр G-карт.
#define GMAP_DLGITEM_ID 3002


// itemId для случайной карты

// Номер элемента диалоге - просмотр H случайных карт.
#define HRMAP_DLGITEM_ID 3003
// Номер элемента диалоге - просмотр XH случайных карт.
#define XHRMAP_DLGITEM_ID 3004
// Номер элемента диалоге - просмотр G случайных карт.
#define GRMAP_DLGITEM_ID 3005


void SetRmcHint(H3DlgItem* item, BOOL8 alloc_mem)
{
    libc::sprintf(h3_TextBuffer, BTN_RMC_HINT_KEY, item->GetID());
    LPCSTR rmcHint = EraJS::read(h3_TextBuffer);
    item->SetHints(NULL, rmcHint, alloc_mem);
}



_LHF_(LoHook_MapSelectDlg_Create_SizeButtons)
{
    // Список элементов диалога.
    H3Vector<H3DlgItem*>* items_lst = reinterpret_cast<H3Vector<H3DlgItem*>*>(c->edi);

    // H-карты.
    items_lst->Append(H3DlgDefButton::Create(211, 52, 41, 33, HMAP_DLGITEM_ID, "SCHGBUT.def", 0, 1, 0, 0));

    // XH-карты.
    items_lst->Append(H3DlgDefButton::Create(258, 52, 41, 33, XHMAP_DLGITEM_ID, "SCXHBUT.def", 0, 1, 0, 0));

    // G-карты.
    items_lst->Append(H3DlgDefButton::Create(305, 52, 41, 33, GMAP_DLGITEM_ID, "SCGTBUT.def", 0, 1, 0, 0));

    return EXEC_DEFAULT;
}

// Обработка кнопок выбора размера в создании части для случайных карт диалога выбора карты.
void __stdcall HiHook_MapSelectDlg_RandMapDlgCreate(HiHook* h, H3SelectScenarioDialog* this_)
{
    // Оригинальные элементы.
    THISCALL_1(void, h->GetDefaultFunc(), this_);

    // Диалог.
    H3Vector<H3DlgItem*>* items_lst = &this_->GetList();

    // Генерация H-карты.
    items_lst->Append(H3DlgDefButton::Create(230, 81, 41, 33, HRMAP_DLGITEM_ID, "RanSizHG.def", 0, 1, 0, 0));

    // Генерация XH-карты.
    items_lst->Append(H3DlgDefButton::Create(273, 81, 41, 33, XHRMAP_DLGITEM_ID, "RanSizXH.def", 0, 1, 0, 0));

    // Генерация G-карты.
    items_lst->Append(H3DlgDefButton::Create(316, 81, 41, 33, GRMAP_DLGITEM_ID, "RanSizGT.def", 0, 1, 0, 0));

    //byte_104A779C = 0;
    //byte_104A7440 = 0;
}

// Обработка кнопок выбора размера в показе спика карт.
_LHF_(LoHook_MapSelectDlg_MapListShow_SizeButtons)
{
    // Диалог.
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->esi);

    // Элемент диалога.
    H3DlgItem* item;

    // Должны ли элементы быть доступными.
    bool need_enabled = (!IntAt(0x69959C) 
        || !DwordAt(0x69D858) 
        || THISCALL_1(INT8, *(_ptr_*)(*(_ptr_*)DwordAt(0x69D858) + 144), DwordAt(0x69D858))
        || dlg->saveGameMode);

    // H-карты.
    item = dlg->GetH3DlgItem(HMAP_DLGITEM_ID);
    if (item)
    {
        // Показываем и устанавливаем доступность элементу.
        item->SendCommand(5, 6);
        item->EnableItem(need_enabled);
    }

    // XH-карты.
    item = dlg->GetH3DlgItem(XHMAP_DLGITEM_ID);
    if (item)
    {
        // Показываем и устанавливаем доступность элементу.
        item->SendCommand(5, 6);
        item->EnableItem(need_enabled);
    }

    // G-карты.
    item = dlg->GetH3DlgItem(GMAP_DLGITEM_ID);
    if (item)
    {
        // Показываем и устанавливаем доступность элементу.
        item->SendCommand(5, 6);
        item->EnableItem(need_enabled);
    }


    return EXEC_DEFAULT;
}

// Обработка новых элементов в скрытии спика карт.
void __stdcall LoHook_MapSelectDlg_MapListHide(HiHook* h, H3SelectScenarioDialog* this_)
{
    this_->RemoveControlState(HMAP_DLGITEM_ID, eControlState(6));
    this_->RemoveControlState(XHMAP_DLGITEM_ID, eControlState(6));
    this_->RemoveControlState(GMAP_DLGITEM_ID, eControlState(6));

    // Скрываем оригинальные элементы.
    THISCALL_1(void, h->GetDefaultFunc(), this_);
}


// Обработка кнопок выбора размера в показе меню настроек ГСК.
_LHF_(LoHook_MapSelectDlg_RMGShow_SizeButtons)
{
    // Диалог.
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->esi);

    dlg->AddControlState(HRMAP_DLGITEM_ID, eControlState(6));
    dlg->AddControlState(XHRMAP_DLGITEM_ID, eControlState(6));
    dlg->AddControlState(GRMAP_DLGITEM_ID, eControlState(6));

    return EXEC_DEFAULT;
}


// Обработка кнопок выбора размера в скрытии меню настроек ГСК.
_LHF_(LoHook_MapSelectDlg_RMGHide_SizeButtons)
{
    // Диалог.
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->esi);

    dlg->RemoveControlState(HRMAP_DLGITEM_ID, eControlState(6));
    dlg->RemoveControlState(XHRMAP_DLGITEM_ID, eControlState(6));
    dlg->RemoveControlState(GRMAP_DLGITEM_ID, eControlState(6));

    return EXEC_DEFAULT;
}


// Перерисовка новых кнопок выбора размера карты вместе со всеми.
_LHF_(LoHook_MapSelectDlg_SizeButtonsRedraw)
{
    // Перерисовываем новые кнопки.
    THISCALL_4(void, PtrAt(PtrAt(c->ebx) + 20), c->ebx, 0, HMAP_DLGITEM_ID, GMAP_DLGITEM_ID);

    return EXEC_DEFAULT;
}


// Обработка нажатия кнопок в меню выбора карты.
_LHF_(LoHook_MapSelectDlg_ButtonClick)
{
    // Диалог.
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->ebx);

    // Сообщение на обновление дефа размера карты в правом верхнем углу
    //H3Msg* msg = reinterpret_cast<H3Msg*>(c->esi);
    H3Msg msg;
    msg.command = eMsgCommand::MOUSE_BUTTON;
    msg.subtype = eMsgSubtype::SET_FRAME;
    msg.itemId = 189;
    int itemId = c->eax;
    int mapDimension = 0;

    if (itemId >= 3000)
    {
        // Номер элемента.
        switch (itemId)
        {
            // Фильтр H-карт.
        case HMAP_DLGITEM_ID:
            THISCALL_2(void, 0x586060, dlg, 180);
            c->return_address = 0x5879B0;
            return NO_EXEC_DEFAULT;
            break;

            // Фильтр XH-карт.
        case XHMAP_DLGITEM_ID:
            THISCALL_2(void, 0x586060, dlg, 216);
            c->return_address = 0x5879B0;
            return NO_EXEC_DEFAULT;
            break;

            // Фильтр G-карт.
        case GMAP_DLGITEM_ID:
            THISCALL_2(void, 0x586060, dlg, 252);
            c->return_address = 0x5879B0;
            return NO_EXEC_DEFAULT;
            break;

            // Генерация H-карты.
        case HRMAP_DLGITEM_ID:
            // Устанавливаем размер.
            mapDimension = 180;
            dlg->mapDimension = mapDimension;
            THISCALL_1(void, 0x57F240, dlg);
            dlg->mapInfo.mapDimension = mapDimension;
            P_Game->mapInfo.mapDimension = mapDimension;
            msg.parameter = reinterpret_cast<void*>(4);
            THISCALL_2(void, 0x5FF3A0, dlg, &msg);
            // Выход с перерисовкой.
            c->return_address = 0x587277;
            return NO_EXEC_DEFAULT;
            break;

            // Генерация XH-карты.
        case XHRMAP_DLGITEM_ID:
            // Устанавливаем размер.
            mapDimension = 216;
            dlg->mapDimension = mapDimension;
            THISCALL_1(void, 0x57F240, dlg);
            dlg->mapInfo.mapDimension = mapDimension;
            P_Game->mapInfo.mapDimension = mapDimension;
            msg.parameter = reinterpret_cast<void*>(5);
            THISCALL_2(void, 0x5FF3A0, dlg, &msg);
            // Выход с перерисовкой.
            c->return_address = 0x587277;
            return NO_EXEC_DEFAULT;
            break;

            // Генерация G-карты.
        case GRMAP_DLGITEM_ID:
            // Устанавливаем размер.
            mapDimension = 252;
            dlg->mapDimension = mapDimension;
            THISCALL_1(void, 0x57F240, dlg);
            dlg->mapInfo.mapDimension = mapDimension;
            P_Game->mapInfo.mapDimension = mapDimension;
            msg.parameter = reinterpret_cast<void*>(6);
            THISCALL_2(void, 0x5FF3A0, dlg, &msg);
            // Выход с перерисовкой.
            c->return_address = 0x587277;
            return NO_EXEC_DEFAULT;
            break;

        default:
            return EXEC_DEFAULT;
            break;
        }
    }

    else
    {
        if (itemId < 281)
        {
            return EXEC_DEFAULT;
        }   
        if (itemId > 284)
        {
            if (itemId == 285)
            {
                bool v4 = dlg->mapInfo.hasUnderground == 0;
                dlg->mapInfo.hasUnderground = v4;
                P_Game->mapInfo.hasUnderground = v4;
            }
            return EXEC_DEFAULT;
        }
        dlg->mapInfo.mapDimension = 4 * (9 * itemId - 2520);
        P_Game->mapInfo.mapDimension = 36 * (c->eax - 280);
        msg.parameter = reinterpret_cast<void*>(c->eax - 281);
        THISCALL_2(void, 0x5FF3A0, dlg, &msg);
        return EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}


// Установка подсказок в диалоге выбора карты для новых элементов.
void __stdcall HiHook_MapSelectDlg_SetTips(HiHook* h, H3SelectScenarioDialog* this_, DWORD* tips, INT32 start_item_id, INT32 end_item_id, BOOL8 alloc_mem)
{
    // Устанавливаем подсказки стандартным элементам.
    THISCALL_5(void, h->GetDefaultFunc(), this_, tips, start_item_id, end_item_id, alloc_mem);

    // Элемент диалога.
    H3DlgItem* item;

    // Деф размера карты в правом верхнем углу
    item = this_->GetH3DlgItem(189);
    if (item) SetRmcHint(item, alloc_mem);

    // H-карты.
    item = this_->GetH3DlgItem(HMAP_DLGITEM_ID);
    if (item) SetRmcHint(item, alloc_mem);

    // XH-карты.
    item = this_->GetH3DlgItem(XHMAP_DLGITEM_ID);
    if (item) SetRmcHint(item, alloc_mem);

    // G-карты.
    item = this_->GetH3DlgItem(GMAP_DLGITEM_ID);
    if (item) SetRmcHint(item, alloc_mem);

    // Генерация H-карты.
    item = this_->GetH3DlgItem(HRMAP_DLGITEM_ID);
    if (item) SetRmcHint(item, alloc_mem);

    // Генерация XH-карты.
    item = this_->GetH3DlgItem(XHRMAP_DLGITEM_ID);
    if (item) SetRmcHint(item, alloc_mem);

    // Генерация G-карты.
    item = this_->GetH3DlgItem(GRMAP_DLGITEM_ID);
    if (item) SetRmcHint(item, alloc_mem);
}


// Настройка состояния кнопок размера настроек случайной карты.
_LHF_(LoHook_MapSelectDlg_RmgSetSizeButtonsState)
{
    // Диалог выбора карты.
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->esi);

    // Делаем кнопки новых размеров ненажатыми.
    dlg->RemoveControlState(HRMAP_DLGITEM_ID, eControlState::SELECTED);
    dlg->RemoveControlState(XHRMAP_DLGITEM_ID, eControlState::SELECTED);
    dlg->RemoveControlState(GRMAP_DLGITEM_ID, eControlState::SELECTED);

    switch (c->eax)
    {
        // H-карта.
    case 180:
        // Выделяем кнопку как нажатую.
        dlg->AddControlState(HRMAP_DLGITEM_ID, eControlState::SELECTED);
        // Пропускаем стандартное выделение.
        c->return_address = 0x57F2B6;
        return NO_EXEC_DEFAULT;
        break;

        // XH-карта.
    case 216:
        // Выделяем кнопку как нажатую.
        dlg->AddControlState(XHRMAP_DLGITEM_ID, eControlState::SELECTED);
        // Пропускаем стандартное выделение.
        c->return_address = 0x57F2B6;
        return NO_EXEC_DEFAULT;
        break;

        // G-карта.
    case 252:
        // Выделяем кнопку как нажатую.
        dlg->AddControlState(GRMAP_DLGITEM_ID, eControlState::SELECTED);
        // Пропускаем стандартное выделение.
        c->return_address = 0x57F2B6;
        return NO_EXEC_DEFAULT;
        break;

    default:
        return EXEC_DEFAULT;
        break;
    }

    return EXEC_DEFAULT;
}


// Настройка иконки размера карты: выбор сценария кампании.
_LHF_(LoHook_MapSelectDlg_SizeIcon_CampScenario)
{
    // Размер карты.
    switch (c->esi)
    {
        // S-карта.
    case 36:
        IntAt(c->ebp - 20) = 0;
        break;

        // M-карта.
    case 72:
        IntAt(c->ebp - 20) = 1;
        break;

        // L-карта.
    case 108:
        IntAt(c->ebp - 20) = 2;
        break;

        // XL-карта.
    case 144:
        IntAt(c->ebp - 20) = 3;
        break;

        // H-карта.
    case 180:
        IntAt(c->ebp - 20) = 4;
        break;

        // XH-карта.
    case 216:
        IntAt(c->ebp - 20) = 5;
        break;

        // G-карта.
    case 252:
        IntAt(c->ebp - 20) = 6;
        break;

    default:
        IntAt(c->ebp - 20) = 7;
        break;
    }

    // Пропускаем стандартную настройку.
    c->return_address = 0x457895;
    return NO_EXEC_DEFAULT;
}


// Настройка иконки размера карты: информация о сценарии.
_LHF_(LoHook_MapSelectDlg_SizeIcon_ScenarioInfo)
{
    // Размер карты.
    switch (c->eax)
    {
        // S-карта.
    case 36:
        c->eax = 0;
        break;

        // M-карта.
    case 72:
        c->eax = 1;
        break;

        // L-карта.
    case 108:
        c->eax = 2;
        break;

        // XL-карта.
    case 144:
        c->eax = 3;
        break;

        // H-карта.
    case 180:
        c->eax = 4;
        break;

        // XH-карта.
    case 216:
        c->eax = 5;
        break;

        // G-карта.
    case 252:
        c->eax = 6;
        break;

    default:
        c->eax = 7;
        break;
    }

    // Пропускаем стандартную настройку.
    c->return_address = 0x56950F;
    return NO_EXEC_DEFAULT;
}


// Настройка иконки размера карты: выбор карты.
_LHF_(LoHook_MapSelectDlg_SizeIcon_SelectMap)
{
    // Размер карты.
    switch (c->eax)
    {
        // S-карта.
    case 36:
        IntAt(c->ebp - 8) = 0;
        break;

        // M-карта.
    case 72:
        IntAt(c->ebp - 8) = 1;
        break;

        // L-карта.
    case 108:
        IntAt(c->ebp - 8) = 2;
        break;

        // XL-карта.
    case 144:
        IntAt(c->ebp - 8) = 3;
        break;

        // H-карта.
    case 180:
        IntAt(c->ebp - 8) = 4;
        break;

        // XH-карта.
    case 216:
        IntAt(c->ebp - 8) = 5;
        break;

        // G-карта.
    case 252:
        IntAt(c->ebp - 8) = 6;
        break;

    default:
        IntAt(c->ebp - 8) = 7;
        break;
    }

    // Пропускаем стандартную настройку.
    c->return_address = 0x5858D5;
    return NO_EXEC_DEFAULT;
}


// Настройка иконки размера карты: выбор карты - сеть.
_LHF_(LoHook_MapSelectDlg_SizeIcon_SelectMapOnline)
{
    // Размер карты.
    switch (c->eax)
    {
        // S-карта.
    case 36:
        IntAt(c->ebp - 8) = 0;
        break;

        // M-карта.
    case 72:
        IntAt(c->ebp - 8) = 1;
        break;

        // L-карта.
    case 108:
        IntAt(c->ebp - 8) = 2;
        break;

        // XL-карта.
    case 144:
        IntAt(c->ebp - 8) = 3;
        break;

        // H-карта.
    case 180:
        IntAt(c->ebp - 8) = 4;
        break;

        // XH-карта.
    case 216:
        IntAt(c->ebp - 8) = 5;
        break;

        // G-карта.
    case 252:
        IntAt(c->ebp - 8) = 6;
        break;

    default:
        IntAt(c->ebp - 8) = 7;
        break;
    }

    // Пропускаем стандартную настройку.
    c->return_address = 0x585AF6;
    return NO_EXEC_DEFAULT;
}


// Настройка иконки размера карты: фильтр карт.
_LHF_(LoHook_MapSelectDlg_SizeIcon_FilterMaps)
{
    // Восстанавливаем затёртые команды.
    IntAt(c->ebp - 56) = c->ecx;
    IntAt(c->ebp - 52) = c->ecx;
    IntAt(c->ebp - 48) = c->ecx;
    IntAt(c->ebp - 40) = c->ecx;
    IntAt(c->ebp - 68) = 0x200;
    IntAt(c->ebp - 60) = 189;
    IntAt(c->ebp - 64) = c->edx;

    // Размер карты.
    switch (c->eax)
    {
        // S-карта.
    case 36:
        IntAt(c->ebp - 44) = 0;
        break;

        // M-карта.
    case 72:
        IntAt(c->ebp - 44) = 1;
        break;

        // L-карта.
    case 108:
        IntAt(c->ebp - 44) = 2;
        break;

        // XL-карта.
    case 144:
        IntAt(c->ebp - 44) = 3;
        break;

        // H-карта.
    case 180:
        IntAt(c->ebp - 44) = 4;
        break;

        // XH-карта.
    case 216:
        IntAt(c->ebp - 44) = 5;
        break;

        // G-карта.
    case 252:
        IntAt(c->ebp - 44) = 6;
        break;

    default:
        IntAt(c->ebp - 44) = 7;
        break;
    }

    // Пропускаем стандартную настройку.
    c->return_address = 0x5861C5;
    return NO_EXEC_DEFAULT;
}


_LHF_(LoHook_MapSelectDlg_SizeAbbr)
{
    int mapSize = c->esi;

    if (mapSize == MAPSIZE_H || mapSize == MAPSIZE_XH || mapSize == MAPSIZE_G)
    {
        libc::sprintf(h3_TextBuffer, COLUMN_MAP_SIZE_KEY, mapSize);
        LPCSTR rmcHint = EraJS::read(h3_TextBuffer);
        c->edi = (_ptr_)rmcHint;
        c->return_address = 0x584ACC;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}


void __stdcall HiHook_Cheat_OpenMap(HiHook* h, H3Game* this_, INT32 x, INT32 y, INT32 z, INT32 player_id, INT32 radius, BOOL8 unk)
{
    // Открываем всю карту.
    THISCALL_7(void, h->GetDefaultFunc(),
        this_,
        IntAt(0x6783C8) / 2,
        IntAt(0x6783CC) / 2,
        z,
        player_id,
        IntAt(4 * (IntAt(0x6783C8) < IntAt(0x6783CC)) + 0x6783C8), //std::max<INT32>(IntAt(0x6783C8), IntAt(0x6783CC)),
        unk);
}


void __stdcall HiHook_0040275b(HiHook* h, H3Game* this_, int x, int y, int z, int player_owner, int radius)
{
    THISCALL_6(void, h->GetDefaultFunc(),
        this_,
        IntAt(0x6783C8) / 2,
        IntAt(0x6783CC) / 2,
        z,
        player_owner,
        IntAt(4 * (IntAt(0x6783C8) < IntAt(0x6783CC)) + 0x6783C8));
}


// Отправляем сообщение при нажатии на изменение размера карты в настройках ГСК.
_LHF_(LoHook_MapSelectDlg_RmgSizeButtonClick_OnlineMsg)
{
    // Размер карты.
    switch (c->eax)
    {
        // ГСК-размеры карт.
    case 281:
    case 282:
    case 283:
    case 284:
    case 285:
    case HRMAP_DLGITEM_ID:
    case XHRMAP_DLGITEM_ID:
    case GRMAP_DLGITEM_ID:
        // Отправляем сообщение.
        c->return_address = 0x5868E6;

        return NO_EXEC_DEFAULT;
        break;

        // Иначе по-умолчанию.
    default:
        return EXEC_DEFAULT;
        break;
    }
}


_LHF_(LoHook_005806d9)
{
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->esi);
    int param = 0;

    // Размер карты.
    switch (dlg->mapDimension)
    {
        // S-карта.
    case 36:
        param = 0;
        break;

        // M-карта.
    case 72:
        param = 1;
        break;

        // L-карта.
    case 108:
        param = 2;
        break;

        // XL-карта.
    case 144:
        param = 3;
        break;

        // H-карта.
    case 180:
        param = 4;
        break;

        // XH-карта.
    case 216:
        param = 5;
        break;

        // G-карта.
    case 252:
        param = 6;
        break;

    default:
        param = 7;
        break;
    }

    H3Msg msg;
    msg.command = eMsgCommand::MOUSE_BUTTON;
    msg.subtype = eMsgSubtype::SET_FRAME;
    msg.itemId = 189;
    msg.parameter = reinterpret_cast<void*>(param);
    THISCALL_2(void, 0x5FF3A0, dlg, &msg);

    return EXEC_DEFAULT;
}


_LHF_(LoHook_00584470)
{
    UINT32 mapIndex = -1;
    int mapDimension = 0;
    bool emptyMapSize = true;
    H3SelectScenarioDialog* dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->ebx);
    int size = dlg->currentMapsList.Size();

    if (dlg->randomMapGeneration)
    {
        mapDimension = dlg->mapDimension;
    LABEL_10:
        emptyMapSize = mapDimension <= 0;
        goto LABEL_11;
    }
    mapIndex = dlg->selectedMapIndex;

    if (mapIndex < 0
        || size == 0
        || mapIndex >= static_cast<UINT32>(size)
        || !dlg->currentMapsList[mapIndex].mapVersion
        || (mapDimension = dlg->currentMapsList[mapIndex].mapDimension, emptyMapSize = mapDimension <= 0, !mapDimension))
    {
        if (!dlg->saveGameMode)
        {
            return 1;
        }
            
        mapDimension = P_Game->mapInfo.mapDimension;
        goto LABEL_10;
    }
LABEL_11:
    if (!emptyMapSize)
    {
        sprintf_s(h3_TextBuffer, 0x300u, "%dx%d", mapDimension, mapDimension);
        P_TinyFont->TextDraw(
            P_WindowManager->screenPcx16,
            h3_TextBuffer, 
            dlg->GetX() + 712,
            dlg->GetY() + 55, 35, 16, eTextColor(4), eTextAlignment(5));
    }

    return EXEC_DEFAULT;
}


_LHF_(LoHook_00567a0b)
{
    H3SelectScenarioDialog* dlg = *reinterpret_cast<H3SelectScenarioDialog**>(c->ebp - 0x34);
    sprintf_s(h3_TextBuffer, 0x300u, "%dx%d", P_Game->mapInfo.mapDimension, P_Game->mapInfo.mapDimension);
    dlg->AddItem(H3DlgText::Create(709, 49, 35, 16, h3_TextBuffer, "tiny.fnt", 4, 100, 5, 0), false);
    
    return EXEC_DEFAULT;
}



_LHF_(LoHook_00586103)
{
    H3ScenarioMapInformation* mapInfo = (H3ScenarioMapInformation*)(c->ecx + c->ebx);
    int filterSize = c->eax;
    int mapDimension = mapInfo->mapDimension;
    
    if (!mapInfo->mapVersion)
    {
        c->return_address = 0x586109;
        return NO_EXEC_DEFAULT;
    }   
    if (mapDimension > filterSize)
    {
        if (filterSize == 252)
        {
            c->return_address = 0x586109;
            return NO_EXEC_DEFAULT;
        }
    }
    else if (mapDimension > filterSize - 36)
    {
        c->return_address = 0x586109;
        return NO_EXEC_DEFAULT;
    }
    c->return_address = 0x58611C;
    return NO_EXEC_DEFAULT;
}


_LHF_(LoHook_005854be)
{
    H3ScenarioMapInformation* mapInfo = (H3ScenarioMapInformation*)(c->ebx + c->esi);
    int filterSize = c->eax;
    int mapDimension = mapInfo->mapDimension;
    
    if (!mapInfo->mapVersion)
    {
        c->return_address = 0x5854C4;
        return NO_EXEC_DEFAULT;
    }
    if (mapDimension > filterSize)
    {
        if (filterSize == 252)
        {
            c->return_address = 0x5854C4;
            return NO_EXEC_DEFAULT;
        }
    }
    else if (mapDimension > filterSize - 36)
    {
        c->return_address = 0x5854C4;
        return NO_EXEC_DEFAULT;
    }
    c->return_address = 0x5854DC;
    return NO_EXEC_DEFAULT;
}





















// Удаление мусора из имени рандом карты

void __stdcall HiHook_00580D40(HiHook* h, H3SelectScenarioDialog* this_, int randMaps)
{
    THISCALL_2(void, h->GetDefaultFunc(), this_, randMaps);

    if (this_->mapListShown)
    {
        THISCALL_3(void, 0x5857D0, this_, this_->selectedMapIndex, 0); // UpdateNewGameMap
    }
}










void MapSize_Init()
{
    _PI->WriteLoHook(0x57C734, LoHook_MapSelectDlg_Create_SizeButtons);
    _PI->WriteHiHook(0x57D440, SPLICE_, EXTENDED_, THISCALL_, HiHook_MapSelectDlg_RandMapDlgCreate);
    _PI->WriteLoHook(0x5810BB, LoHook_MapSelectDlg_MapListShow_SizeButtons);
    _PI->WriteHiHook(0x581C70, SPLICE_, EXTENDED_, THISCALL_, LoHook_MapSelectDlg_MapListHide);
    _PI->WriteLoHook(0x58023D, LoHook_MapSelectDlg_RMGShow_SizeButtons);
    _PI->WriteLoHook(0x582102, LoHook_MapSelectDlg_RMGHide_SizeButtons);
    _PI->WriteLoHook(0x585D12, LoHook_MapSelectDlg_SizeButtonsRedraw);
    _PI->WriteLoHook(0x586921, LoHook_MapSelectDlg_ButtonClick);
    _PI->WriteHiHook(0x57C99D, CALL_, EXTENDED_, THISCALL_, HiHook_MapSelectDlg_SetTips);
    _PI->WriteLoHook(0x57F276, LoHook_MapSelectDlg_RmgSetSizeButtonsState);
    _PI->WriteLoHook(0x45785B, LoHook_MapSelectDlg_SizeIcon_CampScenario);
    _PI->WriteLoHook(0x5694DB, LoHook_MapSelectDlg_SizeIcon_ScenarioInfo);
    _PI->WriteLoHook(0x58589B, LoHook_MapSelectDlg_SizeIcon_SelectMap);
    _PI->WriteLoHook(0x585AB8, LoHook_MapSelectDlg_SizeIcon_SelectMapOnline);
    _PI->WriteLoHook(0x58616E, LoHook_MapSelectDlg_SizeIcon_FilterMaps);
    _PI->WriteLoHook(0x584A7D, LoHook_MapSelectDlg_SizeAbbr);
    _PI->WriteByte(0x57AFD5 + 1, 25);
    _PI->WriteDword(0x57AE22 + 1, 23);
    _PI->WriteByte(0x57AE1E + 1, 41);
    _PI->WriteDword(0x57AE79 + 1, 70);
    _PI->WriteByte(0x57AE75 + 1, 41);
    _PI->WriteDword(0x57AED0 + 1, 117);
    _PI->WriteByte(0x57AECC + 1, 41);
    _PI->WriteDword(0x57AF27 + 1, 164);
    _PI->WriteByte(0x57AF23 + 1, 41);
    _PI->WriteDword(0x57AF7E + 1, 351);
    _PI->WriteDword(0x57D539 + 1, 58);
    _PI->WriteByte(0x57D535 + 1, 41);
    _PI->WriteDword(0x57D593 + 1, 101);
    _PI->WriteByte(0x57D58F + 1, 41);
    _PI->WriteDword(0x57D5ED + 1, 144);
    _PI->WriteByte(0x57D5E9 + 1, 41);
    _PI->WriteDword(0x57D647 + 1, 187);
    _PI->WriteByte(0x57D643 + 1, 41);
    _PI->WriteDword(0x57D6A1 + 1, 359);
    _PI->WriteByte(0x57D69D + 1, 34);
    _PI->WriteCodePatch(0x584909, "%n", 54); // 54 nop
    _PI->WriteCodePatch(0x57D49B, "%n", 2); // 2 nop
    _PI->WriteCodePatch(0x57D4A6, "%n", 70); // 70 nop
    _PI->WriteCodePatch(0x57D4EF, "%n", 27); // 27 nop
    _PI->WriteCodePatch(0x5801CA, "%n", 23); // 23 nop
    _PI->WriteCodePatch(0x58208F, "%n", 23); // 23 nop
    _PI->WriteCodePatch(0x584E35, "%n", 54); // 54 nop
    _PI->WriteHiHook(0x402704, CALL_, EXTENDED_, THISCALL_, HiHook_Cheat_OpenMap); // Чит
    _PI->WriteHiHook(0x4F4B61, CALL_, EXTENDED_, THISCALL_, HiHook_Cheat_OpenMap); // Чит-меню
    _PI->WriteHiHook(0x40275b, CALL_, EXTENDED_, THISCALL_, HiHook_0040275b);
    _PI->WriteLoHook(0x5868CC, LoHook_MapSelectDlg_RmgSizeButtonClick_OnlineMsg);
    _PI->WriteLoHook(0x5806d9, LoHook_005806d9);
    _PI->WriteLoHook(0x584470, LoHook_00584470);
    _PI->WriteLoHook(0x567a0b, LoHook_00567a0b);
    _PI->WriteLoHook(0x586103, LoHook_00586103);
    _PI->WriteLoHook(0x5854be, LoHook_005854be);
    _PI->WriteDword(0x584503, "%02d.%02d.%d, %d:%02d");
    _PI->WriteHiHook(0x580D40, SPLICE_, EXTENDED_, THISCALL_, HiHook_00580D40);
}
