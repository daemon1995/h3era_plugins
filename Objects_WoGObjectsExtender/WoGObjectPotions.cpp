#include <algorithm>
#include <array>

#include "WoGObjectIds.h"
#include "WoGObjectPolicy.h"
#include "WoGObjectPotionDialog.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"

namespace wog
{
namespace handlers
{
BOOL VisitPotion(H3Hero *, int, BOOL);
namespace
{
using namespace objects;
struct Drinker
{
    int hero = -1, mana = 0;
    bool hydromel = false, berserker = false, venus = false;
    std::array<int, 4> applied{};
};
Drinker drinkers[2];
bool pending = false;
std::string PotionKey(int side, const char *field)
{
    return "wog_objects.native.potions.battle." + std::to_string(side) + "." + field;
}
void SaveDrinkers()
{
    state::Set("wog_objects.native.potions.battle.pending", pending);
    for (int side = 0; side < 2; ++side)
    {
        const auto &d = drinkers[side];
        state::Set(PotionKey(side, "hero"), d.hero);
        state::Set(PotionKey(side, "mana"), d.mana);
        state::Set(PotionKey(side, "flags"), d.hydromel + 2 * d.berserker + 4 * d.venus);
        for (int stat = 0; stat < 4; ++stat)
            state::Set(PotionKey(side, ("stat" + std::to_string(stat)).c_str()), d.applied[stat]);
    }
}

bool Human(const H3Hero *hero)
{
    return hero && hero->owner >= 0 && hero->owner < 8 && P_Main->IsHuman(hero->owner);
}
state::IntValue Doses(H3Hero *hero, int variable)
{
    return state::IntValue(state::HeroKey(variable == 12   ? VENUS_FOUNTAIN
                                          : variable == 11 ? BERSERKER_FOUNTAIN
                                                           : -1,
                                          hero->id, "doses"));
}
bool Drink(H3Hero *hero, const int variable, const bool accept)
{
    auto doses = Doses(hero, variable);
    if (doses <= 0 || !accept)
        return false;
    --doses;
    return true;
}
void AddPrimary(H3Hero *hero, Drinker &drinker, const int skill, const int amount)
{
    const int before = static_cast<unsigned char>(hero->primarySkill[skill]);
    const int applied = (std::max)(0, (std::min)(amount, 125 - before));
    if (applied && util::GivePrimary(hero, skill, applied))
        drinker.applied[skill] += applied;
}
void __stdcall BeforePotionBattle(Era::TEvent *)
{
    if (pending || policy::LoadingGame())
        return; // Loading a saved battle must not consume another dose.
    // Match script 61: the two network sides must not independently consume doses or ask the defender.
    if (util::ReadReceiver("BA:E?i^wog_objects.native.bridge.result^;") >= 2)
        return;
    pending = true;
    for (int side = 0; side < 2; ++side)
    {
        auto &drinker = drinkers[side];
        drinker = {};
        const int id = util::ReadReceiver(H3String::Format("BA:H%d/?i^wog_objects.native.bridge.result^;", side).String(), -1);
        if (id < 0 || id >= H3HeroCount::Get())
            continue;
        H3Hero *hero = P_Main->GetHero(id);
        if (!hero || hero->owner < 0 || hero->owner > 7)
            continue;
        drinker.hero = id;
        const bool human = Human(hero);
        if (side == 1 && human && drinkers[0].hero >= 0 && Human(P_Main->GetHero(drinkers[0].hero)))
            continue;
        std::array<bool, 3> selected{};
        if (human && (Doses(hero, 10) > 0 || Doses(hero, 11) > 0 || Doses(hero, 12) > 0))
            selected = ChoosePotions(hero, {{int(Doses(hero, 10)), int(Doses(hero, 11)), int(Doses(hero, 12))}});
        drinker.hydromel = Drink(hero, 10, human ? selected[0] : hero->army.HasCreatures());
        if (drinker.hydromel)
        {
            AddPrimary(hero, drinker, 0, 2);
            AddPrimary(hero, drinker, 1, 2);
        }
        const bool physical = hero->spellPoints <= 5 && util::ArmyAiValue(hero->army) > 0;
        drinker.berserker = Drink(hero, 11, human ? selected[1] : physical);
        if (drinker.berserker)
        {
            AddPrimary(hero, drinker, 0, 10);
            drinker.mana = hero->spellPoints;
            hero->spellPoints = 0;
            hero->luckBonus = 3;
        }
        const int luck = hero->luckBonus;
        const int morale = hero->moraleBonus;
        drinker.venus = Drink(hero, 12, human ? selected[2] : luck < 3 || morale < 3);
        if (drinker.venus)
            hero->moraleBonus = hero->luckBonus = 3;
    }
    SaveDrinkers();
}
void RestorePotionBonuses(const bool applyPenalty)
{
    if (!pending || !P_Main)
        return;
    pending = false;
    for (auto &drinker : drinkers)
    {
        if (drinker.hero < 0 || drinker.hero >= H3HeroCount::Get())
            continue;
        H3Hero *hero = P_Main->GetHero(drinker.hero);
        if (!hero)
            continue;
        for (int skill = 0; skill < 4; ++skill)
            if (drinker.applied[skill])
                util::GivePrimary(hero, skill, -drinker.applied[skill]);
        if (drinker.berserker)
            hero->spellPoints = static_cast<INT16>((std::min)(32767, int(hero->spellPoints) + drinker.mana));
        if (applyPenalty && hero->owner >= 0 && drinker.hydromel && drinker.berserker && util::Random(0, 100) > 50 &&
            util::CleanPrimary(hero, 2) > 1)
        {
            util::GivePrimary(hero, 2, -1);
            if (Human(hero))
                H3Messagebox::Show(util::Tr("potions.mixed", {"hero", hero->name}).c_str());
        }
        drinker = {};
    }
    SaveDrinkers();
}
void __stdcall AfterPotionBattle(Era::TEvent *)
{
    RestorePotionBonuses(true);
}
void __stdcall CancelPotionBattle(Era::TEvent *)
{
    RestorePotionBonuses(false);
}
void __stdcall ResetPotionBattle(Era::TEvent *)
{
    pending = false;
    drinkers[0] = {};
    drinkers[1] = {};
}
void __stdcall LoadPotionBattle(Era::TEvent *)
{
    pending = state::Get("wog_objects.native.potions.battle.pending") != 0;
    for (int side = 0; side < 2; ++side)
    {
        auto &d = drinkers[side];
        d = {};
        d.hero = state::Get(PotionKey(side, "hero"), -1);
        d.mana = state::Get(PotionKey(side, "mana"));
        const int flags = state::Get(PotionKey(side, "flags"));
        d.hydromel = (flags & 1) != 0;
        d.berserker = (flags & 2) != 0;
        d.venus = (flags & 4) != 0;
        for (int stat = 0; stat < 4; ++stat)
            d.applied[stat] = state::Get(PotionKey(side, ("stat" + std::to_string(stat)).c_str()));
    }
}
void __stdcall ObserveFountain(H3Hero *hero, H3MapItem *item, H3Position, BOOL human)
{
    if (hero && item && item->objectType == 30 && item->objectSubtype == 0 && policy::Handles(item) && util::OptionEnabled(OPT_POTIONS))
        VisitPotion(hero, -1, human);
}
} // namespace

BOOL VisitPotion(H3Hero *hero, const int subtype, const BOOL human)
{
    if (!hero || hero->owner < 0 || hero->owner > 7)
        return TRUE;
    const int variable = subtype == VENUS_FOUNTAIN ? 12 : subtype == BERSERKER_FOUNTAIN ? 11 : 10;
    const int previous = state::Get(state::HeroKey(subtype, hero->id, "doses"));
    state::Set(state::HeroKey(subtype, hero->id, "doses"), 3);
    if (human && previous < 3)
        H3Messagebox::Show(util::Tr("potions.refill", {"hero", hero->name, "doses", "3", "potion",
                                                       util::Tr(variable == 10   ? "potions.hydromel"
                                                                : variable == 11 ? "potions.berserker"
                                                                                 : "potions.venus")})
                               .c_str());
    return TRUE;
}
int PotionAiWeight(H3Hero *hero, const int subtype)
{
    if (!hero || hero->owner < 0 || hero->owner > 7)
        return 0;
    const int variable = subtype == VENUS_FOUNTAIN ? 12 : subtype == BERSERKER_FOUNTAIN ? 11 : 10;
    if (variable == 11 && hero->spellPoints > 5)
        return 0;
    return (std::max)(0, 3 - state::Get(state::HeroKey(subtype, hero->id, "doses"))) * 200;
}
void RegisterPotionEvents()
{
    extender::RegisterObjectVisitObserver(ObserveFountain);
    Era::RegisterHandler(BeforePotionBattle, "OnBeforeBattle");
    Era::RegisterHandler(AfterPotionBattle, "OnAfterBattleUniversal");
    Era::RegisterHandler(CancelPotionBattle, "OnBattleCancelled");
    Era::RegisterHandler(ResetPotionBattle, "OnBeforeLoadGame");
    Era::RegisterHandler(LoadPotionBattle, "OnAfterLoadGame");
    Era::RegisterHandler(ResetPotionBattle, "OnGameLeave");
}
} // namespace handlers
} // namespace wog
