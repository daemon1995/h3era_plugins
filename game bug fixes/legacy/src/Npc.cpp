////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// фикс: командиры, имеющие флаг стрельбы и двойной атаки, в рукопашной бъют один раз.
// Исправим это, ибо они уникальны (03/12/2023)
_LHF_(Y_NPC_FixDoubleAttackOnMelle)
{
    _BattleStack_ *stack = (_BattleStack_ *)c->edi;
    // CF_DOUBLEATTACK уже проверена к этому моменту
    // т.е. мы гарантированно знаем, что стек имеет двойную атаку
    if (stack->creature.flags & BCF_CAN_SHOOT)
    {
        if (Era::IsCommanderId(stack->creature_id))
        {
            // может иметь двойную атаку
            c->return_address = 0x441BB1;
        }
        // НЕ может иметь двойную атаку
        else
            c->return_address = 0x441C01;
    }
    // может иметь двойную атаку
    else
        c->return_address = 0x441BB1;

    return NO_EXEC_DEFAULT;
}

/////////////////////////////// добавление расчета силы командиров для AI (Fight_Value и AI_Value)
////////////////////////////

_int64_ getAIValue_NPC(int heroID)
{
    _int64_ value = 50;

    if (heroID < 0 && heroID >= o_HeroesCount)
        return value; // выход, если номер героя неправильный

    _Npc_ *npc = GetNpc(heroID); // структура командира
    if (!npc || npc->on < 1 || npc->alive)
        return value; // выход, если командиров нет (-1), уволен или мертв

    _Hero_ *hero = o_GameMgr->GetHero(heroID);

    int attNPC = Get_NpcSkillPower(npc, 0);
    attNPC += hero->primary_skill[0];
    int defNPC = Get_NpcSkillPower(npc, 1);
    defNPC += hero->primary_skill[1];
    int hpNPC = Get_NpcSkillPower(npc, 2);
    int damageNPC = Get_NpcSkillPower(npc, 3);
    int speedNPC = Get_NpcSkillPower(npc, 5);
    int isShoot = npc->specBon[0] & 0x10 ? 2 : 1;

    value = (_int64_)((attNPC * damageNPC * speedNPC * isShoot + defNPC * hpNPC) >> 2);

    return value;
}

bool check_AIValue_isNotHero = false;

_int_ __stdcall get_AIValue_Hook(HiHook *hook, int army)
{
    _int_ value;

    int armyNPC = army;

    value = CALL_1(_int_, __thiscall, hook->GetDefaultFunc(), army);

    if (!check_AIValue_isNotHero)
    {
        int heroID = *(int *)(armyNPC - 119);

        if (heroID >= 0 && heroID < o_HEROES_COUNT)
            value += (_int_)getAIValue_NPC(heroID);
    }

    check_AIValue_isNotHero = false;

    if (value < 0 || value >= (INT_MAX - 500))
    {
        int random = Randint(550, 600);
        value = INT_MAX - random;
    }

    return value;
}

int __stdcall get_AIValue_And_NPC_Error(LoHook *h, HookContext *c)
{
    check_AIValue_isNotHero = true;
    return EXEC_DEFAULT;
}

int __stdcall get_Fight_Value_Hook(LoHook *h, HookContext *c)
{
    _Hero_ *hero = o_BattleMgr->hero[c->ebx];

    if (!hero)
        return EXEC_DEFAULT;

    c->ecx += static_cast<int>(getAIValue_NPC(hero->id));

    return EXEC_DEFAULT;
}

_LHF_(Y_FixCrit_SuccubusNotCharmed)
{
    // c->return_address = 0x76CECC;
    // return NO_EXEC_DEFAULT;

    _dword_ hero = DwordAt(c->ebp + 0x8);

    if (hero)
        c->return_address = 0x76CB7B;
    else
        c->return_address = 0x76CD1E;

    return NO_EXEC_DEFAULT;
}

// @Berserker
// исправление расчёта золота для Зверя после битвы
int bruteGoldAdded = 0;
_LHF_(gem_Brute_AddGold)
{
    int bruteGold = IntAt(c->ebp - 64);
    if (bruteGold < 0)
    {
        bruteGold = INT_MAX;
    }
    int playerGold = IntAt(c->ebp - 68);

    __int64 summ = static_cast<__int64>(bruteGold) + playerGold;

    if (summ > INT_MAX)
    {
        summ = INT_MAX;
    }
    playerGold = static_cast<int>(summ) - bruteGold;

    IntAt(c->ebp - 68) = playerGold;
    IntAt(c->ebp - 64) = bruteGold;

    bruteGoldAdded = bruteGold;

    return EXEC_DEFAULT;
}
// отображение золота Зверя в окошке просмотра информации после битвы
_ERH_(OnAfterBattleUniversal)
{
    if (bruteGoldAdded && o_MeID == o_ActivePlayerID)
    {
        auto msg = Get_ITxt(175, 1);
        CALL_4(void, __thiscall, 0x415FC0, o_AdvMgr, msg, GOLD, bruteGoldAdded);
        bruteGoldAdded = 0;
    }
}
/// исправления Slavas's Ring
// @ daemon_n
// корректировка количества заклинаний Командира с кольцом Славы
_LHF_(NPC_InitInCombat)
{
    _Npc_ *npc = reinterpret_cast<_Npc_ *>(c->ecx);
    if (npc && npc->HasArtifact(155))
    {
        _CreatureInfo_ *creatureInfo = *reinterpret_cast<_CreatureInfo_ **>(c->ebp + 0xC);
        if (creatureInfo->spells_count < 2)
        {
            creatureInfo->spells_count = 2;
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(NPC_CalculatePrimarySkills)
{

    _Npc_ *npc = reinterpret_cast<_Npc_ *>(c->ecx);
    if (npc && npc->secondary_skills[c->eax] < 2 && npc->HasArtifact(155))
    {
        c->return_address = 0x769556; // jumping directly to the calculation of bonuses from the ring, bypassing the
                                      // addition of standard bonuses]
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

// @ daemon_n
// фикс WoG бага, когда командиры призывают боевые машины в банках существ и баллисту в качестве защитника города
_LHF_(WoG_CombatStart_SummonNPC)
{
    const auto *mgr = *reinterpret_cast<_BattleMgr_ **>(c->ebp + 0x8);

    if (mgr->isBank                                             // если банк
        || mgr->town && mgr->siegeKind2 && IntAt(c->ebp + 0x10) // или и город, и со стенами, и это защитник,
               && h->GetAddress() == 0x76B97B)                  // и пытается поместить баллисту
    {
        IntAt(c->ebp - 0xC) = 1;
    }
    return EXEC_DEFAULT;
}

_ERH_(OnAfterCreateWindow)
{
    // @ daemon_n
    // фикс WoG бага, когда командиры призывают боевые машины в банках существ
    constexpr LPCSTR fixKey = "gbfe.disable_fix.npc.creature_banks_war_machines";
    if (atoi(Era::tr(fixKey)) == 0)
    {
        _PI->WriteLoHook(0x76B889, WoG_CombatStart_SummonNPC); // tent
        _PI->WriteLoHook(0x76B97B, WoG_CombatStart_SummonNPC); // ballista
    }
}

// @daemon_n
// отключаем командира инферно, забирающего армию не у нейтральных армий
_LHF_(NPC_DisableInfernoCommander)
{

    const int defenderPlayerId = IntAt(0x02846BC0);
    if (defenderPlayerId >= 0 && defenderPlayerId < 8)
    {
        c->return_address = 0x0075A637; // пропуск всей механики забора армии
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
// @daemon_n
// фикс: сообщение о посещении кристалов командиров показываются до их удаления с карты
int wogObjectType = 0;
int wogIsHuman = 0;
_LHF_(WoG_BeforeMapItemVisit)
{
    wogObjectType = IntAt(c->ebp - 0x14);
    wogIsHuman = IntAt(c->ebp - 0x4);
    return EXEC_DEFAULT;
}
void __stdcall AdvMgr_Chest_Visit(HiHook *h, _AdvMgr_ *AvdManager, _Hero_ *Hero, _MapItem_ *mapItem, int PosMixed,
                                  char isHuman)
{

    if (wogObjectType >= 17 && wogObjectType <= ByteAt(0x0706053 + 3))
    {
        CALL_3(void, __cdecl, 0x07708DE, Hero, wogObjectType - 17, wogIsHuman);
        wogObjectType = 0;
    }

    CALL_5(void, __thiscall, h->GetDefaultFunc(), AvdManager, Hero, mapItem, PosMixed, isHuman);
}
// void __stdcall AdvMgr_Resource_Visit(HiHook *h, _Hero_ *Hero, int resType, int resCount)
//{
//     if (wogObjectType == 30)
//     {
//         struct PlayersMithril
//         {
//             int mithril[8];
//         };
//         if (Hero->owner_id >= 0 && Hero->owner_id <= 7)
//         {
//            // reinterpret_cast<PlayersMithril *>(0x27F9A00)->mithril[Hero->owner_id] += resCount;
//             //   CALL_3(void, __cdecl, 0x07708DE, Hero, resType, resCount);
//             wogObjectType = 0;
//         }
//     }
//     CALL_3(void, __thiscall, h->GetDefaultFunc(), Hero, resType, resCount);
// }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Npc(PatcherInstance *_PI)
{
    // командиры, имеющие флаг стрельбы и двойной атаки, в рукопашной бъют один раз. Исправим это, ибо они уникальны
    _PI->WriteLoHook(0x441BAA, Y_NPC_FixDoubleAttackOnMelle);

    // добавляем расчет командиров в проверку Fight_Value
    _PI->WriteLoHook(0x41EAD2, get_Fight_Value_Hook);
    // добавляем в расчет AI_Value и расчет командиров
    _PI->WriteHiHook(0x44A950, SPLICE_, EXTENDED_, THISCALL_, get_AIValue_Hook);
    // ставим лоухук, чтобы понять что идет расчет AI_Value без наличия героя
    _PI->WriteLoHook(0x5C1867, get_AIValue_And_NPC_Error);
    _PI->WriteLoHook(0x42758F, get_AIValue_And_NPC_Error);
    _PI->WriteLoHook(0x42CA6B, get_AIValue_And_NPC_Error);
    _PI->WriteLoHook(0x52846A, get_AIValue_And_NPC_Error);

    // исправление бага блока командира, когда защита падала из-за флага "в защите"
    _PI->WriteCodePatch(0x76E7D7, "%n", 24); // 15 nop
    _PI->WriteCodePatch(0x76E80B, "%n", 13); // 13 nop
    _PI->WriteHexPatch(0x76E7D7, "8B4D 08 C601 01 C641 02 04");

    // исправление вылета при присоединении монстров Суккубом
    // в замке, когда разрушаем здание и провоцируем бой
    // отменяем диалог очарования монстров
    _PI->WriteCodePatch(0x76CB70, "%n", 6); // 6 nops
    _PI->WriteLoHook(0x76CB76, Y_FixCrit_SuccubusNotCharmed);

    // исправление одного из багов Астрального духа
    // убираем WoG сообщение, которое вызывает непонятную ошибку
    _PI->WriteHexPatch(0x76D4B3, "EB17");

    // исправление бага, когда командир после пропуска хода может оказаться мёртвым
    // затираем вызов функции CheckForAliveNPCAfterBattle в функции добавления опыта
    // Она тут не нужна! Этот воговский хук стоит в добавлении опыта!
    _PI->WriteCodePatch(0x76B39E, "%n", 7);

    // расширяем свитч хинтов колдовства для описаний командиров
    // и монстров с номером больше 134
    _PI->WriteHexPatch(0x492A56, "81FF B7000000 90 7747");
    _PI->WriteDword(0x492A63, *(_int_ *)0x44825F);

    // @Berserker
    // исправление расчёта золота для Зверя после битвы
    _PI->WriteLoHook(0x769A52, gem_Brute_AddGold);
    _PI->WriteByte(0x769A5C, 0xEB); // [jump over Brute Gold Bonus Message]

    Era::RegisterHandler(OnAfterBattleUniversal, "OnAfterBattleUniversal");
    /// исправления Slavas's Ring
    // @daemon_n
    // корректировка значения силы магии Командира
    _PI->WriteLoHook(0x076C1AA, NPC_InitInCombat);
    _PI->WriteByte(0x7695A0, 1); //[Change the number to check the skill level at 0x76959C cmp     dword ptr
                                 //[eax+edx*4+38h], 2 / jg      short loc_7695E3]
    _PI->WriteLoHook(0x076949E, NPC_CalculatePrimarySkills);

    Era::RegisterHandler(OnAfterCreateWindow, "OnAfterCreateWindow");

    // @daemon_n
    // фикс: сообщение о посещении кристалов командиров показываются до их удаления с карты
    _PI->WriteLoHook(0x0705FDD, WoG_BeforeMapItemVisit);
    _PI->WriteHiHook(0x04A9F93, CALL_, EXTENDED_, THISCALL_, AdvMgr_Chest_Visit);
    // пропуск оригинальной механики
    _PI->WriteJmp(0x0706059, 0x0706070);

    // @daemon_n
    // отключаем командира инферно, забирающего армию не у нейтральных армий
    _PI->WriteLoHook(0x075A615, NPC_DisableInfernoCommander);
    //
    //   // увеличение количества кристаллов до 6;
    //   // ResetNpc
    //   _PI->WriteByte(0x0770BD6 + 3, 13);
    //   _PI->WriteByte(0x0770C17 + 3, 13);

    //   // WoG visit MapItem
    //    _PI->WriteByte(0x0705BDB + 2, 12);
    //    // WoG visit Gem
    //   _PI->WriteByte(0x0706053 + 3, 22);
    //   _PI->WriteLoHook(0x0770967, Hero_PickupNPCGem);
}
