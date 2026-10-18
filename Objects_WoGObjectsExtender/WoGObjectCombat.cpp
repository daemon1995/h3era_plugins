#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include "WoGObjectsExtender.h"
#include <algorithm>
#include <array>
#include <vector>

namespace wog
{
namespace handlers
{
namespace
{
using namespace objects;

constexpr int ART_RECHANTERS_CLOAK = 83;
constexpr int ART_ORB_OF_VULNERABILITY = 93;
constexpr int ART_ORB_OF_INHIBITION = 126;
constexpr int ART_POWER_OF_DRAGON_FATHER = 134;
constexpr int TERRAIN_CURSED_GROUND = 21;
constexpr int RES_MITHRIL = 7;

constexpr int SPELL_DEATH_RIPPLE = 24;
constexpr int SPELL_FIRE_SHIELD = 29;
constexpr int SPELL_COUNTERSTRIKE = 58;
constexpr int SPELL_AGE = 75;
constexpr std::array<int, 20> SHRINE_SPELLS = {
    {27, 28, 29, 30, 31, 32, 33, 34, 41, 43, 44, 46, 48, 49, 51, 53, 55, 56, 58, 36}};

bool deathBattle = false;
int deathDifficulty = 0;
int deathHeroId = -1;
int deathRound = -1;

int PendingShrineSpell(const H3Hero *hero)
{
    if (!hero)
        return -1;
    return state::Get(state::HeroKey(FORGOTTEN_SHRINE, hero->id, "pendingSpell"), 0) - 1;
}
void SetPendingShrineSpell(const H3Hero *hero, const int spell)
{
    if (hero)
        state::Set(state::HeroKey(FORGOTTEN_SHRINE, hero->id, "pendingSpell"), spell + 1);
}

bool HeroWears(const H3Hero *hero, const int artifact)
{
    if (!hero)
        return false;
    for (int i = 0; i < 19; ++i)
        if (hero->bodyArtifacts[i].id == artifact)
            return true;
    return false;
}

bool ShrineSpellBlocked(const H3Hero *hero, const int spell)
{
    if (!hero || spell < 0)
        return true;
    if (HeroWears(hero, ART_ORB_OF_INHIBITION))
        return true;
    // Power of the Dragon Father blocks all spells except Magic Mirror in the original script.
    if (HeroWears(hero, ART_POWER_OF_DRAGON_FATHER) && spell != 36)
        return true;
    // Recanter's Cloak does not block the explicitly whitelisted low-level buffs used by the script.
    if (HeroWears(hero, ART_RECHANTERS_CLOAK))
    {
        switch (spell)
        {
        case 29:
        case 48:
        case 55:
        case 56:
        case 58:
        case 36:
            break;
        default:
            return true;
        }
    }
    return false;
}

void ApplyShrineToSide(const int side)
{
    if (!P_CombatManager || side < 0 || side > 1)
        return;
    H3Hero *hero = P_CombatManager->hero[side];
    const int spell = PendingShrineSpell(hero);
    if (spell < 0 || ShrineSpellBlocked(hero, spell))
        return;

    // Native ApplySpell performs the creature's real spell-immunity checks. Orb of Vulnerability is handled
    // by the engine's combat setup; this avoids the hard-coded creature table from the old ERM script.
    for (int i = 0; i < 21; ++i)
    {
        H3CombatCreature &stack = P_CombatManager->stacks[side][i];
        if (stack.numberAlive > 0 && stack.type >= 0)
            stack.ApplySpell(spell, 50, eSecSkillLevel::EXPERT, hero);
    }
}

void ClearShrineBlessings()
{
    if (!P_CombatManager)
        return;
    for (int side = 0; side < 2; ++side)
        if (P_CombatManager->hero[side])
            SetPendingShrineSpell(P_CombatManager->hero[side], -1);
}

BOOL VisitForgottenShrine(H3Hero *hero, const bool human)
{
    if (!hero)
        return TRUE;
    if (PendingShrineSpell(hero) >= 0)
    {
        if (human)
            H3Messagebox::Show(util::Tr("shrine.alreadyBlessed").c_str());
        return TRUE;
    }
    if (hero->movement <= 200)
    {
        if (human)
            H3Messagebox::Show(util::Tr("shrine.noMovement").c_str());
        return TRUE;
    }
    if (human && !H3Messagebox::Choice(util::Tr("shrine.ask").c_str()))
        return TRUE;

    hero->movement = 0;
    const int spell = SHRINE_SPELLS[util::Random(0, static_cast<int>(SHRINE_SPELLS.size()) - 1)];
    SetPendingShrineSpell(hero, spell);
    if (human)
    {
        const char *name = P_Spell[spell].name ? P_Spell[spell].name : "";
        H3Messagebox::Show(util::Tr("shrine.granted", {"spell", name}).c_str(),
                           H3PictureCategories::Spell(static_cast<eSpell>(spell)));
    }
    return TRUE;
}

int DeathDifficulty(H3MapItem *item)
{
    if (!item)
        return 1;
    const std::string key = state::ObjectKey(item, DEATH_CHAMBER, "difficulty", -1);
    int value = state::Get(key, 0);
    if (value < 1 || value > 4)
    {
        value = util::Random(1, 4);
        state::Set(key, value);
    }
    return value;
}

H3Army MakeDeathArmy(const int difficulty)
{
    H3Army army;
    army.Clear();
    // The original map battle starts with the King only and summons the support stacks at fixed hexes.
    // Keeping the army this way lets OnSetupBattlefield create the exact support layout.
    army.AddStack(eCreature::POWER_LICH, 1, 0);
    return army;
}

void SetupDeathBattlefield()
{
    if (!deathBattle || !P_CombatManager)
        return;
    deathRound = -1;
    H3CombatCreature &king = P_CombatManager->stacks[1][0];
    if (king.numberAlive <= 0)
        return;

    const int statBonus = deathDifficulty * 2;
    king.baseHP = deathDifficulty * 200 + 400;
    king.info.hitPoints = king.baseHP;
    king.info.speed = 50;
    king.retaliations = 5000;
    king.info.doubleAttack = 1;
    king.info.attack += statBonus;
    king.info.defence += statBonus;
    king.info.damageLow += statBonus;
    king.info.damageHigh += statBonus;
    if (statBonus > 4)
    {
        king.info.flyer = 1;
        king.info.noMeleePenalty = 1;
    }
    if (statBonus > 6)
    {
        king.info.noRetaliation = 1;
        king.info.attackAllAround = 1;
    }
    if (statBonus > 10)
        king.info.fireImmunity = 1;

    const int dreads = deathDifficulty + 1;
    const int wraiths = dreads * 2 - 1;
    const int liches = (std::max)(0, dreads - 1) * 10;
    const int dreadHexes[] = {41, 47, 143, 149};
    const int wraithHexes[] = {6, 8, 10, 176, 178, 180};
    for (int hex : dreadHexes)
        P_CombatManager->SummonCreature(1, eCreature::DREAD_KNIGHT, dreads, hex, FALSE, FALSE);
    for (int hex : wraithHexes)
        P_CombatManager->SummonCreature(1, eCreature::WRAITH, wraiths, hex, FALSE, FALSE);
    if (liches > 0)
        P_CombatManager->SummonCreature(1, eCreature::LICH, liches, 94, FALSE, FALSE);
}

void DeathRoundStarted()
{
    if (!deathBattle || !P_CombatManager)
        return;
    ++deathRound;
    if (deathRound == 1)
    {
        const int amount = util::Random(1, 10) * deathDifficulty;
        const int hexes[] = {60, 128, 90, 98};
        for (int hex : hexes)
            P_CombatManager->SummonCreature(1, eCreature::LICH, amount, hex, FALSE, TRUE);
    }
    if (deathRound >= 2)
    {
        for (int i = 0; i < 21; ++i)
        {
            H3CombatCreature &stack = P_CombatManager->stacks[0][i];
            if (stack.numberAlive > 0)
                stack.ApplySpell(SPELL_AGE, 5, eSecSkillLevel::EXPERT, nullptr);
        }
    }
    if (deathRound == 2 && util::ValidCreature(creature::GHOST))
    {
        H3Hero *hero = P_CombatManager->hero[0];
        const int level = hero ? (std::max)(1, static_cast<int>(hero->level)) : 1;
        const int amount = deathDifficulty * 6 * level + (std::max)(0, P_CombatManager->turn);
        const int hexes[] = {4, 12, 174, 182};
        for (int hex : hexes)
            P_CombatManager->SummonCreature(1, creature::GHOST, amount, hex, FALSE, TRUE);
    }
}

void DeathBeforeAction()
{
    if (!deathBattle || deathRound < 0 || !P_CombatManager || !P_CombatManager->activeStack)
        return;
    H3CombatCreature *active = P_CombatManager->activeStack;
    H3CombatCreature *king = &P_CombatManager->stacks[1][0];
    if (active == king)
    {
        const int limiter = 3;
        const int damageBonus = (std::max)(0, deathDifficulty - limiter);
        const int casts = (std::max)(1, (std::min)(limiter, deathDifficulty));
        for (int i = 0; i < casts; ++i)
        {
            const int power = i % 2 + deathDifficulty + damageBonus;
            const int school = (std::min)(3, (std::max)(1, power));
            P_CombatManager->CastSpell(SPELL_DEATH_RIPPLE, -1, 2, -1, school, power);
        }
        return;
    }

    if (active->side == 1)
    {
        const int addPower = (std::max)(1, (std::min)(20, deathDifficulty));
        if (active->activeSpellDuration[SPELL_FIRE_SHIELD] == 0)
            active->ApplySpell(SPELL_FIRE_SHIELD, addPower, eSecSkillLevel::BASIC, nullptr);
        if (active->activeSpellDuration[SPELL_COUNTERSTRIKE] == 0)
            active->ApplySpell(SPELL_COUNTERSTRIKE, addPower, eSecSkillLevel::BASIC, nullptr);
    }
    else if (active->side == 0)
    {
        static const int curse[4] = {52, 47, 45, 50}; // Misfortune, Disrupting Ray, Weakness, Sorrow.
        const int spell = curse[(std::max)(1, (std::min)(4, deathDifficulty)) - 1];
        active->ApplySpell(spell, (std::max)(1, deathDifficulty), eSecSkillLevel::BASIC, nullptr);
    }
}

BOOL VisitDeathChamber(H3Hero *hero, H3MapItem *item, const H3Position pos, const bool human)
{
    if (!hero || !item)
        return TRUE;
    if (state::Get(state::ObjectKey(item, DEATH_CHAMBER, "visited", hero->owner), 0))
    {
        if (human)
            H3Messagebox::Show(util::Tr("death.visited").c_str());
        return TRUE;
    }

    const int difficulty = DeathDifficulty(item);
    const long long estimatedEnemy = 8000LL * difficulty * difficulty + 12000LL;
    if (human)
    {
        if (!H3Messagebox::Choice(util::Tr("death.ask", {"difficulty", std::to_string(difficulty)}).c_str(),
                                  H3PictureCategories::Creature(eCreature::POWER_LICH, 1)))
            return TRUE;
    }
    else if (util::ArmyAiValue(hero->army) < estimatedEnemy * 5 / 4)
        return TRUE;

    H3Army defenders = MakeDeathArmy(difficulty);
    deathBattle = true;
    deathDifficulty = difficulty;
    deathHeroId = hero->id;
    deathRound = -1;

    const int seed = H3Random::MultiplayerRandom(1, 0x7FFFFFFF);
    constexpr H3Town *town = nullptr;
    constexpr H3Hero *defHero = nullptr;
    const int result = THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), static_cast<UINT>(pos), hero, &hero->army,
                                   hero->owner, town, defHero, &defenders, seed, TRUE, TRUE);
    (void)result;

    const bool won = hero->owner >= 0 && hero->army.HasCreatures();
    deathBattle = false;
    deathDifficulty = 0;
    deathHeroId = -1;
    deathRound = -1;
    if (!won)
        return TRUE;

    const int ghosts = (difficulty + 1) * 10;
    const int gold = difficulty * 3000 + 6000;
    hero->AddResource(eResource::GOLD, gold);
    if (WoGObjectsExtender::IsWoGOptionEnabled(36))
        hero->AddResource(RES_MITHRIL, difficulty * 2 + 2);
    if (util::ValidCreature(creature::GHOST))
        util::AddArmy(hero->army, {{creature::GHOST, ghosts}});
    state::Set(state::ObjectKey(item, DEATH_CHAMBER, "visited", hero->owner), 1);
    if (human)
        H3Messagebox::Show(
            util::Tr("death.reward", {"gold", std::to_string(gold), "ghosts", std::to_string(ghosts)}).c_str());
    return TRUE;
}

BOOL VisitMirror(H3Hero *, const bool human)
{
    if (!human)
        return TRUE; // Deliberately preserve the ERM fix: AI Town-Portal use can loop on this object.

    const BYTE oldMoveCheck = ByteAt(0x41D55E);
    const WORD oldCosts = WordAt(0x41D9EC);
    ByteAt(0x41D55E) = 235;
    WordAt(0x41D9EC) = 15851;
    H3AdventureManager *adv = *reinterpret_cast<H3AdventureManager **>(0x6992B8);
    if (!adv)
        adv = P_AdventureManager->Get();
    THISCALL_2(void, 0x41D4E0, adv, 3);
    ByteAt(0x41D55E) = oldMoveCheck;
    WordAt(0x41D9EC) = oldCosts;
    return TRUE;
}

void __stdcall OnSetupBattlefieldNative(Era::TEvent *)
{
    ApplyShrineToSide(0);
    ApplyShrineToSide(1);
    SetupDeathBattlefield();
}
void __stdcall OnAfterBattleNative(Era::TEvent *)
{
    ClearShrineBlessings();
    deathBattle = false;
    deathDifficulty = 0;
    deathHeroId = -1;
    deathRound = -1;
}
void __stdcall OnBattleRoundNative(Era::TEvent *)
{
    DeathRoundStarted();
}
void __stdcall OnBeforeBattleActionNative(Era::TEvent *)
{
    DeathBeforeAction();
}
void __stdcall OnAfterLoadCombatNative(Era::TEvent *)
{
    deathBattle = false;
    deathDifficulty = 0;
    deathHeroId = -1;
    deathRound = -1;
}

} // namespace

BOOL VisitCombatObject(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept
{
    if (!hero || !item)
        return TRUE;
    switch (item->objectSubtype)
    {
    case FORGOTTEN_SHRINE:
        return VisitForgottenShrine(hero, human != FALSE);
    case DEATH_CHAMBER:
        return VisitDeathChamber(hero, item, pos, human != FALSE);
    case MIRROR_OF_HOME_WAY:
        return VisitMirror(hero, human != FALSE);
    default:
        return FALSE;
    }
}

int CombatObjectAiWeight(H3MapItem *item, H3Hero *hero) noexcept
{
    if (!item || !hero)
        return 0;
    switch (item->objectSubtype)
    {
    case MIRROR_OF_HOME_WAY:
        return 0;
    case FORGOTTEN_SHRINE:
        return PendingShrineSpell(hero) >= 0 || hero->movement <= 200 ? 0 : 750;
    case DEATH_CHAMBER:
        if (state::Get(state::ObjectKey(item, DEATH_CHAMBER, "visited", hero->owner), 0))
            return 0;
        return util::ArmyAiValue(hero->army) >=
                       (8000LL * DeathDifficulty(item) * DeathDifficulty(item) + 12000LL) * 5 / 4
                   ? 3000
                   : 0;
    default:
        return 0;
    }
}

void RegisterCombatEvents() noexcept
{
    Era::RegisterHandler(OnSetupBattlefieldNative, "OnSetupBattlefield");
    Era::RegisterHandler(OnAfterBattleNative, "OnAfterBattleUniversal");
    Era::RegisterHandler(OnBattleRoundNative, "OnBattleRound");
    Era::RegisterHandler(OnBeforeBattleActionNative, "OnBeforeBattleAction");
    Era::RegisterHandler(OnAfterLoadCombatNative, "OnAfterLoadGame");
}

} // namespace handlers
} // namespace wog
