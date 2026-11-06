#pragma once
#include "pch.h"
#include "../headers/SpellLimits.h"

#include <array>
#include <climits>

namespace creatureInfo
{
constexpr int MAX_COMBAT_SPELL_COUNT = UCHAR_MAX;
constexpr int SPELL_DEF_FRAME_OFFSET = 1;
constexpr int SPELL_DURATION_BUFFER_SIZE = 16;

inline INT32 GetSpellDuration(const H3CombatCreature *stack, int spellId) noexcept
{
    // The spell extender uses the following array's memory for extra durations.
    // Address the patched storage without indexing H3API's fixed 81-element array.
    const auto *durations = reinterpret_cast<const unsigned char *>(&stack->activeSpellDuration);
    return *reinterpret_cast<const INT32 *>(durations + spellId * sizeof(INT32));
}

enum SpellHintTextId : int
{
    SPELL_DURATION_HINT = 612,
    SPELL_SPECIAL_HINT = 681,
    SPELL_BIND_HINT = 682,
    SPELL_BERSERK_HINT = 683,
    SPELL_DISRUPTING_RAY_HINT = 684
};

struct ActiveSpellList
{
    std::array<int, MAX_COMBAT_SPELL_COUNT> ids = {};
    int count = 0;
};

inline ActiveSpellList CollectActiveSpells(const H3CombatCreature *stack)
{
    ActiveSpellList result;
    const int spellCount = spellLimits::GetSpellCount();
    if (stack)
        for (int spell = 0; spell < spellCount; ++spell)
            if (GetSpellDuration(stack, spell))
                result.ids[result.count++] = spell;
    return result;
}

inline bool HasVisibleSpellDuration(int spellId)
{
    switch (spellId)
    {
    case h3::eSpell::BERSERK:
    case h3::eSpell::DISRUPTING_RAY:
    case h3::eSpell::BIND:
        return false;
    default:
        return true;
    }
}

inline void SetCreatureSpellHints(H3DlgItem *item, const H3CombatCreature *stack, int spellId)
{
    if (!item || !stack || spellId < 0 || spellId >= spellLimits::GetSpellCount())
        return;
    const char *name = h3::H3Spell::Get()[spellId].name;
    const char *description = h3::H3Spell::Get()[spellId].description[0];
    if (!description || !*description)
        description = name;

    int specialTextId = -1;
    switch (spellId)
    {
    case h3::eSpell::BIND: specialTextId = SPELL_BIND_HINT; break;
    case h3::eSpell::BERSERK: specialTextId = SPELL_BERSERK_HINT; break;
    case h3::eSpell::DISRUPTING_RAY: specialTextId = SPELL_DISRUPTING_RAY_HINT; break;
    default: break;
    }
    auto *generalText = h3::H3GeneralText::Get();
    if (specialTextId >= 0)
        h3::libc::sprintf(h3::h3_TextBuffer, generalText->GetText(SPELL_SPECIAL_HINT), name,
                          generalText->GetText(specialTextId));
    else
        h3::libc::sprintf(h3::h3_TextBuffer, generalText->GetText(SPELL_DURATION_HINT), name,
                          GetSpellDuration(stack, spellId));
    item->SetHints(h3::h3_TextBuffer, description, TRUE);
}
} // namespace creatureInfo
