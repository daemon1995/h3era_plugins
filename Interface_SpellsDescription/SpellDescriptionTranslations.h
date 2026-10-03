#pragma once

#include <cstddef>

namespace SpellDescriptions
{
namespace Translation
{
enum class Text
{
    Damage, Recovery, Cure, SacrificeTarget, SacrificeSource,
    BookDamage, BookCure, BookHypnotize, BookMine, BookRecovery, BookFirstDamage, BookSummon,
    Demons, Archangel, AreaDamage, ChainDamage, ChainTarget, ResistanceCondition, ChainIndex,
    DurationRounds, DurationBattle, DurationNextAttack, DurationNextTurn,
    BattleDurationTarget, BattleDurationArea, BookDuration, Count
};

struct Definition
{
    const char *key;
    const char *arguments;
};

#define ERA_SPELL_TEXT(section, field) "interface_spells_description." #section "." #field

// Keys and printf signatures only. The mod owns all translated text.
constexpr Definition TEXTS[] = {
    {ERA_SPELL_TEXT(battle, damage), "ssss"},
    {ERA_SPELL_TEXT(battle, recovery), "ssss"},
    {ERA_SPELL_TEXT(battle, cure), "ss"},
    {ERA_SPELL_TEXT(battle, sacrificeTarget), "sss"},
    {ERA_SPELL_TEXT(battle, sacrificeSource), "sssss"},
    {ERA_SPELL_TEXT(book, damage), "d"},
    {ERA_SPELL_TEXT(book, cure), "d"},
    {ERA_SPELL_TEXT(book, hypnotize), "d"},
    {ERA_SPELL_TEXT(book, mine), "d"},
    {ERA_SPELL_TEXT(book, recovery), "d"},
    {ERA_SPELL_TEXT(book, firstDamage), "d"},
    {ERA_SPELL_TEXT(book, summon), "d"},
    {ERA_SPELL_TEXT(battle, demons), "ss"},
    {ERA_SPELL_TEXT(battle, archangel), "ss"},
    {ERA_SPELL_TEXT(battle, areaDamage), "sssss"},
    {ERA_SPELL_TEXT(battle, chainDamage), "ssssss"},
    {ERA_SPELL_TEXT(battle, chainTarget), "sssss"},
    {ERA_SPELL_TEXT(battle, resistanceCondition), ""},
    {ERA_SPELL_TEXT(battle, chainIndex), "s"},
    {ERA_SPELL_TEXT(duration, rounds), "s"},
    {ERA_SPELL_TEXT(duration, battle), ""},
    {ERA_SPELL_TEXT(duration, nextAttack), ""},
    {ERA_SPELL_TEXT(duration, nextTurn), ""},
    {ERA_SPELL_TEXT(battle, durationTarget), "sss"},
    {ERA_SPELL_TEXT(battle, durationArea), "ss"},
    {ERA_SPELL_TEXT(book, duration), "s"},
};
#undef ERA_SPELL_TEXT

static_assert(sizeof(TEXTS) / sizeof(TEXTS[0]) == static_cast<std::size_t>(Text::Count),
              "Missing spell text key");

// Validates presence and the exact signature on each request, without a cache.
const char *GetText(Text text);
} // namespace Translation
} // namespace SpellDescriptions
