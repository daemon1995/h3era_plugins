/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// ПАТЧИ ////////////////////////////////////////////////
namespace movement
{

// запрет взаимодействия через "Пробел" при пролёте над несоюзным городом
void __stdcall AdvMgr_Enter2Object_Player(HiHook *h, _AdvMgr_ *mgr, const _MapItem_ *mip, const int isHuman)
{

    if (mip->object_type == 98)
    {

        const _Town_ *townBelow = o_GameMgr->GetTown(mip->setup);
        //  if (townBelow->down_hero_id == -1)
        {
            const int townOwner = townBelow->owner_id;

            if (townOwner <
                0) // o_GameMgr->GetPlayerTeam(townBelow->owner_id) != o_GameMgr->GetPlayerTeam(o_ActivePlayerID))
            {
                return;
            }
        }
        return;
    }
    CALL_3(void, __thiscall, h->GetDefaultFunc(), mgr, mip, isHuman);
}
// Оставшиеся полные очки передвижения героя.
int HeroFullMP_Rem;

// Обновление стрелок маршрута героя.
_LHF_(LoHook_RouteUpdate)
{
    CALL_4(void, __thiscall, 0x418D30, o_AdvMgr, TRUE, TRUE, TRUE);
    o_AdvMgr->lastHoverX = -1;
    CALL_1(void, __thiscall, 0x4ECCD0, o_InputMgr);
    return 1;
}

// Не теребить кнопку лошади
_LHF_(LoHook_ProcessMessage)
{
    if (IntAt(c->ebp + 0x10))
    {
        return EXEC_DEFAULT;
    }

    c->return_address += 14;
    return NO_EXEC_DEFAULT;
}

// Инициализация оставшихся полных очков перемещения героя.
_LHF_(LoHook_HeroRoute_InitMaxMP)
{
    _Hero_ *hero = reinterpret_cast<_Hero_ *>(c->ebx);
    _Player_ *player = &o_GameMgr->players[hero->owner_id];

    // Получаем полные очки перемещения героя.
    HeroFullMP_Rem = hero->movement_max;

    bool v4 = CALL_1(bool, __thiscall, 0x4BAA40, o_ActivePlayer);

    int curDayOfWeek = o_GameMgr->curr_day_ix;

    char is_human2 = player->isHuman;
    char v7 = o_ActivePlayer->isHuman;

    if (v4 || (is_human2 && v7 && v7 > is_human2) || v7 == is_human2 && o_ActivePlayerID > hero->owner_id)
    {
        if ((hero->flags & 0x1000000) == 0) // cheats
        {
            // Лодка и конюшни
            if ((hero->flags & 0x40000) == 0 && (hero->flags & 2) != 0 && curDayOfWeek >= 7)
            {
                HeroFullMP_Rem -= IntAt(0x0698AE4); // o_MoveTXT_Obj_94
            }
        }
    }

    if (!v4)
    {
        IntAt(c->ebp - 0x4) = 0;
    }

    return EXEC_DEFAULT;
}

// Уменьшение оставшихся полных очков перемещения героя.
_LHF_(LoHook_HeroRoute_ReduceMaxMP)
{
    // Герой.
    _Hero_ *hero = reinterpret_cast<_Hero_ *>(c->ebx);

    // Уменьшаем полные очки перемещения героя на шаг.
    HeroFullMP_Rem -= CALL_4(int, __fastcall, 0x4B1620, hero, c->esi, DwordAt(c->ebp - 0x1C), HeroFullMP_Rem);

    return EXEC_DEFAULT;
}

// Показ особых стрелок, если путь дальше, чем максимальные очки перемещения героя.
_LHF_(LoHook_HeroRoute_SpecRouteMaxMP)
{
    // Смещение кадра стрелок.
    if (HeroFullMP_Rem < 0)
    {
        WordAt(c->edx + 2 * c->eax) += 25;
    }

    return EXEC_DEFAULT;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void HeroMovementFixes(PatcherInstance *_PI)
{

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////// Фиксы Карты Приключений /////////////////////////
    // запрет взаимодействия через "Пробел" при пролёте над несоюзным городом
    //   _PI->WriteHiHook(0x0408DC5, CALL_, EXTENDED_, THISCALL_, AdvMgr_Enter2Object_Player);

    // Обновление стрелок маршрута героя.
    _PI->WriteLoHook(0x4082CD, LoHook_RouteUpdate); // После окна героя
    _PI->WriteLoHook(0x409E2F, LoHook_RouteUpdate); // После окна обзора королевства
    _PI->WriteLoHook(0x4AA7E5, LoHook_RouteUpdate); // После посещения объекта
    _PI->WriteLoHook(0x41C5F2, LoHook_RouteUpdate); // После колдовства заклинания

    // AdvMgr_RouteUpdate 00418D30

    // Не теребить кнопку лошади
    _PI->WriteLoHook(0x0418D8D, LoHook_ProcessMessage); // 10004570
    _PI->WriteLoHook(0x0418E1D, LoHook_ProcessMessage);
    _PI->WriteLoHook(0x0419153, LoHook_ProcessMessage);
}
} // namespace movement
