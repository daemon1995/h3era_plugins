#include "../../include/Plugin.h"
namespace gbf
{
namespace
{
_LHF_(PreventAISplitDivZero)
{
    if (c->ebx == 0)
        c->ebx = 1;
    return EXEC_DEFAULT;
}
_LHF_(PreventAICombatDivZero)
{
    if (c->edi == 0)
        c->edi = 1;
    return EXEC_DEFAULT;
}
_LHF_(FixAIConfluxMerchant)
{
    if ((c->eax & 0xFF) == 8)
    { // Conflux
        *reinterpret_cast<int *>(c->ebp - 36) = c->ecx;
        c->return_address = 0x525EE0;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
_LHF_(AIWaterwalkFly)
{
    if (c->eax != 0)
        return EXEC_DEFAULT; // Angel Wings
    const auto *hero = reinterpret_cast<const h3::H3Hero *>(c->esi);
    if (!hero)
        return EXEC_DEFAULT;
    if (!hero->learnedSpells[6] && !hero->availableSpell[6] && (hero->learnedSpells[7] || hero->availableSpell[7]))
    {
        c->return_address = hero->waterwalkPower == -1 ? 0x430231 : 0x430540;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
_LHF_(AITownPortalCursedGround)
{
    const auto *hero = reinterpret_cast<const h3::H3Hero *>(c->esi);
    if (hero && hero->GetSpecialTerrain() == 0x15)
    {
        c->return_address = 0x56B6F4;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
int __stdcall ClampAISwapValue(HiHook *hook, DWORD that, int creatureId, short quantity, short *slot, char replace)
{
    const int result = THISCALL_5(int, hook->GetDefaultFunc(), that, creatureId, quantity, slot, replace);
    return result < 0 ? 0 : result;
}
// AI estimates for immobile creatures / wall machines must not divide by zero.
int __stdcall ReachRangedTarget(HiHook *hook, DWORD that, h3::H3CombatCreature *stack, int parameter)
{
    if (!stack || stack->info.siegeWeapon || stack->info.speed < 1)
        return 99;
    return THISCALL_3(int, hook->GetDefaultFunc(), that, stack, parameter);
}
// WoG extends jousting to new creatures. The original WoG helper accepts
// EBX=attacker, EDI=defender (not the usual thiscall/fastcall ABI).
_LHF_(CavalryBonus)
{
    auto *attacker = reinterpret_cast<h3::H3CombatCreature *>(c->ecx);
    if (!attacker || attacker->info.flyer)
        return EXEC_DEFAULT;
    auto *defender = *reinterpret_cast<h3::H3CombatCreature **>(c->esi + 8);
    int result;
    __asm {
        mov ebx, attacker
        mov edi, defender
        mov eax, 0x075D7F5
        call eax
        mov result, eax
    }
    c->ecx = result;
    c->return_address = 0x436208;
    return NO_EXEC_DEFAULT;
}
_LHF_(DefenderDamageEstimate)
{
    c->eax = THISCALL_3(int, 0x426390, c->edi, h3::IntAt(c->ebp - 8), FALSE);
    c->return_address = 0x426D22;
    return NO_EXEC_DEFAULT;
}
_LHF_(DefenderApplyDamage)
{
    c->eax = THISCALL_4(int, 0x426170, c->edi, c->eax, 1, h3::IntAt(c->ebp - 8));
    c->return_address = 0x426D57;
    return NO_EXEC_DEFAULT;
}
static bool ignoreShootingObstacles;
void __stdcall SetupAICombat(HiHook *hook, DWORD that, double coeff, h3::H3Hero *enemy)
{
    auto *ourHero = *reinterpret_cast<h3::H3Hero **>(that + 36);
    ignoreShootingObstacles = ourHero && (ourHero->WearsArtifact(91) || ourHero->WearsArtifact(137));
    const DWORD *words = reinterpret_cast<const DWORD *>(&coeff);
    // The x86 native signature passes the double coefficient as two words.
    THISCALL_4(void, hook->GetDefaultFunc(), that, words[0], words[1], enemy);
}
float __stdcall AddAIBaseShootingDamage(HiHook *hook, h3::H3Hero *hero)
{
    return 1.0f + THISCALL_1(float, hook->GetDefaultFunc(), hero);
}
_LHF_(ObstaclesForSpecialShooters)
{
    if (ignoreShootingObstacles)
    {
        c->return_address = 0x424429;
        return NO_EXEC_DEFAULT;
    }
    const int type = h3::IntAt(c->ebp - 4);
    switch (type)
    {
    case 34:
    case 136:
    case 137:
    case 149:
    case 170:
    case 171:
        c->return_address = 0x424429;
        return NO_EXEC_DEFAULT;
    default:
        return EXEC_DEFAULT;
    }
}
// Old _AIQuickBattle_ has no direct H3API counterpart. The exact legacy
// x86 ABI is preserved as a 0x34-byte overlay instead of treating it as H3Army.
#pragma pack(push, 1)
struct QuickBattleSide
{
    DWORD opaque[4];      // +00
    DWORD specialTerrain; // +10
    DWORD spellPoints;    // +14
    BYTE canCastSpells;   // +18
    BYTE pad19[3];        // +19
    INT32 armyStrength;   // +1C
    BYTE tactics;         // +20
    BYTE pad21[3];        // +21
    h3::H3Hero *heroA;    // +24
    h3::H3Army *army;     // +28
    h3::H3Hero *heroD;    // +2C
    BYTE turrets;         // +30
    BYTE pad31;           // +31
    WORD turretsLevel;    // +32
    void DeleteCreatures()
    {
        THISCALL_2(void, 0x424880, this, 1);
    }
};
#pragma pack(pop)
static_assert(sizeof(QuickBattleSide) == 0x34, "Quick Battle ABI");
static_assert(offsetof(QuickBattleSide, armyStrength) == 0x1C, "Quick Battle armyStrength offset");
static_assert(offsetof(QuickBattleSide, army) == 0x28, "Quick Battle army offset");
_LHF_(AINecromancyKeepLosers)
{
    c->return_address = 0x426FED;
    return NO_EXEC_DEFAULT;
}
void __stdcall AINecromancyRestore(HiHook *hook, QuickBattleSide *winner, QuickBattleSide *loser, int town)
{
    THISCALL_3(void, hook->GetDefaultFunc(), winner, loser, town);
    if (loser)
        loser->DeleteCreatures();
}
int __stdcall AIQuickBattleDamage(HiHook *hook, QuickBattleSide *self, int a, int b)
{
    int result = THISCALL_3(int, hook->GetDefaultFunc(), self, a, b);
    if (self && self->armyStrength > 0 && result < 0)
        return INT_MAX;
    return result;
}
void __stdcall AIQuickBattleHugeArmy(HiHook *hook, QuickBattleSide *self, int a, int b, int d)
{
    if (self && self->armyStrength < 0)
        self->armyStrength = INT_MAX;
    THISCALL_4(void, hook->GetDefaultFunc(), self, a, b, d);
}
void __stdcall AIQuickBattleNoSpawning(HiHook *hook, QuickBattleSide *self, int a)
{
    if (!self || !self->army)
    {
        THISCALL_2(void, hook->GetDefaultFunc(), self, a);
        return;
    }
    const h3::H3Army previous = *self->army;
    THISCALL_2(void, hook->GetDefaultFunc(), self, a);
    for (int i = 0; i < 7; ++i)
        if (self->army->type[i] != -1 && self->army->count[i] > previous.count[i])
            self->army->count[i] = previous.count[i];
}
void __stdcall ShowHeroInfoAndRefresh(HiHook *hook, int heroId, int a, int b)
{
    THISCALL_3(void, hook->GetDefaultFunc(), heroId, a, b);
    THISCALL_4(void, 0x417540, h3::H3AdventureManager::Get(), 0, 0, 0);
}
} // namespace
void InstallAI(PatcherInstance *pi)
{
    // Original RoseKavalier fixes were gated if another ERA bug-fix module
    // already owns their addresses.
    if (patcher && !patcher->GetInstance("ERA_bug_fixes"))
    {
        // if AI has tactics and shooters, but you have 0 creatures -> crash
        pi->WriteLoHook(0x42DDA6, PreventAISplitDivZero);
        // divides by speed which shouldn't be 0
        pi->WriteLoHook(0x42437D, PreventAICombatDivZero);
        pi->WriteLoHook(0x426FE4, AINecromancyKeepLosers);
        pi->WriteHiHook(0x426EE0, SPLICE_, EXTENDED_, THISCALL_, AINecromancyRestore);
        pi->WriteHiHook(0x426390, SPLICE_, EXTENDED_, THISCALL_, AIQuickBattleDamage);
        pi->WriteHiHook(0x42443B, CALL_, EXTENDED_, THISCALL_, AIQuickBattleHugeArmy);
        pi->WriteHiHook(0x4274D4, CALL_, EXTENDED_, THISCALL_, AIQuickBattleNoSpawning);
        // исправление бага посещения банков в которых дают существ (вылет в диалоге присоедиенния монстров) ©
        // RoseKavalier
        pi->WriteHiHook(0x5D52CA, CALL_, EXTENDED_, THISCALL_, ShowHeroInfoAndRefresh);
        pi->WriteLoHook(0x43020E, AIWaterwalkFly);
        pi->WriteLoHook(0x56B344, AITownPortalCursedGround);

        // небольшое исправление блоков на дорогах
        pi->WriteByte(0x541B6A, 235); // road blocks
        // работающая кнопка Отмена в Арене
        pi->WriteByte(0x49E4ED, 99); // Arena cancel
        // ХЗ  что это (возможно операции со стеками)
        pi->WriteByte(0x49C021, 183);
        pi->WriteByte(0x4A763F, 183);
        pi->WriteByte(0x4A9423, 183);
        pi->WriteByte(0x4AA82E, 183);
        pi->WriteByte(0x4C9662, 183);
        pi->WriteByte(0x4FD164, 183);
        pi->WriteByte(0x505C9F, 183);
        pi->WriteByte(0x52CD36, 183);
        pi->WriteHexPatch(0x4FD12A, "0F BF 78 24 83 FF FF 0F 84");
        pi->WriteHexPatch(0x505C75, "0F BF 77 24 83 FE FF 74");
        pi->WriteHexPatch(0x52CD26, "0F BF 7F 24 83 FF FF 74");
        pi->WriteHexPatch(0x44AA4F, "90 8B 7C 81 1C");
        pi->WriteHexPatch(0x44AA58, "90 89 F8 89 44 B2 1C");
        pi->WriteHexPatch(0x42D922, "90 8B 56 1C 89 45 DC 89 55 E4 90");
        pi->WriteHexPatch(0x42DA39, "8B 4D E4 90 57 51");
        pi->WriteHexPatch(0x42DAD9, "8B 4D E4 90 57 51");
    }

    // AIFixes, enabled unconditionally in the original plugin.
    // © Raistlin
    // Исправляем косяк, из-за которого ИИ не мог использовать торговцев артефактами в Сопряжении
    pi->WriteLoHook(0x525ED1, FixAIConfluxMerchant);
    // Фикс бесконечного цикла ИИ: когда он оценивает ценность получения стека обнуляем отрицательное значение, чтобы
    // избежать переполнения.
    pi->WriteHiHook(0x42C830, SPLICE_, EXTENDED_, THISCALL_, ClampAISwapValue);
    // AI БИТВА
    // AI всегда будет получать артефакты и опыт за побеждённого врага
    pi->WriteByte(0x426F41, 0);
    // Исправление бага SoD с передачей артефактов побеждённому компьютеру.
    pi->WriteHexPatch(0x426FA2, "CA");
    pi->WriteHexPatch(0x426FA3, "53");
    // Исправляем баг SoD: ИИ считал, что облако личей не задевает только нежить.
    pi->WriteByte(0x41EFA8, 4);
    pi->WriteHexPatch(0x41EFAC, "75 16");
    // временно отключаем проверку на каст заклинаний для ИИ в теоретических битвах
    pi->WriteByte(0x423F4A, 0);

    // AI битва (просчёт)
    // проверка на скорость монстра и когда он дойдет до защиты стрелка.
    // Убираем из проверки существ с нулевой скоростью и боевые машины
    pi->WriteHiHook(0x4B3C80, SPLICE_, EXTENDED_, THISCALL_, ReachRangedTarget);
    auto *info = h3::H3CreatureInformation::Get();
    if (info)
        info[eCreature::DRACOLICH].fireballAttack = true; // Dracolich, WoG creature
    // Снимаем ограничение для ИИ в 8 героев на команду
    // © daemon_n
    // фикс бага WoG -- для ИИ не был добавлен рассчёт кавалерийского бонуса для новых существ и SE:
    pi->WriteLoHook(0x436200, CavalryBonus);
    // Исправление багов ИИ-ИИ битвы.
    // При расчёте ИИ-ИИ битвы при нападении защищающегося исправляем параметры вычисления урона.
    pi->WriteLoHook(0x426D17, DefenderDamageEstimate);
    // При расчёте ИИ-ИИ битвы при нападении защищающегося исправляем параметры нанесения урона.
    pi->WriteLoHook(0x426D4B, DefenderApplyDamage);
    // Перед настройкой стеков при расчёте битвы инициализируем некоторые значения.
    pi->WriteHiHook(0x424120, SPLICE_, EXTENDED_, THISCALL_, SetupAICombat);
    // При расчёте ИИ-ИИ битвы при взятии модификатора стрелковой атаки учитываем и собственные 100% урона стека (баг
    // SoD).
    pi->WriteHiHook(0x42426D, CALL_, EXTENDED_, THISCALL_, AddAIBaseShootingDamage);
    // Учитываем всех стреляющих сквозь препятствия стрелков (а не только архимагов).
    pi->WriteLoHook(0x424412, ObstaclesForSpecialShooters);
}
} // namespace gbf
