#include "SpellDescriptions.h"
#include "SpellDescriptionErm.h"
#include "SpellDescriptionHintApi.h"
#include "SpellDescriptionLogic.h"
#include "SpellDescriptionText.h"
#include "SpellDescriptionOverlay.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace SpellDescriptions
{
namespace
{
using Detail::Kind;
constexpr int BATTLE_HEXES = 187;
constexpr size_t TEXT_CAPACITY = 512;
constexpr size_t API_CAPACITY = 1024;
constexpr int COMMANDER_FIRST = 174;
constexpr int COMMANDER_LAST = 191;

enum class Text
{
    Damage,
    Recovery,
    Cure,
    SacrificeTarget,
    SacrificeSource,
    BookDamage,
    BookCure,
    BookHypnotize,
    BookMine,
    BookRecovery,
    BookFirstDamage,
    BookSummon,
    Demons,
    Archangel,
    AreaDamage,
    ChainDamage,
    ChainTarget,
    ResistanceCondition,
    ChainIndex,
    DurationRounds,
    DurationBattle,
    DurationNextAttack,
    DurationNextTurn,
    DurationRoundsOrAttack,
    DurationRoundsOrDamage,
    BattleDurationTarget,
    BattleDurationArea,
    BookDuration,
    Count
};

struct TextDefinition
{
    LPCSTR key;
    LPCSTR arguments;
};

#define ERA_SPELL_TEXT(section, field) "interface_spells_description." #section "." #field

// Store JSON keys and printf argument signatures only, like the settings dialog.
const TextDefinition TEXTS[] = {
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
    {ERA_SPELL_TEXT(duration, roundsOrAttack), "s"},
    {ERA_SPELL_TEXT(duration, roundsOrDamage), "s"},
    {ERA_SPELL_TEXT(battle, durationTarget), "sss"},
    {ERA_SPELL_TEXT(battle, durationArea), "ss"},
    {ERA_SPELL_TEXT(book, duration), "s"},
};
static_assert(sizeof(TEXTS) / sizeof(TEXTS[0]) == static_cast<size_t>(Text::Count), "Missing spell text key");

bool initialized = false;
bool previewInProgress = false;
bool queryInProgress = false;

INT32 *ErmAddress(DWORD address)
{
    return static_cast<INT32 *>(Era::GetRealAddr(reinterpret_cast<void *>(address)));
}

class ScopedAssocValue
{
    LPCSTR key;
    int saved;

  public:
    ScopedAssocValue(LPCSTR name, int value) : key(name), saved(Era::GetAssocVarIntValue(name))
    {
        Era::SetAssocVarIntValue(key, value);
    }
    ~ScopedAssocValue()
    {
        Era::SetAssocVarIntValue(key, saved);
    }
    ScopedAssocValue(const ScopedAssocValue &) = delete;
    ScopedAssocValue &operator=(const ScopedAssocValue &) = delete;
};

class ScopedSpellPreview
{
    Detail::ScopedValues<bool, 1> recursion;
    Detail::ScopedValues<eCombatAction, 1> action;
    Detail::ScopedValues<INT32, 4> parameters;
    Detail::ScopedValues<INT32, 8> mr;
    ScopedAssocValue side;
    ScopedAssocValue preview;

  public:
    ScopedSpellPreview(H3CombatManager *combat, int spellId, int hex, int casterSide, int casterKind)
        : recursion({&previewInProgress}), action({&combat->action}),
          parameters(
              {&combat->actionParameter, &combat->actionTarget, &combat->actionParameter2, &combat->currentActiveSide}),
          // Resolve relocated WoG globals; preserve MR0/MR1 and MR2 scratch.
          mr({ErmAddress(0x2860214), ErmAddress(0x286021C), ErmAddress(0x2846898), ErmAddress(0x283282C),
              ErmAddress(0x2846884), ErmAddress(0x2860254), ErmAddress(0x2846870), ErmAddress(0x2860258)}),
          side("battle_current_side", casterSide), preview("era.opt.spells.preview", 1)
    {
        previewInProgress = true;
        combat->action = casterKind == 1 ? eCombatAction::MONSTER_SPELL : eCombatAction::CAST_SPELL;
        combat->actionParameter = spellId;
        combat->actionTarget = hex;
        combat->actionParameter2 = 0;
        combat->currentActiveSide = casterSide;
    }
};

LPCSTR GetText(Text text)
{
    const auto &definition = TEXTS[static_cast<size_t>(text)];
    bool found = false;
    const auto format = EraJS::read(definition.key, found);
    return found && Detail::ValidFormat(format, definition.arguments) ? format : nullptr;
}

bool FormatV(char *output, size_t capacity, LPCSTR format, va_list arguments)
{
    if (!output || capacity < 2 || !format)
        return false;
    const int length = _vsnprintf_s(output, capacity, _TRUNCATE, format, arguments);
    if (length <= 0)
    {
        output[0] = 0;
        return false;
    }
    return true;
}

bool Format(char *output, size_t capacity, Text text, ...)
{
    va_list arguments;
    va_start(arguments, text);
    const bool formatted = FormatV(output, capacity, GetText(text), arguments);
    va_end(arguments);
    return formatted;
}

// A validated per-request template avoids repeating JSON reads for every hop.
bool Format(char *output, size_t capacity, LPCSTR format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const bool formatted = FormatV(output, capacity, format, arguments);
    va_end(arguments);
    return formatted;
}

// Preserve full numbers: abbreviated k/M values hide the exact damage.
Detail::DecimalNumber Number(std::int64_t value)
{
    return Detail::DecimalNumber(value);
}

bool ValidSpell(int spellId)
{
    return spellId >= 0 && spellId < limits::SPELLS;
}

bool ValidSide(int side)
{
    return side == 0 || side == 1;
}

H3CombatCreature *CurrentCaster(H3CombatManager *combat);

ErmBridge::Arguments QueryArguments(int spellId, ErmBridge::Context context, ErmBridge::Phase phase, Kind kind,
                                    H3Hero *hero, H3CombatManager *combat, int hex, int power, int mastery,
                                    H3CombatCreature *target, int amount = -1)
{
    // The hint API explicitly requests a creature cast even if the battle's
    // current action is still a targeting/UI action. A hero-less battle query
    // therefore carries the actual current caster, not the old action code.
    auto caster = combat && !hero ? CurrentCaster(combat) : nullptr;
    return {{ErmBridge::VERSION, phase, context, spellId, static_cast<int>(P_Spell[spellId].flags),
             hero ? hero->id : -1, caster ? caster->side * 21 + caster->sideIndex : -1,
             target ? target->side * 21 + target->sideIndex : -1, power, mastery, static_cast<int>(kind), amount, 1,
             combat ? combat->currentActiveSide : -1, hex, 0}};
}

bool QueryErm(const ErmBridge::Arguments &request, ErmBridge::Arguments &result)
{
    if (queryInProgress)
        return false;
    int eventId = 0;
    // ERA resets named functions when ERM is recompiled. Resolve by name each
    // time; a false return means an existing ID was reused, not an error.
    Era::AllocErmFunc(ErmBridge::EVENT_NAME, eventId);
    if (!eventId)
        return false;
    return ErmBridge::Dispatch(*Era::GetArgXVars(), *Era::GetRetXVars(), queryInProgress, request, result,
                               [eventId]() { Era::FireErmEvent(eventId); });
}

Kind GetKind(int spellId, ErmBridge::Context context, H3Hero *hero = nullptr, H3CombatManager *combat = nullptr,
             int hex = -1, int power = -1, int mastery = -1, H3CombatCreature *target = nullptr)
{
    if (!ValidSpell(spellId))
        return Kind::None;
    const auto &spell = P_Spell[spellId];
    const auto identity = spellId == eSpell::SACRIFICE   ? Detail::Identity::Sacrifice
                          : spellId == eSpell::TELEPORT  ? Detail::Identity::Teleport
                          : spellId == eSpell::LAND_MINE ? Detail::Identity::LandMine
                          : spellId == eSpell::FIRE_WALL ? Detail::Identity::FireWall
                          : spellId == eSpell::DISRUPTING_RAY ? Detail::Identity::DisruptingRay
                          : spellId == eSpell::CLONE ? Detail::Identity::Clone
                                                         : Detail::Identity::Generic;
    const auto kind = Detail::Classify(identity, spell.flags, spell.type, spell.spEffect);
    const auto request =
        QueryArguments(spellId, context, ErmBridge::Classify, kind, hero, combat, hex, power, mastery, target);
    ErmBridge::Arguments result;
    if (QueryErm(request, result))
        return result[ErmBridge::Enabled] ? static_cast<Kind>(result[ErmBridge::SpellKind]) : Kind::None;
    return kind;
}

bool IsCommander(const H3CombatCreature *stack)
{
    return stack && stack->type >= COMMANDER_FIRST && stack->type <= COMMANDER_LAST;
}

H3CombatCreature *CurrentCaster(H3CombatManager *combat)
{
    if (!combat || !ValidSide(combat->currentActiveSide) || combat->currentMonIndex < 0 ||
        combat->currentMonIndex >= 21)
        return nullptr;
    return &combat->stacks[combat->currentActiveSide][combat->currentMonIndex];
}

int CreaturePower(H3CombatCreature *stack)
{
    if (!stack || stack->numberAlive <= 0)
        return 0;
    if (stack->type == eCreature::FAERIE_DRAGON)
        return Detail::NonnegativeInt(std::int64_t(stack->numberAlive) * 5);
    if (IsCommander(stack))
        return CDECL_1(int, 0x76BEEA, stack); // WoG NPC::GetMagicPower, absent from H3API
    return 0;
}

int CreatureMastery(const H3CombatManager *combat)
{
    // CombatManager::CastSpell (0x5A01D5) upgrades creature casts on Magic Plains.
    return combat->specialTerrain == 1 ? 3 : 2;
}

// The remaining engine calls have no H3API wrapper; all structures are H3API.
bool CanApplySpell(H3CombatManager *combat, H3CombatCreature *target, int spellId, int side, int casterKind)
{
    return THISCALL_6(BOOL8, 0x5A3F90, combat, spellId, side, target, 1, casterKind) != 0;
}

int ReducedDamage(H3CombatManager *combat, H3CombatCreature *target, int spellId, int damage)
{
    // Hero bonuses are already included. The native function sets EDI to
    // target and enters WoG's 0x75D88D wrapper via its 0x5A7C3B call site:
    // MR0 -> standard/commander resistance -> experience -> MR1 -> protection.
    // Applying the internal reductions again would double the resistance.
    return combat->CalculateSpellDamageOnTarget(damage, spellId, nullptr, target->GetOwner(), target, FALSE);
}

int BaseEffect(int spellId, int power, int mastery)
{
    const auto &spell = P_Spell[spellId];
    return Detail::NonnegativeInt(std::int64_t(power) * spell.spEffect + spell.baseValue[mastery]);
}

int HeroEffect(H3Hero *hero, H3CombatCreature *target, int spellId, int amount)
{
    if (!hero)
        return amount;
    if (P_Spell[spellId].damageSpell)
        return hero->GetSorceryEffect(spellId, amount, target);
    return Detail::NonnegativeInt(std::int64_t(amount) +
                                  hero->GetSpellSpecialtyEffect(spellId, target ? target->info.level : 0, amount));
}

bool ResolveDuration(int spellId, int power, int mastery, Kind kind, H3Hero *hero, H3CombatManager *combat,
                     int hex, H3CombatCreature *target, ErmBridge::Context context, Detail::Duration &duration,
                     bool extendTarget = true)
{
    if (kind != Kind::Duration && kind != Kind::Hypnotize)
        return false;
    // General duration eligibility comes from current flags. These expiry
    // rules are inseparable native mechanics in ApplySpell, not mod options.
    const auto rule = spellId == eSpell::DISRUPTING_RAY ? Detail::DurationRule::DisruptingRay :
                      spellId == eSpell::BERSERK ? Detail::DurationRule::Berserk :
                      spellId == eSpell::FRENZY ? Detail::DurationRule::Frenzy :
                      spellId == eSpell::BLIND ? Detail::DurationRule::Blind :
                      spellId == eSpell::STONE ? Detail::DurationRule::Stone :
                      spellId == eSpell::PARALYZE ? Detail::DurationRule::Paralyze :
                      spellId == eSpell::CLONE ? Detail::DurationRule::Clone : Detail::DurationRule::Generic;
    // No H3API wrapper for the equipment bonus. Call the actual engine entry
    // so patches affecting artifacts are respected, with no artifact ID list.
    const bool needsBonus = (P_Spell[spellId].timeScale || rule == Detail::DurationRule::Clone) &&
                            rule != Detail::DurationRule::Berserk && rule != Detail::DurationRule::Frenzy &&
                            rule != Detail::DurationRule::DisruptingRay;
    // SummonClone takes its timer from the battle hero independently of the
    // caster-kind/SpellPower arguments passed to CastSpell.
    const auto timerHero = rule == Detail::DurationRule::Clone && combat
                               ? combat->hero[combat->currentActiveSide] : hero;
    const int timerPower = rule == Detail::DurationRule::Clone && combat
                              ? combat->heroSpellPower[combat->currentActiveSide] : power;
    const int bonus = timerHero && needsBonus ? THISCALL_1(int, 0x4E5020, timerHero) : 0;
    duration = Detail::NativeDuration(rule, P_Spell[spellId].flags, timerPower, bonus);
    if (rule == Detail::DurationRule::Clone && !timerHero) duration = {};
    // ApplySpell extends an existing timer; a weaker recast never shortens it.
    if (extendTarget && target && rule != Detail::DurationRule::Clone && Detail::NumericDuration(duration.mode) &&
        spellId < 81 && target->activeSpellDuration[spellId] > duration.rounds)
        duration.rounds = target->activeSpellDuration[spellId];
    auto request = QueryArguments(spellId, context, ErmBridge::Duration, kind, hero, combat, hex, power, mastery,
                                  target, duration.rounds);
    request[ErmBridge::Reserved] = int(duration.mode);
    ErmBridge::Arguments result;
    if (QueryErm(request, result))
    {
        if (!result[ErmBridge::Enabled]) return false;
        duration = {static_cast<Detail::DurationMode>(result[ErmBridge::Reserved]), result[ErmBridge::Amount]};
    }
    return duration.mode != Detail::DurationMode::None;
}

bool FormatDuration(const Detail::Duration &duration, char *output, size_t capacity)
{
    Text text;
    switch (duration.mode)
    {
    case Detail::DurationMode::Rounds: text = Text::DurationRounds; break;
    case Detail::DurationMode::Battle: text = Text::DurationBattle; break;
    case Detail::DurationMode::NextAttack: text = Text::DurationNextAttack; break;
    case Detail::DurationMode::NextTurn: text = Text::DurationNextTurn; break;
    case Detail::DurationMode::RoundsOrAttack: text = Text::DurationRoundsOrAttack; break;
    case Detail::DurationMode::RoundsOrDamage: text = Text::DurationRoundsOrDamage; break;
    default: return false;
    }
    return Detail::NumericDuration(duration.mode) ? Format(output, capacity, text, Number(duration.rounds).c_str())
                                                  : Format(output, capacity, text);
}

bool FormatDurationHint(H3CombatManager *combat, H3CombatCreature *target, int hex, int spellId, int power,
                        int mastery, int side, H3Hero *hero, int casterKind, Kind kind,
                        char *output, size_t capacity)
{
    ScopedSpellPreview preview(combat, spellId, hex, side, casterKind);
    const auto &spell = P_Spell[spellId];
    const bool area = spell.targetAnywhere || (spell.expertMassVersion && mastery == 3);
    if (!area && target && (target->type < 0 || target->numberAlive <= 0 ||
                   !CanApplySpell(combat, target, spellId, side, casterKind))) return false;
    if (!target && spell.singleTarget && !area) return false;
    Detail::Duration duration;
    char text[TEXT_CAPACITY] = {};
    if (!ResolveDuration(spellId, power, mastery, kind, hero, combat, hex, target, ErmBridge::Battle, duration, !area) ||
        !FormatDuration(duration, text, sizeof(text)) || !P_Spell[spellId].name) return false;
    if (target && !area)
    {
        const auto name = target->GetCreatureName();
        return name && Format(output, capacity, Text::BattleDurationTarget, P_Spell[spellId].name, name, text);
    }
    return Format(output, capacity, Text::BattleDurationArea, P_Spell[spellId].name, text);
}

std::int64_t FullHealth(const H3CombatCreature *stack)
{
    if (!stack || stack->numberAlive <= 0 || stack->info.hitPoints <= 0)
        return 0;
    if (stack->IsClone())
        return 1; // CombatCreature::GetFullHealth (0x442DA0)
    const auto health = std::int64_t(stack->numberAlive) * stack->info.hitPoints - stack->healthLost;
    return health > 0 ? health : 0;
}

std::int64_t MissingHealth(const H3CombatCreature *stack)
{
    const auto health = std::int64_t(stack->numberAtStart) * stack->info.hitPoints - FullHealth(stack);
    return health > 0 ? health : 0;
}

int SpellChance(H3CombatManager *combat, H3CombatCreature *target, int spellId, int side, int casterKind)
{
    // Read-only part of the engine's resistance check. Never call Random here.
    const double chance = THISCALL_7(double, 0x5A83A0, combat, spellId, side, target, 0, 1, casterKind);
    return Detail::ChancePercent(chance);
}

struct DamagePreview
{
    Detail::DamageTotals totals;
    int amounts[42] = {};
    int deaths[42] = {};
    bool uncertain = false;

    void Add(H3CombatManager *combat, H3CombatCreature *target, int spellId, int base, H3Hero *hero)
    {
        // Do not fire MR again for a duplicate or an already full preview.
        if (totals.count >= 42 || Contains(target))
            return;
        const int amount =
            Detail::NonnegativeInt(ReducedDamage(combat, target, spellId, HeroEffect(hero, target, spellId, base)));
        const int killed = Detail::Killed(target->numberAlive, target->info.hitPoints, target->healthLost, amount,
                                          target->IsClone() != 0);
        const auto index = totals.count;
        if (totals.AddUnique(reinterpret_cast<std::uintptr_t>(target), amount, killed))
        {
            amounts[index] = amount;
            deaths[index] = killed;
        }
    }

    bool Contains(H3CombatCreature *target) const
    {
        return totals.Contains(reinterpret_cast<std::uintptr_t>(target));
    }

    H3CombatCreature *TargetAt(std::size_t index) const
    {
        return reinterpret_cast<H3CombatCreature *>(totals.targets[index]);
    }
};

bool LiveDamageTarget(const H3CombatCreature *target)
{
    return target && target->type >= 0 && target->numberAlive > 0 && target->info.hitPoints > 0;
}

void AddAreaTarget(DamagePreview &preview, H3CombatManager *combat, H3CombatCreature *target, int spellId, int base,
                   int side, H3Hero *hero, int casterKind)
{
    if (!LiveDamageTarget(target) || target->info.cannotMove || preview.Contains(target))
        return;
    const int chance = SpellChance(combat, target, spellId, side, casterKind);
    if (!chance)
        return;
    preview.uncertain |= chance < 100;
    preview.Add(combat, target, spellId, base, hero);
}

void CollectAreaDamage(DamagePreview &preview, H3CombatManager *combat, int hex, int spellId, int base, int side,
                       H3Hero *hero, int casterKind, Detail::DamageShape shape)
{
    if (shape == Detail::DamageShape::Global)
    {
        for (int team = 0; team < 2; ++team)
            for (int i = 0; i < Clamp(0, combat->heroMonCount[team], 21); ++i)
                AddAreaTarget(preview, combat, &combat->stacks[team][i], spellId, base, side, hero, casterKind);
        return;
    }
    // Geometry helper used by 0x5A4C80 -> 0x5A4A00. It only fills our vector;
    // no massSpellTarget, highlights, animation or random state is modified.
    // IDs distinguish geometry only; current flags select the hint category.
    H3Vector<INT32> hexes;
    THISCALL_5(void, 0x5A4480, combat, hex, spellId == eSpell::INFERNO ? 2 : 1, spellId != eSpell::FROST_RING, &hexes);
    for (const auto affectedHex : hexes)
        if (affectedHex >= 0 && affectedHex < BATTLE_HEXES)
            AddAreaTarget(preview, combat, combat->squares[affectedHex].GetMonster(), spellId, base, side, hero,
                          casterKind);
}

bool CollectChainDamage(DamagePreview &preview, H3CombatManager *combat, H3CombatCreature *first, int spellId, int base,
                        int mastery, int side, int casterKind)
{
    if (!LiveDamageTarget(first) || !CanApplySpell(combat, first, spellId, side, casterKind))
        return false;
    preview.uncertain = SpellChance(combat, first, spellId, side, casterKind) < 100;
    // 0x5A6670 reads this mastery table on every cast, including mod changes.
    const int hits = Clamp(0, IntAt(0x642284 + mastery * sizeof(int)), 42);
    int amount = combat->CalculateSpellDamageOnTarget(base, spellId, nullptr, nullptr, nullptr, FALSE);
    // Unlike CastSpell's local hero variable, this helper explicitly reads
    // the battle hero even for a creature cast (call at 0x5A687E).
    auto damageHero = combat->hero[side];
    auto current = first;
    for (int hit = 0; current && hit < hits; ++hit)
    {
        preview.Add(combat, current, spellId, amount, damageHero);
        if (hit + 1 == hits)
            break;
        Detail::ChainCandidate candidates[42] = {};
        H3CombatCreature *stacks[42] = {};
        unsigned char chances[42] = {};
        std::size_t count = 0;
        for (int team = 0; team < 2; ++team)
            for (int i = 0; i < Clamp(0, combat->heroMonCount[team], 21); ++i)
            {
                auto candidate = &combat->stacks[team][i];
                stacks[count] = candidate;
                if (LiveDamageTarget(candidate) && !preview.Contains(candidate))
                {
                    // Verified in 0x5A6500: 1-team is passed independently
                    // of the original caster side, allowing jumps to both armies.
                    const int chance = SpellChance(combat, candidate, spellId, 1 - team, 0);
                    chances[count] = static_cast<unsigned char>(chance);
                    candidates[count] = {candidate->GetX(), candidate->GetY(), chance > 0};
                }
                ++count;
            }
        const int next = Detail::NextChainTarget(current->GetX(), current->GetY(), candidates, count);
        if (next >= 0)
            preview.uncertain |= chances[next] < 100;
        current = next >= 0 ? stacks[next] : nullptr;
        // Halve BEFORE each target's hero bonuses/protection, rather than
        // halving the previous target's final (already reduced) damage.
        amount >>= 1;
    }
    return preview.totals.count > 0;
}

bool FormatDamageTargets(H3CombatManager *combat, H3CombatCreature *target, int hex, int spellId, int power,
                         int mastery, int side, H3Hero *hero, int casterKind, Detail::DamageShape shape, char *output,
                         size_t capacity, Detail::ChainRoute *route)
{
    if (hex < 0 || hex >= BATTLE_HEXES || !P_Spell[spellId].name)
        return false;
    DamagePreview preview;
    const int base = BaseEffect(spellId, power, mastery);
    const bool chain = shape == Detail::DamageShape::Chain;
    if (chain)
    {
        if (!CollectChainDamage(preview, combat, target, spellId, base, mastery, side, casterKind))
            return false;
    }
    else
    {
        // Native local-area helper 0x5A4C80 also obtains its hero directly.
        auto damageHero =
            shape == Detail::DamageShape::Area || spellId == eSpell::ARMAGEDDON ? combat->hero[side] : hero;
        CollectAreaDamage(preview, combat, hex, spellId, base, side, damageHero, casterKind, shape);
    }
    LPCSTR condition = preview.uncertain ? GetText(Text::ResistanceCondition) : "";
    if (!condition)
        return false;
    const auto count = Number(preview.totals.count);
    const auto damage = Number(preview.totals.damage);
    const auto killed = Number(preview.totals.killed);
    if (chain && route)
    {
        // Publish only a successful interactive hint. API requests keep their
        // textual chain and never change the battlefield overlay.
        if (!Format(output, capacity, Text::AreaDamage, P_Spell[spellId].name, count.c_str(), damage.c_str(),
                    killed.c_str(), condition))
            return false;
        for (std::size_t i = 0; i < preview.totals.count; ++i)
        {
            const auto stack = preview.TargetAt(i);
            if (!route->Add(stack->position, stack->GetSecondSquare(), stack->side * 21 + stack->sideIndex))
            {
                route->count = 0;
                return false;
            }
        }
        return true;
    }
    if (!chain)
        return Format(output, capacity, Text::AreaDamage, P_Spell[spellId].name, count.c_str(), damage.c_str(),
                      killed.c_str(), condition);
    Detail::FixedText<API_CAPACITY> details;
    const auto stepFormat = GetText(Text::ChainTarget);
    bool complete = stepFormat != nullptr;
    for (std::size_t i = 0; complete && i < preview.totals.count; ++i)
    {
        char step[TEXT_CAPACITY] = {};
        const auto stack = preview.TargetAt(i);
        const auto name = stack->GetCreatureName();
        complete = name && Format(step, sizeof(step), stepFormat, Number(i + 1).c_str(), name,
                                   Number(stack->position).c_str(), Number(preview.amounts[i]).c_str(),
                                   Number(preview.deaths[i]).c_str()) && details.Append(step, capacity);
    }
    if (complete && Format(output, capacity, Text::ChainDamage, P_Spell[spellId].name, count.c_str(), damage.c_str(),
               killed.c_str(), details.c_str(), condition))
        return true;
    // Long names/translations: retain complete totals instead of a partial chain.
    return Format(output, capacity, Text::AreaDamage, P_Spell[spellId].name, count.c_str(), damage.c_str(),
                  killed.c_str(), condition);
}

bool FormatSpell(H3CombatManager *combat, H3CombatCreature *target, int hex, int spellId, int power, int mastery,
                 int side, H3Hero *hero, int casterKind, char *output, size_t capacity,
                 const Kind *resolvedKind = nullptr, Detail::ChainRoute *route = nullptr)
{
    if (!combat || previewInProgress || queryInProgress || !ValidSpell(spellId) || !ValidSide(side) || mastery < 0 ||
        mastery > 3)
        return false;
    const Kind kind =
        resolvedKind ? *resolvedKind : GetKind(spellId, ErmBridge::Battle, hero, combat, hex, power, mastery, target);
    if (kind == Kind::Duration || kind == Kind::Hypnotize)
        return FormatDurationHint(combat, target, hex, spellId, power, mastery, side, hero, casterKind, kind,
                                  output, capacity);
    if (kind != Kind::Damage && kind != Kind::Recovery && kind != Kind::Cure)
        return false;
    // MR scripts may check BG:A and the acting side while merely hovering.
    ScopedSpellPreview context(combat, spellId, hex, side, casterKind);
    const auto shape = Detail::GetDamageShape(P_Spell[spellId].flags);
    if (kind == Kind::Damage && shape != Detail::DamageShape::Single)
        return FormatDamageTargets(combat, target, hex, spellId, power, mastery, side, hero, casterKind, shape, output,
                                   capacity, route);
    if (!target || target->type < 0 || target->info.hitPoints <= 0)
        return false;
    if (target->numberAlive > 0 && !CanApplySpell(combat, target, spellId, side, casterKind))
        return false;
    if (target->numberAlive <= 0 && kind != Kind::Recovery)
        return false;

    int amount = HeroEffect(hero, target, spellId, BaseEffect(spellId, power, mastery));
    if (kind == Kind::Damage)
        amount = ReducedDamage(combat, target, spellId, amount);
    amount = Detail::NonnegativeInt(amount);
    const auto targetName = target->GetCreatureName();
    const auto spellName = P_Spell[spellId].name;
    if (!targetName || !spellName)
        return false;

    if (kind == Kind::Cure)
    {
        amount = (std::min)(amount, (std::max)(0, target->healthLost));
        return Format(output, capacity, Text::Cure, targetName, Number(amount).c_str());
    }
    const int count = kind == Kind::Damage ? Detail::Killed(target->numberAlive, target->info.hitPoints,
                                                            target->healthLost, amount, target->IsClone() != 0)
                                           : Detail::Resurrected(target->numberAtStart, target->numberAlive,
                                                                 target->info.hitPoints, target->healthLost, amount);
    if (kind == Kind::Recovery)
        amount = Detail::NonnegativeInt((std::min)(std::int64_t(amount), MissingHealth(target)));
    return Format(output, capacity, kind == Kind::Damage ? Text::Damage : Text::Recovery, spellName, targetName,
                  Number(amount).c_str(), Number(count).c_str());
}

bool FormatAbility(H3CombatCreature *caster, H3CombatCreature *target, bool demons, char *output, size_t capacity)
{
    if (!caster || !target || target->type < 0)
        return false;
    const auto name = target->GetCreatureName();
    if (!name)
        return false;
    const int count = THISCALL_2(int, 0x447050, caster, target);
    return Format(output, capacity, demons ? Text::Demons : Text::Archangel, Number(count).c_str(), name);
}

// Never keep a corpse pointer across battles.
H3CombatCreature *sacrificeTarget = nullptr;

_ERH_(ResetSacrificeTarget)
{
    sacrificeTarget = nullptr;
}

bool IsBattleStack(H3CombatManager *combat, H3CombatCreature *stack)
{
    for (auto &side : combat->stacks)
        for (auto &candidate : side)
            if (&candidate == stack)
                return true;
    return false;
}

bool FormatSacrifice(H3CombatManager *combat, H3CombatCreature *target, bool choosingTarget, int power, int mastery,
                     char *output, size_t capacity)
{
    if (choosingTarget)
        sacrificeTarget = nullptr;
    if (!target || target->type < 0 || target->info.hitPoints <= 0 || mastery < 0 || mastery > 3)
        return false;
    const auto targetName = target->GetCreatureName();
    if (!targetName)
        return false;
    if (choosingTarget)
    {
        sacrificeTarget = target;
        return Format(output, capacity, Text::SacrificeTarget, targetName, Number(MissingHealth(target)).c_str(),
                      Number(target->numberAtStart - target->numberAlive).c_str());
    }
    if (!sacrificeTarget || !IsBattleStack(combat, sacrificeTarget) || sacrificeTarget == target ||
        sacrificeTarget->info.hitPoints <= 0 || target->numberAlive <= 0)
        return false;
    // Native 0x5A1CEC: (creature table HP + spell power + mastery base) * count.
    // Current stack wounds/buffs and hero damage bonuses do not enter this formula.
    const auto restored = Detail::SacrificeHealth(P_CreatureInformation[target->type].hitPoints, power,
                                                  P_Spell[eSpell::SACRIFICE].baseValue[mastery], target->numberAlive);
    const int available = (std::max)(0, sacrificeTarget->numberAtStart - sacrificeTarget->numberAlive);
    const int count = Detail::Resurrected(sacrificeTarget->numberAtStart, sacrificeTarget->numberAlive,
                                          sacrificeTarget->info.hitPoints, sacrificeTarget->healthLost,
                                          Detail::NonnegativeInt(restored));
    return Format(output, capacity, Text::SacrificeSource, targetName, Number(restored).c_str(),
                  Number(MissingHealth(sacrificeTarget)).c_str(), Number(count).c_str(), Number(available).c_str());
}

void __stdcall PrepareSpellHint(HiHook *hook, H3CombatManager *combat, int spellId, int hex, BOOL8 choosingTarget)
{
    if (queryInProgress || previewInProgress || !combat || combat->IsHiddenBattle() || !combat->dlg ||
        !ValidSpell(spellId) || !ValidSide(combat->currentActiveSide) || hex < 0 || hex >= BATTLE_HEXES)
    {
        if (!queryInProgress && !previewInProgress)
            Overlay::Clear();
        THISCALL_4(void, hook->GetDefaultFunc(), combat, spellId, hex, choosingTarget);
        return;
    }
    const int side = combat->currentActiveSide;
    const bool creatureCast = combat->action == eCombatAction::MONSTER_SPELL;
    H3Hero *hero = creatureCast ? nullptr : combat->hero[side];
    int power = hero ? combat->heroSpellPower[side] : 1;
    int mastery = hero ? hero->GetSpellExpertise(spellId, combat->specialTerrain) : 0;
    if (creatureCast)
    {
        power = CreaturePower(CurrentCaster(combat));
        mastery = CreatureMastery(combat);
        if (power <= 0)
        {
            Overlay::Clear();
            THISCALL_4(void, hook->GetDefaultFunc(), combat, spellId, hex, choosingTarget);
            return;
        }
    }
    auto target = combat->squares[hex].GetMonster();
    const Kind kind = GetKind(spellId, ErmBridge::Battle, hero, combat, hex, power, mastery, target);
    if (kind == Kind::Recovery)
    {
        // Resurrection and Animate Dead share flags; their target rules differ.
        target =
            spellId == eSpell::ANIMATE_DEAD ? combat->GetAnimateDeadTarget(hex) : combat->GetResurrectionTarget(hex);
    }
    else if (kind == Kind::Sacrifice && choosingTarget)
        target = combat->GetResurrectionTarget(hex);
    if (kind != Kind::Sacrifice)
        sacrificeTarget = nullptr;
    char hint[TEXT_CAPACITY] = {};
    Detail::ChainRoute route;
    const bool formatted = kind == Kind::Sacrifice ? FormatSacrifice(combat, target, choosingTarget != 0, power,
                                                                     mastery, hint, sizeof(hint))
                                                   : FormatSpell(combat, target, hex, spellId, power, mastery, side,
                                                                 hero, creatureCast ? 1 : 0, hint, sizeof(hint), &kind,
                                                                 &route);
    if (!formatted)
    {
        Overlay::Clear();
        // Keep engine/mod targeting prompts and unsupported spell hints intact.
        THISCALL_4(void, hook->GetDefaultFunc(), combat, spellId, hex, choosingTarget);
        return;
    }
    std::memcpy(h3_TextBuffer, hint, std::strlen(hint) + 1);
    THISCALL_4(void, 0x4729D0, combat->dlg, h3_TextBuffer, FALSE, TRUE);
    Overlay::Update(combat, spellId, route);
}

bool FormatBookEffect(int spellId, int power, int mastery, H3Hero *hero, char *output, size_t capacity)
{
    if (queryInProgress || previewInProgress || !ValidSpell(spellId) || mastery < 0 || mastery > 3)
        return false;
    const auto &spell = P_Spell[spellId];
    const Kind kind = GetKind(spellId, ErmBridge::Book, hero, nullptr, -1, power, mastery);
    int amount = kind == Kind::Duration ? 0 : HeroEffect(hero, nullptr, spellId, BaseEffect(spellId, power, mastery));
    Text text = Text::Count;
    switch (kind)
    {
    case Kind::Damage:
        text = spell.aiAreaEffect && spell.singleTarget ? Text::BookFirstDamage : Text::BookDamage;
        break;
    case Kind::FireWall:
        text = Text::BookDamage;
        break;
    case Kind::LandMine:
        text = Text::BookMine;
        break;
    case Kind::Cure:
        text = Text::BookCure;
        break;
    case Kind::Recovery:
        text = Text::BookRecovery;
        break;
    case Kind::Hypnotize:
        text = Text::BookHypnotize;
        break;
    case Kind::Summon:
        text = Text::BookSummon;
        amount = Detail::NonnegativeInt(std::int64_t(spell.baseValue[mastery]) * power);
        amount = HeroEffect(hero, nullptr, spellId, amount);
        break;
    default:
        break;
    }
    Detail::FixedText<TEXT_CAPACITY> composed;
    char segment[TEXT_CAPACITY] = {};
    if (text != Text::Count)
    {
        const auto request = QueryArguments(spellId, ErmBridge::Book, ErmBridge::BookEffect, kind, hero, nullptr, -1,
                                            power, mastery, nullptr, amount);
        ErmBridge::Arguments result;
        if (QueryErm(request, result))
        {
            if (!result[ErmBridge::Enabled]) return false;
            amount = result[ErmBridge::Amount]; // Final book number, no repeated hero bonuses.
        }
        if (!Format(segment, sizeof(segment), text, amount) || !composed.Append(segment, capacity)) return false;
    }
    Detail::Duration duration;
    char durationText[TEXT_CAPACITY] = {};
    if (ResolveDuration(spellId, power, mastery, kind, hero, nullptr, -1, nullptr, ErmBridge::Book, duration) &&
        FormatDuration(duration, durationText, sizeof(durationText)) &&
        Format(segment, sizeof(segment), Text::BookDuration, durationText))
    {
        if (!composed.Append(segment, capacity)) return false;
    }
    if (!composed.c_str()[0]) return false;
    std::memcpy(output, composed.c_str(), std::strlen(composed.c_str()) + 1);
    return true;
}

_LHF_(AppendSpellbookEffect)
{
    // Spellbook::BuildDescription (0x59BDA0), used by the RMB popup at
    // 0x59D3BA and the other full spell popup at 0x59D7B6. Its hover request
    // at 0x59DA42 uses compactDescription=true and must keep the short text.
    if (ByteAt(c->ebp + 0x14))
        return EXEC_DEFAULT;
    auto hero = reinterpret_cast<H3Hero *>(IntAt(c->ebp + 0x10));
    if (!hero)
        return EXEC_DEFAULT;
    const int spellId = IntAt(c->ebp + 0xC);
    const int mastery = IntAt(c->ebp - 0x10); // already calculated using the book's terrain
    const int power = Clamp(1, static_cast<int>(hero->primarySkill[2]), 99);
    char effect[TEXT_CAPACITY] = {};
    if (FormatBookEffect(spellId, power, mastery, hero, effect, sizeof(effect)))
    {
        // Native H3String local, before the engine copies it to its result.
        auto description = reinterpret_cast<H3String *>(c->ebp - 0x24);
        description->Append(effect);
        // Skip the native damage suffix only after a complete replacement.
        // Unsupported types, missing JSON and ERM opt-out keep the original.
        c->return_address = 0x59C084;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(CreatureResurrectionHint)
{
    if (!queryInProgress && !previewInProgress)
        Overlay::Clear();
    auto combat = H3CombatManager::Get();
    if (!combat || combat->IsHiddenBattle() || !ValidSide(c->esi) || c->eax >= BATTLE_HEXES)
        return EXEC_DEFAULT;
    const bool demons = h->GetAddress() == 0x492AF6;
    auto caster = reinterpret_cast<H3CombatCreature *>(c->ebx);
    auto target = demons ? combat->GetSummonDemonTarget(c->esi, c->eax)
                         : THISCALL_4(H3CombatCreature *, 0x5A3FD0, combat, c->esi, c->eax, 1);
    char hint[TEXT_CAPACITY] = {};
    if (!FormatAbility(caster, target, demons, hint, sizeof(hint)))
        return EXEC_DEFAULT;
    std::memcpy(h3_TextBuffer, hint, std::strlen(hint) + 1);
    c->return_address = 0x492E3B;
    return NO_EXEC_DEFAULT;
}

_LHF_(CreatureSpellHint)
{
    auto combat = H3CombatManager::Get();
    auto caster = CurrentCaster(combat);
    auto target = reinterpret_cast<H3CombatCreature *>(c->eax);
    const int power = CreaturePower(caster);
    char hint[TEXT_CAPACITY] = {};
    Detail::ChainRoute route;
    if (!combat || combat->IsHiddenBattle() || !caster || power <= 0 ||
        !FormatSpell(combat, target, combat->mouseCoord, caster->faerieDragonSpell, power, CreatureMastery(combat),
                     combat->currentActiveSide, nullptr, 1, hint, sizeof(hint), nullptr, &route))
    {
        if (!queryInProgress && !previewInProgress)
            Overlay::Clear();
        return EXEC_DEFAULT;
    }
    Overlay::Update(combat, caster->faerieDragonSpell, route);
    std::memcpy(h3_TextBuffer, hint, std::strlen(hint) + 1);
    c->return_address = 0x492E3B;
    return NO_EXEC_DEFAULT;
}

_LHF_(SupremeArchangelResurrection)
{
    // Preserve the WoG upgrade's native resurrection calculation (creature 150).
    c->return_address = c->eax == eCreature::ARCHANGEL || c->eax == 150 ? 0x44705F : 0x447098;
    return NO_EXEC_DEFAULT;
}

int FormatApiRequest(const SpellDescriptionHint::RequestV1 *request)
{
    using namespace SpellDescriptionHint;
    if (!initialized || !request || request->structSize != sizeof(RequestV1) || request->abiVersion != ABI_VERSION ||
        request->flags || request->reserved[0] || request->reserved[1] || !request->output ||
        request->outputCapacity < 2 || request->outputCapacity > API_CAPACITY || !ValidSide(request->casterSide) ||
        request->targetHex < 0 || request->targetHex >= BATTLE_HEXES)
        return FORMAT_INVALID;
    auto combat = H3CombatManager::Get();
    if (!combat || request->battleManager != combat)
        return FORMAT_INVALID;
    auto caster = static_cast<H3CombatCreature *>(request->casterStack);
    auto target = static_cast<H3CombatCreature *>(request->targetStack);
    if (!caster || !IsBattleStack(combat, caster) || (target && (!IsBattleStack(combat, target) || target->type < 0)) ||
        caster != CurrentCaster(combat) || caster->type < 0 || caster->numberAlive <= 0)
        return FORMAT_DECLINED;
    char hint[API_CAPACITY] = {};
    bool formatted = false;
    if (request->hintKind == HINT_SPELL)
    {
        if (!ValidSpell(request->spellId) || request->spellPower <= 0 || request->spellMastery < 0 ||
            request->spellMastery > 3)
            return FORMAT_INVALID;
        // Caller supplies exact creature power/mastery. Native area/chain
        // helpers still use the battle hero, as their actual casts do.
        formatted = FormatSpell(combat, target, request->targetHex, request->spellId, request->spellPower,
                                request->spellMastery, request->casterSide, nullptr, 1, hint, sizeof(hint));
    }
    else if (request->hintKind == HINT_RESURRECT_CREATURE || request->hintKind == HINT_SUMMON_DEMONS)
    {
        if (request->spellId != -1 || request->spellPower || request->spellMastery)
            return FORMAT_INVALID;
        formatted = FormatAbility(caster, target, request->hintKind == HINT_SUMMON_DEMONS, hint, sizeof(hint));
    }
    else
        return FORMAT_INVALID;
    const size_t length = std::strlen(hint);
    if (!formatted || !length || length >= request->outputCapacity)
        return FORMAT_DECLINED;
    // A declined/invalid request never changes the caller's output.
    std::memcpy(request->output, hint, length + 1);
    return FORMAT_SUCCESS;
}

int32_t __stdcall TryFormatBattleHint(const SpellDescriptionHint::RequestV1 *request)
{
    __try
    {
        return FormatApiRequest(request);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return SpellDescriptionHint::FORMAT_INVALID;
    }
}

SpellDescriptionHint::ApiV1 battleHintApi = {SpellDescriptionHint::MAGIC,
                                             sizeof(SpellDescriptionHint::ApiV1),
                                             SpellDescriptionHint::ABI_VERSION,
                                             TryFormatBattleHint,
                                             {0, 0, 0, 0}};
} // namespace

void PublishApi()
{
    if (!globalPatcher)
        return;
    const auto address = static_cast<_dword_>(reinterpret_cast<DWORD_PTR>(&battleHintApi));
    auto variable = globalPatcher->VarFind(SpellDescriptionHint::PATCHER_VARIABLE);
    if (!variable)
        globalPatcher->VarInit(SpellDescriptionHint::PATCHER_VARIABLE, address);
    else
        variable->SetValue(address);
}

void Install(PatcherInstance *pi)
{
    static bool installed = false;
    if (installed || !pi)
        return;
    installed = true;
    initialized = true;
    Overlay::Install(pi);
    Era::RegisterHandler(ResetSacrificeTarget, "OnSetupBattleField");
    Era::RegisterHandler(ResetSacrificeTarget, "OnAfterBattleUniversal");
    Era::RegisterHandler(ResetSacrificeTarget, "OnGameLeave");

    pi->WriteHiHook(0x5A89A0, THISCALL_, PrepareSpellHint);
    pi->WriteLoHook(0x59BFB3, AppendSpellbookEffect);
    pi->WriteLoHook(0x492A6E, CreatureResurrectionHint);
    pi->WriteLoHook(0x492AF6, CreatureResurrectionHint);
    pi->WriteLoHook(0x492B8E, CreatureSpellHint);
    pi->WriteLoHook(0x492B53, CreatureSpellHint); // area cast aimed at an empty hex
    pi->WriteLoHook(0x44705A, SupremeArchangelResurrection);

    PublishApi();
}
} // namespace SpellDescriptions
