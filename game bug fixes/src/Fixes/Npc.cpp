#include "../../WoG_NPC.h"
#include "../../include/Plugin.h"
#include <cstdlib>
// NPC layout is taken from provided headers/WoG/NPC.h, not the obsolete _Npc_.
static_assert(sizeof(WoG::NPC) == 296, "WoG commander structure mismatch");
namespace gbf
{
namespace npc
{
namespace
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// фикс: командиры, имеющие флаг стрельбы и двойной атаки, в рукопашной бъют один раз.
// Исправим это, ибо они уникальны (03/12/2023)
_LHF_(Y_NPC_FixDoubleAttackOnMelle)
{
    h3::H3CombatCreature *stack = (h3::H3CombatCreature *)c->edi;
    // CF_DOUBLEATTACK уже проверена к этому моменту
    // т.е. мы гарантированно знаем, что стек имеет двойную атаку
    if (stack->info.shooter)
    {
        if (Era::IsCommanderId(stack->type))
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

__int64 getAIValue_NPC(int heroID)
{
    __int64 value = 50;

    if (heroID < 0 || heroID >= P_HeroCount)
        return value; // выход, если номер героя неправильный

    WoG::NPC *npc = WoG::NPC::Get(heroID); // структура командира
    if (!npc || npc->on < 1 || !npc->alive)
        return value; // выход, если командиров нет (-1), уволен или мертв

    h3::H3Hero *hero = h3::H3Main::Get()->GetHero(heroID);

    const int attNPC = npc->CalculatePrimarySkill(WoG::ATTACK) + hero->primarySkill[0];
    const int defNPC = npc->CalculatePrimarySkill(WoG::DEFENCE) + hero->primarySkill[1];
    const int hpNPC = npc->CalculatePrimarySkill(WoG::HEALTH);
    const int damageNPC = npc->CalculatePrimarySkill(WoG::DAMAGE);
    const int speedNPC = npc->CalculatePrimarySkill(WoG::SPEED);
    const int isShoot = npc->abilities.shooting ? 2 : 1;

    value =
        ((static_cast<__int64>(attNPC) * damageNPC * speedNPC * isShoot + static_cast<__int64>(defNPC) * hpNPC) >> 2);

    return value;
}

bool check_AIValue_isNotHero = false;

int __stdcall get_AIValue_Hook(HiHook *hook, int army)
{
    int value;

    int armyNPC = army;

    value = THISCALL_1(int, hook->GetDefaultFunc(), army);

    if (!check_AIValue_isNotHero)
    {
        int heroID = *(int *)(armyNPC - 119);

        if (heroID >= 0 && heroID < P_HeroCount)
            value += (int)getAIValue_NPC(heroID);
    }

    check_AIValue_isNotHero = false;

    if (value < 0 || value >= (INT_MAX - 500))
    {
        int random = FASTCALL_2(int, 0x50C7C0, 550, 600);
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
    h3::H3Hero *hero = h3::H3CombatManager::Get()->hero[c->ebx];

    if (!hero)
        return EXEC_DEFAULT;

    c->ecx += static_cast<int>(getAIValue_NPC(hero->id));

    return EXEC_DEFAULT;
}

_LHF_(Y_FixCrit_SuccubusNotCharmed)
{
    // c->return_address = 0x76CECC;
    // return NO_EXEC_DEFAULT;

    DWORD hero = h3::DwordAt(c->ebp + 0x8);

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
    int bruteGold = h3::IntAt(c->ebp - 64);
    if (bruteGold < 0)
    {
        bruteGold = INT_MAX;
    }
    int playerGold = h3::IntAt(c->ebp - 68);

    __int64 summ = static_cast<__int64>(bruteGold) + playerGold;

    if (summ > INT_MAX)
    {
        summ = INT_MAX;
    }
    playerGold = static_cast<int>(summ) - bruteGold;

    h3::IntAt(c->ebp - 68) = playerGold;
    h3::IntAt(c->ebp - 64) = bruteGold;

    bruteGoldAdded = bruteGold;

    return EXEC_DEFAULT;
}
// отображение золота Зверя в окошке просмотра информации после битвы
void __stdcall OnAfterBattleUniversal(Era::TEvent *event)
{
    if (bruteGoldAdded && h3::IntAt(0x6995A4) == h3::H3CurrentPlayerID::Get())
    {
        auto msg = CDECL_3(char *, 0x77710B, 175, 1, 0x2860724);
        THISCALL_4(void, 0x415FC0, h3::H3AdventureManager::Get(), msg, 6, bruteGoldAdded);
        bruteGoldAdded = 0;
    }
}
/// исправления Slavas's Ring
// @ daemon_n
// корректировка количества заклинаний Командира с кольцом Славы
_LHF_(NPC_InitInCombat)
{
    WoG::NPC *npc = reinterpret_cast<WoG::NPC *>(c->ecx);
    if (npc && npc->WearsArtifact(155))
    {
        h3::H3CreatureInformation *creatureInfo = *reinterpret_cast<h3::H3CreatureInformation **>(c->ebp + 0xC);
        if (creatureInfo->spellCharges < 2)
        {
            creatureInfo->spellCharges = 2;
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(NPC_CalculatePrimarySkills)
{

    WoG::NPC *npc = reinterpret_cast<WoG::NPC *>(c->ecx);
    if (npc && npc->skills.asArray[c->eax] < 2 && npc->WearsArtifact(155))
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
    const auto *mgr = *reinterpret_cast<h3::H3CombatManager **>(c->ebp + 0x8);

    if (mgr->creatureBank                                           // если банк
        || mgr->town && mgr->siegeKind2 && h3::IntAt(c->ebp + 0x10) // или и город, и со стенами, и это защитник,
               && h->GetAddress() == 0x76B97B)                      // и пытается поместить баллисту
    {
        h3::IntAt(c->ebp - 0xC) = 1;
    }
    return EXEC_DEFAULT;
}

void __stdcall OnAfterCreateWindow(Era::TEvent *event)
{
    // @ daemon_n
    // фикс WoG бага, когда командиры призывают боевые машины в банках существ
    constexpr LPCSTR fixKey = "gbfe.disable_fix.npc.creature_banks_war_machines";
    if (std::atoi(Era::tr(fixKey, {}).c_str()) == 0)
    {
        instance->WriteLoHook(0x76B889, WoG_CombatStart_SummonNPC); // tent
        instance->WriteLoHook(0x76B97B, WoG_CombatStart_SummonNPC); // ballista
    }
}

// @daemon_n
// отключаем командира инферно, забирающего армию не у нейтральных армий
_LHF_(NPC_DisableInfernoCommander)
{

    const int defenderPlayerId = h3::IntAt(0x02846BC0);
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
    wogObjectType = h3::IntAt(c->ebp - 0x14);
    wogIsHuman = h3::IntAt(c->ebp - 0x4);
    return EXEC_DEFAULT;
}
void __stdcall AdvMgr_Chest_Visit(HiHook *h, h3::H3AdventureManager *AvdManager, h3::H3Hero *Hero,
                                  h3::H3MapItem *mapItem, int PosMixed, char isHuman)
{

    if (wogObjectType >= 17 && wogObjectType <= h3::ByteAt(0x0706053 + 3))
    {
        CDECL_3(void, 0x07708DE, Hero, wogObjectType - 17, wogIsHuman);
        wogObjectType = 0;
    }

    THISCALL_5(void, h->GetDefaultFunc(), AvdManager, Hero, mapItem, PosMixed, isHuman);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Install(PatcherInstance *_PI)
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
    _PI->WriteDword(0x492A63, *(int *)0x44825F);

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

} // namespace
} // namespace npc
void InstallNpc(PatcherInstance *pi)
{
    npc::Install(pi);
}
} // namespace gbf
