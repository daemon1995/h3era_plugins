#include "../../include/Plugin.h"
#include <cstdio>

namespace gbf {
namespace spells {
namespace {
// spell data, hero/town and map item layouts come from H3API, not the old headers.
static_assert(sizeof(h3::H3Position) == 4, "SoD mixed coordinate ABI");
static_assert(offsetof(h3::H3Main, disabledSpells) == 0x4A, "Spell-ban offset changed");
static_assert(offsetof(h3::H3Main, disabledShrines) == 0x04, "Shrine-ban offset changed");

_LHF_(ViewEarthAirMonoliths) {
    c->edi = c->ecx;
    c->ecx = *reinterpret_cast<DWORD*>(c->esi + 0x1E);
    const BYTE visible = *reinterpret_cast<BYTE*>(c->ebp + 11);
    if (visible) {
        const int object = c->ecx;
        const int type = (object == 43 || object == 44 || object == 45) ? 3 :
                         (object == 103 ? 4 : 0);
        if (type) {
            // Same 6-argument __fastcall native function as in the original plugin.
            FASTCALL_6(int, 0x5F7760, *reinterpret_cast<int*>(0x6AAC88),
                       c->esi, type, *reinterpret_cast<int*>(0x6AAC80) +
                       *reinterpret_cast<int*>(c->ebp + 0x14) * *reinterpret_cast<int*>(0x68C70C),
                       c->edi + 8, *reinterpret_cast<int*>(c->ebp + 0x12));
        }
    }
    c->return_address = 0x5F854A;
    return NO_EXEC_DEFAULT;
}

// Old _Hero_::GetSpell_Specialisation_PowerBonuses(spell, damage, level)
// corresponds to H3Hero::GetSpellSpecialtyEffect(spell, level, damage).
__int64 __stdcall SpellbookSpecialty(HiHook* hook, h3::H3Hero* hero, int spell,
                                    int damage, h3::H3CombatCreature* stack) {
    __int64 value = THISCALL_4(__int64, hook->GetDefaultFunc(), hero, spell, damage, stack);
    if (hero) value += hero->GetSpellSpecialtyEffect(spell, 0, static_cast<int>(value));
    return value;
}
int __stdcall WogSpellSpecialty(HiHook*, h3::H3Hero* hero, int spell, int level, int damage) {
    const int result = hero ? hero->GetSpellSpecialtyEffect(spell, level, damage) : 0;
    h3::IntAt(0x83EEFC) = result;
    return result;
}
int __stdcall ArmageddonStatus(HiHook* hook, h3::H3CombatManager* battle, int damage,
                               int spell, h3::H3Hero* attackingHero, h3::H3Hero* defendingHero,
                               h3::H3CombatCreature* stack, int parameter) {
    if (!attackingHero && battle && battle->currentActiveSide >= 0 && battle->currentActiveSide < 2)
        attackingHero = battle->hero[battle->currentActiveSide];
    return THISCALL_7(int, hook->GetDefaultFunc(), battle, damage, spell,
                      attackingHero, defendingHero, stack, parameter);
}
_LHF_(PoisonDoesNotBlockDispel) {
    return c->eax == 71 ? NO_EXEC_DEFAULT : EXEC_DEFAULT; // WoG poison spell id; see original SPL_POISON
}
_LHF_(DimensionDoorTarget) {
    auto* adv = h3::H3AdventureManager::Get();
    auto* hero = h3::H3ActivePlayer::Get()->GetActiveHero();
    if (!adv || !hero || !h3::H3Main::Get()) return EXEC_DEFAULT;
    DWORD mixed = 0;
    DWORD* mousePos = THISCALL_2(DWORD*, 0x407A70, adv, &mixed);
    if (!mousePos || !THISCALL_1(bool, 0x4B1090, mousePos)) return EXEC_DEFAULT;
    const auto* target = reinterpret_cast<const h3::H3MapItem*>(c->eax);
    const auto* origin = h3::H3Main::Get()->mainSetup.GetMapItem(hero->x, hero->y, hero->z);
    if (target && origin && target != origin && (origin->land == 8) == (target->land == 8))
        return EXEC_DEFAULT;
    c->return_address = 0x4915D3;
    return NO_EXEC_DEFAULT;
}
_LHF_(VisionsMonsterCount) {
    auto* hero = *reinterpret_cast<h3::H3Hero**>(c->ebp - 0x10);
    if (hero && THISCALL_1(int, 0x4E6050, hero) > -1) {
        auto* cell = reinterpret_cast<h3::H3MapItem*>(c->ebx);
        if (cell) {
            // Monster visibility is based on map position, not tile metadata.
            const auto* origin = h3::H3Main::Get()->mainSetup.GetMapItem(0, 0, 0);
            const unsigned size = h3::H3MapSize::Get();
            if (origin && size && cell >= origin) {
                const size_t offset = static_cast<size_t>(cell - origin);
                const h3::H3Position pos(static_cast<short>(offset % size),
                                         static_cast<short>((offset / size) % size),
                                         static_cast<char>(offset / (size * size)));
                DWORD packed = pos.Mixed();
                if (THISCALL_2(bool, 0x4E6080, hero, &packed)) {
                    // Global H3API text buffer holds at least 0x300 bytes in SoD.
                    auto* text = reinterpret_cast<char*>(0x697428);
                    std::snprintf(text, 0x300, "%s: %u", reinterpret_cast<const char*>(c->eax), cell->setup & 0xFFF);
                    c->return_address = 0x40C2F9;
                    return NO_EXEC_DEFAULT;
                }
            }
        }
    }
    return EXEC_DEFAULT;
}
// Same temporary patch scope as the original plugin. These instructions operate on
// the unmodified SoD/WoG spell immunity table and must not be re-entered.
h3::H3CombatCreature* __stdcall SacrificeTarget(HiHook* hook, h3::H3CombatManager* battle,
    int spell, int side, int hex, char unknown, int casterKind) {
    constexpr int SACRIFICE = 40;
    BYTE saved = 0;
    if (spell == SACRIFICE) {
        saved = h3::ByteAt(0x5A4056);
        h3::ByteAt(0x5A4056) = SACRIFICE;
        h3::ByteAt(0x5A4111) = SACRIFICE;
    }
    auto* ret = THISCALL_6(h3::H3CombatCreature*, hook->GetDefaultFunc(), battle,
                           spell, side, hex, unknown, casterKind);
    if (spell == SACRIFICE) {
        h3::ByteAt(0x5A4056) = saved;
        h3::ByteAt(0x5A4111) = saved;
    }
    return ret;
}
void __stdcall ShareTownSpells(HiHook* hook, h3::H3Town* town, h3::H3Hero* hero) {
    THISCALL_2(void, hook->GetDefaultFunc(), town, hero);
    if (!town || town->garrisonHero == -1) return;
    auto* upper = h3::H3Main::Get()->GetHero(town->garrisonHero);
    if (!hero && town->visitingHero != -1)
        hero = h3::H3Main::Get()->GetHero(town->visitingHero);
    if (!upper || !hero) return;
    const WORD saved1 = h3::WordAt(0x4A26D5);
    const WORD saved2 = h3::WordAt(0x4A271E);
    h3::WordAt(0x4A26D5) = 0x66EB;
    h3::WordAt(0x4A271E) = 0x1DEB;
    FASTCALL_2(void, 0x4A25B0, upper, hero);
    h3::WordAt(0x4A26D5) = saved1;
    h3::WordAt(0x4A271E) = saved2;
}
int __stdcall LoadSpellBan(HiHook* hook, int array, char enable) {
    const int spell = *reinterpret_cast<const int*>(array + 4);
    if (spell >= 0 && spell < 70) {
        h3::H3Main::Get()->disabledShrines[spell] = enable;
        h3::H3Main::Get()->disabledSpells[spell] = enable;
    }
    return THISCALL_2(int, hook->GetDefaultFunc(), array, enable);
}
} // namespace

void Install(PatcherInstance* pi) {
    h3::H3Spell::Get()[11].damageSpell = true; // Land Mine
    h3::H3Spell::Get()[13].damageSpell = true; // Fire Wall
    // возможность заходить в гильдию магов без наличия книги и денег у героя-гостя
    pi->WriteHexPatch(0x5CEA83, "EB74");
    pi->WriteHexPatch(0x5CEACD, "2800");
    // корректировка описаний заклинаний в книге (не учитывались бонусы специалистов по заклинаниям)
    pi->WriteHiHook(0x59BFFD, CALL_, SAFE_, THISCALL_, SpellbookSpecialty);
    // исправление бага WOG: IDummy(0x83EEFC) иногда может возвращать мусор см.функцию 0x75D654
    pi->WriteHiHook(0x5A1C7B, CALL_, EXTENDED_, THISCALL_, WogSpellSpecialty);
    // фиксим неотображение Монолитов и Подземных врат в диалоге заклинания Просмотр Земли и Воздуха
    pi->WriteLoHook(0x5F8545, ViewEarthAirMonoliths);
    // фикс неправильного отображения величины урона в окне статуса битвы при касте заклинания Армагеддон
    pi->WriteHiHook(0x5A5522, CALL_, EXTENDED_, THISCALL_, ArmageddonStatus);
    // Наличие яда теперь не даёт колдовать Снятие Заклинаний.
    pi->WriteLoHook(0x5A84BD, PoisonDoesNotBlockDispel);
    // Фикс бага SoD: дд - возможность сделать каст в самого героя и отмена при попытке прыгнуть с суши на воду и за пределы карты
    pi->WriteLoHook(0x4915B7, DimensionDoorTarget);
    // добавление точного количества монстров в хинт бар Карты Приключений при активном заклинании Видение
    pi->WriteLoHook(0x40C2D3, VisionsMonsterCount);
    // исправление невозможности воскресить Золотых Драконов (и прочую шушеру) через жертву (из-за иммунитета к воскрешению)
    pi->WriteHiHook(0x5A3C60, SPLICE_, EXTENDED_, THISCALL_, SacrificeTarget);
    // исправление бага с невозможностью изучить заклинания города при телепортации героя в город
    pi->WriteWord(0x41D9E3, 0x9056);
    // изучение заклинаний в городе героем автоматически вызывает обмен заклинаниями героев в городе;
    pi->WriteHiHook(0x5BE430, SPLICE_, EXTENDED_, THISCALL_, ShareTownSpells);
    if (!Era::PluginExists("typhon"))
        // заполняем таблицу при загрузке карты
        pi->WriteHiHook(0x4C2445, CALL_, EXTENDED_, THISCALL_, LoadSpellBan);
}
} // namespace spells
void InstallSpells(PatcherInstance* pi) { spells::Install(pi); }
} // namespace gbf
