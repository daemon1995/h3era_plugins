/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// фиксим неотображение Монолитов и Подземных врат в диалоге заклинания Просмотр Земли и Воздуха
int __stdcall Y_Fix_ViewEarthOrAirSpell_Add_Monoliths_Show(LoHook* h, HookContext* c)
{
    c->edi = c->ecx;                    // mov edi, ecx
    c->ecx = *(_dword_*)(c->esi +30);   // mov ecx, [esi+1Eh]

    if (_byte_ isVisible = *(_byte_*)(c->ebp + 8 + 3))
    {
        const int obj_id = c->ecx; // номер объекта

        int type = 0;
        if (obj_id == 43 || obj_id == 44 || obj_id == 45) { // Монолиты
            type = 3;
        }

        if (obj_id == 103) { // Подземные врата
            type = 4;
        }

        if (type == 3 || type == 4) { // выполняем функцию отображения Монолитов или Подземных Врат при касте Просмотра Земли или Воздуха
            CALL_6(_int_, __fastcall, 0x5F7760, *(int*)0x6AAC88, c->esi, type, *(int*)0x6AAC80 + (*(int*)(c->ebp + 0x14) * *(int*)0x68C70C), c->edi + 8, *(int*)(c->ebp + 0x12));
        }
    }

    c->return_address = 0x5F854A;
    return NO_EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////


// корректировка описаний заклинаний в книге (не учитывались бонусы специалистов по заклинаниям)
_int64_ __stdcall Y_DlgSpellBook_FixDecription_SpellPower(HiHook* hook, _Hero_ *hero, int spell, signed int damage, _BattleStack_ *stack)
{
    _int64_ power = CALL_4(_int64_, __thiscall, hook->GetDefaultFunc(), hero, spell, damage, stack);
    power += (_int64_)hero->GetSpell_Specialisation_PowerBonuses(spell, (_int_)power, 0);

    return power;
}

// исправление бага WOG: IDummy(0x83EEFC) иногда может возвращать мусор см.функцию 0x75D654
_int_ __stdcall Y_Fix_WoG_HeroGetSpellSpecialityEffect(HiHook* hook, _Hero_* hero, _int_ spellId, _int_ monLevel, _int_ damage)
{
    _int_ result = 0;
    if (hero)
    {
        hero->GetSpell_Specialisation_PowerBonuses(spellId, damage, monLevel);
    }
    WOG_IDummy = result;
    return result;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// фикс неправильного отображения величины урона в окне статуса битвы при касте заклинания Армагеддон
_int_ __stdcall Y_Fix_ReportStatusMsg_CastArmageddonSpell(HiHook* hook, _BattleMgr_ *bm, _int_ damage, _int_ spell, _Hero_ *heroA, _Hero_ *heroD, _BattleStack_ *stack, _int_ a7) 
{
    if ( !heroA ) { // в этом месте структура атакующего героя не подаётся. Поэтому мы создаём её сами
        heroA = bm->hero[bm->currentActiveSide];
    }

    _int_ ret = CALL_7(_int_, __thiscall, hook->GetDefaultFunc(), bm, damage, spell, heroA, heroD, stack, a7);
    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////            ЗАПРЕТЫ ЗАКЛИНАНИЙ             ////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// запрет выдачи заклинаний у артефактов, если они запрещены через UN:J0/spell_id/1;
int __stdcall Y_ArtGive_Spell(HiHook* hook, int spells_array, unsigned int spell_id, char enable)
{
    if (enable && o_GameMgr->disabled_spells[spell_id]) {
        if ( spell_id != SPL_TITANS_LIGHTNING_BOLT ) {
            return 0;
        }
    }

    return CALL_3(int, __thiscall, hook->GetDefaultFunc(), spells_array, spell_id, enable);
}

// заполняем таблицы запретов заклинаний при при загрузке карты, если они запрещены через UN:J0/spell_id/1;
int __stdcall Y_ArtGive_LoadSpells(HiHook* hook, int spells_array, char enable) 
{
    int spell_id = *(int*)(spells_array +4);

    o_GameMgr->disabled_shrines[spell_id] = enable;
    o_GameMgr->disabled_spells[spell_id] = enable;

    return CALL_2(int, __thiscall, hook->GetDefaultFunc(), spells_array, enable);
}

// запрет выдачи заклинаний у томов магии и шляпы школяра, если они запрещены через UN:J0/spell_id/1;
int __stdcall Y_ArtGive_AllSpells(HiHook* hook, int spells_array, char enable) 
{
    int spell_id = *(int*)(spells_array +4);

    if (enable && o_GameMgr->disabled_spells[spell_id]) {
        return 0;
    }

    return CALL_2(int, __thiscall, hook->GetDefaultFunc(), spells_array, enable);
}

// Наличие яда теперь не даёт колдовать Снятие Заклинаний.
int __stdcall LoHook_RestrictDispel_Poison(LoHook* h, HookContext* c)
{
    // Яд - пропускаем проверку на него.
    if (c->eax == SPL_POISON)
    {
        return NO_EXEC_DEFAULT;
    }
    else
    {
        return EXEC_DEFAULT;
    }
}
// Фикс бага SoD: дд - возможность сделать каст в самого героя и отмена при попытке прыгнуть с суши на воду и за пределы карты
_LHF_(DimensionDoorProcMouseOver)
{

    DWORD pos = 0;//H3Position
    DWORD* mousePos = CALL_2(DWORD*, __thiscall, 0x407A70, o_AdvMgr, &pos);
    if (CALL_1(bool, __thiscall,0x4B1090, mousePos))  // H3Position::isCorrect
    {
        _MapItem_* targetCell = reinterpret_cast<_MapItem_*>(c->eax);
        
        _Hero_* hero = o_GameMgr->GetHero(o_ActivePlayer->selected_hero_id);
        _MapItem_* heroCell = o_GameMgr->Map.GetItem(hero->x, hero->y, hero->z);
        if (targetCell != heroCell)
        {
            BOOL isHeroWater = heroCell->land == 8;
            BOOL isTargetWater = targetCell->land == 8;

            if (isHeroWater == isTargetWater)
            {
                return EXEC_DEFAULT;
            }
        }
    }

    c->return_address = 0x4915D3;
    return NO_EXEC_DEFAULT;
}
struct H3Point
{
    INT32 x;
    INT32 y;
    INT32 z;
};
template<typename T>
H3Point ReverseCoordinates(const T* current_point, const T* base_point, UINT map_size)
{
    H3Point coordinates;
    UINT delta = current_point - base_point;
    coordinates.x = delta % map_size;
    delta /= map_size;
    coordinates.y = delta % map_size;
    coordinates.z = delta / map_size;
    return coordinates;
}

// добавление точного количества монстров в хинт бар Карты Приключений при активном заклинании Видение
_LHF_(Visions_MonsterCommand)
{
    _Hero_* hero = *reinterpret_cast<_Hero_**>(c->ebp - 0x10);


    // H3Hero::GetVisionsPower
    if (hero && CALL_1(int,__thiscall, 0x4E6050, hero) > -1)
    {
        _MapItem_* cell = reinterpret_cast<_MapItem_*>(c->ebx);
        DWORD packed = cell->GetPackedCoordinates();

        // H3Hero::IsInVisionsRange
        if (CALL_2(bool, __thiscall, 0x4E6080, hero, &packed))
        {
            const int monCount = cell->setup & 0xFFF;
            sprintf_s(o_TextBuffer, 0x300u, "%s: %d", (LPCSTR)c->eax, monCount);
            c->return_address = 0x40C2F9;
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}

// исправление невозможности воскресить Золотых Драконов (и прочую шушеру) через жертву (из-за иммунитета к воскрешению)
_BattleStack_* __stdcall BattleMgr_CanCastSpellAtCoord(HiHook* h,
    _BattleMgr_* bm,
    const int  spellId,
    const int side,
    const  signed int gex,
    const char a6,
    const int casterKind)
{
    char savedSpell;
    if (spellId == SPL_SACRIFICE)
    {
        savedSpell = ByteAt(0x05A4055 + 1);
        ByteAt(0x05A4055 + 1) = SPL_SACRIFICE;
        ByteAt(0x05A4110 + 1) = SPL_SACRIFICE;

    }
    _BattleStack_* result = CALL_6(_BattleStack_*, __thiscall, h->GetDefaultFunc(), bm, spellId, side, gex, a6, casterKind);
    if (spellId == SPL_SACRIFICE)
    {
        ByteAt(0x05A4055 + 1) = savedSpell;
        ByteAt(0x05A4110 + 1) = savedSpell;

    }
    return result;
}

void __stdcall Town_GiveSpellsToHeroes(HiHook *h, _Town_ *town, _Hero_ *hero)
{
    CALL_2(void, __thiscall, h->GetDefaultFunc(), town, hero);
    if (town->up_hero_id != -1)
    {
        _Hero_ *topHero = o_GameMgr->GetHero(town->up_hero_id);
        if (hero == nullptr && town->down_hero_id != -1)
        {
            hero = o_GameMgr->GetHero(town->down_hero_id);
        }
        if (topHero && hero)
        {
            WORD savedBytes[2] = {WordAt(0x04A26D5), WordAt(0x04A271E)};
            WordAt(0x04A26D5) = 0x66EB;
            WordAt(0x04A271E) = 0x1DEB;
            CALL_2(void, __fastcall, 0x04A25B0, topHero, hero);
			WordAt(0x04A26D5) = savedBytes[0];
			WordAt(0x04A271E) = savedBytes[1];
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Spells(PatcherInstance* _PI)
{
    // добавление флага "Заклинание с уроном"
	o_Spells[SPL_LAND_MINE].flags |= 0x200;
	o_Spells[SPL_FIRE_WALL].flags |= 0x200;

    // возможность заходить в гильдию магов без наличия книги и денег у героя-гостя
    _PI->WriteHexPatch(0x5CEA83, "EB74");
    _PI->WriteHexPatch(0x5CEACD, "2800");

    // корректировка описаний заклинаний в книге (не учитывались бонусы специалистов по заклинаниям)
    _PI->WriteHiHook(0x59BFFD, CALL_, SAFE_, THISCALL_, Y_DlgSpellBook_FixDecription_SpellPower);

    // исправление бага WOG: IDummy(0x83EEFC) иногда может возвращать мусор см.функцию 0x75D654
    _PI->WriteHiHook(0x5A1C7B, CALL_, EXTENDED_, THISCALL_, Y_Fix_WoG_HeroGetSpellSpecialityEffect);

    // фиксим неотображение Монолитов и Подземных врат в диалоге заклинания Просмотр Земли и Воздуха
    _PI->WriteLoHook(0x5F8545, Y_Fix_ViewEarthOrAirSpell_Add_Monoliths_Show);

    // фикс неправильного отображения величины урона в окне статуса битвы при касте заклинания Армагеддон
    _PI->WriteHiHook(0x5A5522, CALL_, EXTENDED_, THISCALL_, Y_Fix_ReportStatusMsg_CastArmageddonSpell);

    // Наличие яда теперь не даёт колдовать Снятие Заклинаний.
    _PI->WriteLoHook(0x5A84BD, LoHook_RestrictDispel_Poison);

    // Фикс бага SoD: дд - возможность сделать каст в самого героя и отмена при попытке прыгнуть с суши на воду и за пределы карты
    _PI->WriteLoHook(0x4915B7, DimensionDoorProcMouseOver);

    // Исправление бага с несбросом защитной стойки и бешенства стека перед ходом, если ход ему был передан первый раз - уже в фазе ожидания.
    // _PI->WriteCodePatch(0x464DF1, "%n", 10); // 10 nop

	// добавление точного количества монстров в хинт бар Карты Приключений при активном заклинании Видение
    _PI->WriteLoHook(0x40C2D3, Visions_MonsterCommand);

	// исправление невозможности воскресить Золотых Драконов (и прочую шушеру) через жертву (из-за иммунитета к воскрешению)
    _PI->WriteHiHook(0x05A3C60, SPLICE_, EXTENDED_, THISCALL_, BattleMgr_CanCastSpellAtCoord);
    
	// исправление бага с невозможностью изучить заклинания города при телепортации героя в город
	_PI->WriteWord(0x041D9E3, 0x9056); // push esi; nop

	// изучение заклинаний в городе героем автоматически вызывает обмен заклинаниями героев в городе;
	_PI->WriteHiHook(0x05BE430, SPLICE_, EXTENDED_, THISCALL_, Town_GiveSpellsToHeroes);
    
    // патчи без Tiphon.dll
    if (!TIPHON)
    {
        // отключено, потому что ломает авторские карты! (03.12.2023)
        // запрет выдачи заклинаний у артефактов
        //_PI->WriteHiHook(0x4D9763, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_Spell);
        //_PI->WriteHiHook(0x4D9786, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_Spell);
        //_PI->WriteHiHook(0x4D9798, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_Spell);
        // заполняем таблицу при загрузке карты
        _PI->WriteHiHook(0x4C2445, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_LoadSpells);
        // запрет выдачи заклинаний у томов магии
        //_PI->WriteHiHook(0x4D961F, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_AllSpells); // том воздуха
        //_PI->WriteHiHook(0x4D9673, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_AllSpells); // том огня
        //_PI->WriteHiHook(0x4D9720, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_AllSpells); // том земли
        //_PI->WriteHiHook(0x4D96C8, CALL_, EXTENDED_, THISCALL_, Y_ArtGive_AllSpells); // том воды
    }

    // заклинание забывчивось на Продвинутом уровне действует на один целевой отряд (баг СОДа)
    // o_Spell[SPL_FORGETFULNESS].flags |= SPF_HAS_MASS_ON_EXPERT;
}
