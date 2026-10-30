#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <vector>

#include "WoGObjectHandlers.h"
#include "WoGObjectChoiceDialogs.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"

namespace wog
{
namespace handlers
{
namespace
{
using namespace objects;

struct DwellingOffer
{
    std::vector<std::pair<int, int>> stacks;
    int gold = 0;
    long long aiValue = 0;
};

uint32_t Mix(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
DwellingOffer BuildDwellingOffer(const H3Hero *hero, const H3MapItem *item)
{
    DwellingOffer result;
    if (!hero)
        return result;
    int town = hero->hero_class / 2;
    town = (std::max)(0, (std::min)(8, town));
    const int heroGate = (std::max)(0, hero->level / 3);
    const int weekGate = (std::max)(0, (state::Day() / 7) / 2);
    const int maxLevel = (std::min)(6, (std::max)(heroGate, weekGate));
    const int remainder = heroGate >= weekGate ? heroGate % 3 : (state::Day() / 7) % 2;
    const H3Point p = state::Position(item);
    int seed = int(Mix(static_cast<uint32_t>(state::MapRandomSeed()) ^
        static_cast<uint32_t>((p.x + 257 * p.y + 65537 * p.z) ^ (hero->id * 7919) ^ (state::Day() * 104729))));
    std::map<int, int> merged;
    for (int i = 0; i < 7; ++i)
    {
        const int level = Era::SplitMix32(&seed, 0, maxLevel);
        bool upgraded = Era::SplitMix32(&seed, 0, 1) != 0;
        if (level == maxLevel && remainder == 0)
            upgraded = false;
        const int creature = util::NativeTownCreature(town, level, upgraded);
        if (!util::ValidCreature(creature))
            continue;
        const int amount = (level - 7) * -2 - (upgraded ? 1 : 0) + 3;
        merged[creature] += (std::max)(1, amount);
    }
    for (const auto &s : merged)
    {
        result.stacks.push_back(s);
        const int64_t cost = int64_t(util::CreatureGoldCost(s.first)) * s.second;
        if (cost > (std::numeric_limits<int>::max)() - result.gold)
            return DwellingOffer{};
        result.gold += int(cost);
        result.aiValue += static_cast<long long>((std::max)(0, P_CreatureInformation[s.first].aiValue)) * s.second;
    }
    return result;
}

BOOL VisitMysteriousDwelling(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (state::HeroEver(MYSTERIOUS_DWELLING, hero->id))
    {
        if (human)
            H3Messagebox::Show(util::Tr("dwelling.visited", {"hero", hero->name}).c_str());
        return TRUE;
    }
    const DwellingOffer offer = BuildDwellingOffer(hero, item);
    if (offer.stacks.empty())
        return TRUE;
    auto &res = P_Main->players[hero->owner].playerResources;
    if (res.gold < offer.gold)
    {
        if (human)
            H3Messagebox::Show(util::Tr("dwelling.noGold", {"gold", std::to_string(offer.gold)}).c_str());
        return TRUE;
    }
    bool accept = true;
    if (human)
    {
        accept = ConfirmDwellingOffer(offer.stacks, offer.gold);
    }
    else
    {
        // Same offer as a human receives. AI buys only efficient reinforcements and keeps a small reserve.
        const long long priceValue = static_cast<long long>(offer.gold) * 2;
        accept = offer.aiValue >= priceValue && res.gold - offer.gold >= 1000;
    }
    if (!accept)
        return TRUE;
    // Generated groups can coincide with one another and with existing hero stacks.
    // Check the merged offer only after the hero accepts it, before spending gold.
    if (res.gold < offer.gold || !util::AddArmy(hero->army, offer.stacks))
    {
        if (human) H3Messagebox::Show(util::Tr("dwelling.cannotBuy", {"gold", std::to_string(offer.gold)}).c_str());
        return TRUE;
    }
    hero->AddResource(eResource::GOLD, -offer.gold);
    state::SetHeroEver(MYSTERIOUS_DWELLING, hero->id);
    return TRUE;
}

BOOL VisitAlmsHouse(H3Hero *hero, const bool human)
{
    if (state::HeroEver(ALMS_HOUSE, hero->id))
    {
        if (human)
            H3Messagebox::Show(util::Tr("alms.visited").c_str());
        return TRUE;
    }
    if (hero->movement <= 0)
    {
        if (human)
            H3Messagebox::Show(util::Tr("alms.tired").c_str());
        return TRUE;
    }
    int benefits = 0;
    auto &r = P_Main->players[hero->owner].playerResources;
    const int gifts[7] = {1, 1, 1, 1, 1, 1, 100};
    for (int i = 0; i < 7; ++i)
        if (r.asArray[i] == 0)
        {
            hero->AddResource(i, gifts[i]);
            ++benefits;
        }
    for (int i = 0; i < 4; ++i)
        if (hero->primarySkill[i] == (i < 2 ? 0 : 1) && util::GivePrimary(hero, i, 1))
            ++benefits;
    if (hero->experience < 200 && util::GiveExperience(hero, 1000))
        ++benefits;
    if (benefits > 0)
    {
        state::SetHeroEver(ALMS_HOUSE, hero->id);
        hero->movement = 0;
        hero->RecalculateMovement();
    }
    if (human)
        H3Messagebox::Show(util::Tr(benefits ? "alms.helped" : "alms.nothing").c_str());
    return TRUE;
}

int MushroomSkill(const int subtype)
{
    switch (subtype)
    {
    case MAGIC_FIRE_MUSHROOM:
        return 14;
    case MAGIC_AIR_MUSHROOM:
        return 15;
    case MAGIC_WATER_MUSHROOM:
        return 16;
    case MAGIC_EARTH_MUSHROOM:
        return 17;
    default:
        return -1;
    }
}
BOOL VisitMushroom(H3Hero *hero, const int subtype, const bool human)
{
    const int skill = MushroomSkill(subtype);
    if (skill < 0 || skill >= 28)
        return TRUE;
    if (hero->secSkill[skill] >= 3)
    {
        if (human)
            H3Messagebox::Show(util::Tr("mushroom.expert").c_str());
        return TRUE;
    }
    const std::string stamp = state::HeroKey(subtype, hero->id, "week");
    if (state::IsStamp(stamp, state::Week()))
    {
        if (human)
            H3Messagebox::Show(util::Tr("mushroom.used").c_str());
        return TRUE;
    }
    const int oldLevel = hero->secSkill[skill];
    if (!util::LearnSkill(hero, skill, 3))
        return TRUE;
    state::Set(state::HeroKey(subtype, hero->id, "oldLevel"), oldLevel);
    state::Set(state::HeroKey(subtype, hero->id, "addedSkill"), 0);
    state::Stamp(stamp, state::Week());
    state::Set(state::HeroKey(subtype, hero->id, "expiresWeek"), state::Week() + 1);
    if (human)
        H3Messagebox::Show(util::Tr("mushroom.granted").c_str(),
                           H3PictureCategories::SecSkill(static_cast<eSecondary>(skill), eSecSkillLevel::EXPERT));
    return TRUE;
}

BOOL VisitArcaneTower(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (state::ObjectPlayerPeriod(item, ARCANE_TOWER, hero->owner, "week", state::Week()))
    {
        if (human)
            H3Messagebox::Show(util::Tr("arcane.visited").c_str());
        return TRUE;
    }
    if (!human)
    {
        util::GiveExperience(hero, 1000);
        state::SetObjectPlayerPeriod(item, ARCANE_TOWER, hero->owner, "week", state::Week(), hero);
        return TRUE;
    }
    const auto trade = ChooseArcaneTrade(hero);
    if (trade.amount <= 0 || trade.paid < 0 || trade.wanted < 0 || trade.paid == trade.wanted)
        return TRUE;
    const int available = util::CleanPrimary(hero, trade.paid) - (trade.paid > 1 ? 1 : 0);
    if (trade.amount > available ||
        int(static_cast<unsigned char>(hero->primarySkill[trade.wanted])) + trade.amount > 125 ||
        !util::GivePrimary(hero, trade.paid, -trade.amount))
        return TRUE;
    if (!util::GivePrimary(hero, trade.wanted, trade.amount))
    {
        util::GivePrimary(hero, trade.paid, trade.amount);
        return TRUE;
    }
    state::SetObjectPlayerPeriod(item, ARCANE_TOWER, hero->owner, "week", state::Week(), hero);
    return TRUE;
}

int DarknessDragonFightValue()
{
    return util::ValidCreature(creature::DARKNESS_DRAGON) ? P_CreatureInformation[creature::DARKNESS_DRAGON].fightValue
                                                          : 1000000;
}
int BloodDragonFightValue()
{
    return util::ValidCreature(creature::BLOOD_DRAGON) ? P_CreatureInformation[creature::BLOOD_DRAGON].fightValue
                                                       : DarknessDragonFightValue();
}
int RandomNeutralForLevel(const int level)
{
    // Exact non-upgraded neutral table from WOG_14_GetNeutralMon. Extended IDs are validated at runtime.
    if (level == 0 && util::ValidCreature(eCreature::HALFLING))
        return eCreature::HALFLING;
    if (level == 1 && util::ValidCreature(eCreature::ROGUE))
        return eCreature::ROGUE;
    if (level == 2 && util::ValidCreature(eCreature::MUMMY))
        return eCreature::MUMMY;
    if (level == 3 && util::ValidCreature(eCreature::SHARPSHOOTER))
        return eCreature::SHARPSHOOTER;
    if (level == 4)
    {
        const int candidates[] = {creature::ARCTIC_SHARPSHOOTER, creature::LAVA_SHARPSHOOTER, eCreature::GOLD_GOLEM};
        std::vector<int> valid;
        for (int id : candidates)
            if (util::ValidCreature(id))
                valid.push_back(id);
        if (!valid.empty())
            return valid[util::Random(0, static_cast<int>(valid.size()) - 1)];
    }
    if (level == 5)
    {
        // Standard WoG: Sorceress=193. If another creature pack omits it, choose among the valid source alternatives.
        const int candidates[] = {eCreature::ENCHANTER, creature::SORCERESS, eCreature::DIAMOND_GOLEM};
        std::vector<int> valid;
        for (int id : candidates)
            if (util::ValidCreature(id))
                valid.push_back(id);
        if (!valid.empty())
            return valid[util::Random(0, static_cast<int>(valid.size()) - 1)];
    }
    if (level == 6 && util::ValidCreature(creature::GORYNYCH))
        return creature::GORYNYCH;
    if (level == 7 && util::ValidCreature(eCreature::FAERIE_DRAGON))
        return eCreature::FAERIE_DRAGON;

    // Compatibility fallback for creature packs that remove a standard WoG neutral.
    int best = -1;
    for (int i = 0; i < H3CreatureCount::Get(); ++i)
        if (P_CreatureInformation[i].town < 0 && P_CreatureInformation[i].level == (std::min)(6, level))
            best = i;
    return best;
}
int TransformTarget(const int source, int town)
{
    if (!util::ValidCreature(source))
        return source;
    int level = P_CreatureInformation[source].level;
    if (level >= 6 && P_CreatureInformation[source].fightValue >= BloodDragonFightValue())
        level = 7;
    if (town >= 0 && town <= 8)
    {
        if (level == 7)
        {
            const int id = creature::SUPREME_ARCHANGEL + town;
            return util::ValidCreature(id) ? id : source;
        }
        const int id = util::NativeTownCreature(town, (std::max)(0, (std::min)(6, level)), false);
        return util::ValidCreature(id) ? id : source;
    }
    return RandomNeutralForLevel(level);
}
BOOL VisitAltar(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (state::ObjectPlayerPeriod(item, ALTAR_OF_TRANSFORMATION, hero->owner, "week", state::Week()))
    {
        if (human)
            H3Messagebox::Show(util::Tr("altar.visited").c_str());
        return TRUE;
    }
    const int ceiling = DarknessDragonFightValue();
    bool any = false;
    for (int i = 0; i < 7; ++i)
    {
        const int mon = hero->army.type[i];
        if (!util::ValidCreature(mon) || hero->army.count[i] <= 0)
            continue;
        if (mon == creature::GHOST || (mon >= creature::FIRST_EMISSARY && mon <= creature::LAST_EMISSARY))
            continue;
        if (P_CreatureInformation[mon].fightValue <= ceiling)
            any = true;
    }
    if (!any)
    {
        if (human)
            H3Messagebox::Show(util::Tr("altar.noMon").c_str());
        return TRUE;
    }
    int chance = 100;
    int town = (std::max)(0, (std::min)(8, hero->hero_class / 2));
    if (human)
    {
        std::array<AltarRow, 7> rows{};
        for (int slot = 0; slot < 7; ++slot)
        {
            auto &row = rows[slot];
            row.source = hero->army.type[slot]; row.count = hero->army.count[slot];
            row.eligible = util::ValidCreature(row.source) && row.count > 0 && row.source != creature::GHOST &&
                (row.source < creature::FIRST_EMISSARY || row.source > creature::LAST_EMISSARY) &&
                P_CreatureInformation[row.source].fightValue <= ceiling;
            if (row.eligible)
                for (int targetTown = 0; targetTown < 9; ++targetTown)
                    row.targets[targetTown] = P_CreatureInformation[row.source].town == targetTown ? row.source :
                        TransformTarget(row.source, targetTown);
        }
        const auto choice = ChooseAltarTransformation(hero, rows);
        if (!choice.accepted || choice.mana < 0 || choice.mana > hero->spellPoints)
            return TRUE;
        chance = choice.mana;
        town = choice.town;
        hero->spellPoints -= static_cast<INT16>(chance);
    }
    for (int i = 0; i < 7; ++i)
    {
        const int mon = hero->army.type[i];
        if (!util::ValidCreature(mon) || hero->army.count[i] <= 0)
            continue;
        if (mon == creature::GHOST || (mon >= creature::FIRST_EMISSARY && mon <= creature::LAST_EMISSARY))
            continue;
        if (P_CreatureInformation[mon].fightValue > ceiling)
            continue;
        // Source skips creature types already belonging to the chosen destination, preserving upgrades/stack
        // experience.
        if (town >= -1 && P_CreatureInformation[mon].town == town)
            continue;
        int selectedTown = (town >= -1 && util::Random(0, 100) <= chance) ? town : util::Random(-1, 8);
        const int target = TransformTarget(mon, selectedTown);
        if (util::ValidCreature(target) &&
            (human || P_CreatureInformation[target].aiValue > P_CreatureInformation[mon].aiValue))
            util::TransformArmyStack(hero, i, target);
    }
    state::SetObjectPlayerPeriod(item, ALTAR_OF_TRANSFORMATION, hero->owner, "week", state::Week(), hero);
    return TRUE;
}

struct AcademyOffer
{
    int skill;
    int artifact;
    int price;
};
AcademyOffer GetAcademyOffer()
{
    const int day = state::Day();
    const std::string stamp = "wog_objects.native.32.offer.day";
    if (!state::IsStamp(stamp, day))
    {
        static const int skills[] = {1, 6, 9, 10, 19, 20, 22, 23};
        const int skill = skills[util::Random(0, 7)];
        static const int arts[] = {7,  8,  9,  10, 11, 18,  19,  20,  21,  22, 31,
                                   38, 39, 40, 60, 91, 146, 147, 148, 152, 154};
        std::vector<int> enabled;
        for (int id : arts)
            if (util::ArtifactEnabled(id))
                enabled.push_back(id);
        const int art = enabled.empty() ? -1 : enabled[util::Random(0, static_cast<int>(enabled.size()) - 1)];
        int price = art >= 146 ? 15000 : (art >= 0 ? H3ArtifactSetup::Get()[art].cost * 3 : 0);
        state::Set("wog_objects.native.32.offer.skill", skill);
        state::Set("wog_objects.native.32.offer.artifact", art);
        state::Set("wog_objects.native.32.offer.price", price);
        state::Stamp(stamp, day);
    }
    return {state::Get("wog_objects.native.32.offer.skill"), state::Get("wog_objects.native.32.offer.artifact", -1),
            state::Get("wog_objects.native.32.offer.price")};
}

int AcademySecondaryCost(const H3Hero *hero, const int skill)
{
    if (!hero || skill < 0 || skill >= 28 || hero->secSkill[skill] > 0)
        return 3000;
    int occupied = 0;
    for (int i = 0; i < 28; ++i)
        if (hero->secSkill[i] > 0)
            ++occupied;
    int maxSlots = 8;
    // Same byte read by the ERM script; ERA patches this value when the secondary-skill limit is extended.
    const int patched = static_cast<int>(ByteAt(0x4A7DD0));
    if (patched > 0 && patched <= 28)
        maxSlots = patched;
    const int overflow = (std::max)(0, occupied - maxSlots + 1);
    return 3000 + overflow * 50000;
}

bool AcademyVisited(const H3Hero *hero)
{
    return state::IsStamp(state::HeroKey(BATTLE_ACADEMY, hero->id, "day"), state::Day());
}
void MarkAcademy(H3Hero *hero)
{
    state::Stamp(state::HeroKey(BATTLE_ACADEMY, hero->id, "day"), state::Day());
}
struct AcademyDecision
{
    int action = -1, primary = -1, score = 0;
};
AcademyDecision AcademyAi(H3Hero *hero, const AcademyOffer &offer)
{
    AcademyDecision decision;
    if (AcademyVisited(hero))
        return decision;
    for (int primary = 0; primary < 2; ++primary)
        if (static_cast<unsigned char>(hero->primarySkill[primary]) < 125 &&
            (decision.primary < 0 || hero->primarySkill[primary] < hero->primarySkill[decision.primary]))
            decision = {0, primary, 1200};
    if (offer.skill >= 0 && offer.skill < 28 && hero->secSkill[offer.skill] < 3)
    {
        const int cost = AcademySecondaryCost(hero, offer.skill);
        const int score = 2000 * (3 - hero->secSkill[offer.skill]) - cost / 3;
        if (cost <= util::Resource(hero->owner, 6) - 1000 && score > decision.score)
            decision = {1, -1, score};
    }
    if (offer.artifact >= 0 && hero->backpackCount < 64 && offer.price <= util::Resource(hero->owner, 6) - 1000)
    {
        const int score = H3ArtifactSetup::Get()[offer.artifact].cost - offer.price / 2;
        if (score > decision.score)
            decision = {2, -1, score};
    }
    return decision;
}
BOOL VisitAcademy(H3Hero *hero, const bool human)
{
    if (AcademyVisited(hero))
    {
        if (human)
            H3Messagebox::Show(util::Tr("academy.visited").c_str());
        return TRUE;
    }
    const AcademyOffer offer = GetAcademyOffer();
    auto &res = P_Main->players[hero->owner].playerResources;
    if (!human)
    {
        const auto decision = AcademyAi(hero, offer);
        const int secCost = AcademySecondaryCost(hero, offer.skill);
        if (decision.action == 1)
        {
            if (!util::LearnSkill(hero, offer.skill, hero->secSkill[offer.skill] + 1))
                return TRUE;
            hero->AddResource(eResource::GOLD, -secCost);
        }
        else if (decision.action == 2)
        {
            hero->AddResource(eResource::GOLD, -offer.price);
            hero->GiveArtifact(H3Artifact(offer.artifact, -1));
        }
        else if (decision.action != 0 || !util::GivePrimary(hero, decision.primary, 1))
            return TRUE;
        MarkAcademy(hero);
        return TRUE;
    }
    struct Service
    {
        int kind, id, price;
    };
    std::vector<Service> services;
    H3PictureVector pictures;
    std::string message = util::Tr("academy.menu") + "\n\n";
    const auto add = [&](Service service, const std::string &name, H3PictureCategories picture) {
        services.push_back(service);
        pictures.Add(picture);
        message += name + ": " + std::to_string(service.price) + " " + P_ResourceName[eResource::GOLD] + "\n";
    };
    for (int primary = 0; primary < 2; ++primary)
        if (static_cast<unsigned char>(hero->primarySkill[primary]) < 125)
        {
            const int cost = (std::max)(0, util::CleanPrimary(hero, primary)) * 500;
            if (res.gold >= cost)
                add({0, primary, cost}, P_PrimarySkillName[primary],
                    H3PictureCategories::PrimarySkill(static_cast<ePrimary>(primary), 1));
        }
    if (offer.skill >= 0 && offer.skill < 28 && hero->secSkill[offer.skill] < 3)
    {
        const int cost = AcademySecondaryCost(hero, offer.skill);
        if (res.gold >= cost)
            add({1, offer.skill, cost}, P_SecondarySkillInfo[offer.skill].name,
                H3PictureCategories::SecSkill(static_cast<eSecondary>(offer.skill),
                                              static_cast<eSecSkillLevel>(hero->secSkill[offer.skill] + 1)));
    }
    if (offer.artifact >= 0 && hero->backpackCount < 64 && res.gold >= offer.price)
        add({2, offer.artifact, offer.price}, H3Artifact(offer.artifact, -1).GetName(),
            H3PictureCategories::Artifact(static_cast<eArtifact>(offer.artifact)));
    if (services.empty())
    {
        H3Messagebox::Show(util::Tr("common.unavailable").c_str());
        return TRUE;
    }
    const int owner = hero->owner, day = state::Day();
    const int index = util::ChoosePictures(message.c_str(), pictures);
    if (index < 0 || index >= int(services.size()) || hero->owner != owner || state::Day() != day ||
        AcademyVisited(hero))
        return TRUE;
    const auto service = services[index];
    if (res.gold < service.price)
        return TRUE;
    if (service.kind == 0)
    {
        if (service.price != (std::max)(0, util::CleanPrimary(hero, service.id)) * 500 ||
            !util::GivePrimary(hero, service.id, 1))
            return TRUE;
    }
    else if (service.kind == 1)
    {
        if (service.price != AcademySecondaryCost(hero, service.id) || hero->secSkill[service.id] >= 3 ||
            !util::LearnSkill(hero, service.id, hero->secSkill[service.id] + 1))
            return TRUE;
    }
    else
    {
        if (hero->backpackCount >= 64 || !util::ArtifactEnabled(service.id))
            return TRUE;
        hero->GiveArtifact(H3Artifact(service.id, -1));
    }
    hero->AddResource(eResource::GOLD, -service.price);
    MarkAcademy(hero);
    return TRUE;
}

void RestoreExpiredMushrooms(bool force = false)
{
    if (!P_Main)
        return;
    const int subs[] = {MAGIC_AIR_MUSHROOM, MAGIC_WATER_MUSHROOM, MAGIC_EARTH_MUSHROOM, MAGIC_FIRE_MUSHROOM};
    for (int id = 0; id < H3HeroCount::Get(); ++id)
    {
        H3Hero *hero = P_Main->GetHero(id);
        if (!hero || hero->owner < 0)
            continue;
        for (int subtype : subs)
        {
            const int expires = state::Get(state::HeroKey(subtype, id, "expiresWeek"), 0);
            if (!expires || (!force && state::Week() < expires))
                continue;
            const int skill = MushroomSkill(subtype);
            const int oldLevel = state::Get(state::HeroKey(subtype, id, "oldLevel"), 0);
            util::LearnSkill(hero, skill, oldLevel);
            state::Set(state::HeroKey(subtype, id, "addedSkill"), 0);
            state::Set(state::HeroKey(subtype, id, "expiresWeek"), 0);
        }
    }
}

_ERH_(OnEveryDayBasic)
{
    RestoreExpiredMushrooms();
}
void __stdcall WinBasic(Era::TEvent *)
{
    if (Era::IsCampaign())
        RestoreExpiredMushrooms(true);
}

} // namespace

// Exposed to dispatcher in WoGObjectHandlers.cpp.
BOOL VisitBasic(H3Hero *hero, H3MapItem *item, const int subtype, const BOOL human)
{
    if (!hero || !item || hero->owner < 0 || hero->owner >= 8)
        return TRUE;
    switch (subtype)
    {
    case MYSTERIOUS_DWELLING:
        return VisitMysteriousDwelling(hero, item, human != FALSE);
    case ARCANE_TOWER:
        return VisitArcaneTower(hero, item, human != FALSE);
    case MAGIC_AIR_MUSHROOM:
    case MAGIC_WATER_MUSHROOM:
    case MAGIC_EARTH_MUSHROOM:
    case MAGIC_FIRE_MUSHROOM:
        return VisitMushroom(hero, subtype, human != FALSE);
    case ALTAR_OF_TRANSFORMATION:
        return VisitAltar(hero, item, human != FALSE);
    case ALMS_HOUSE:
        return VisitAlmsHouse(hero, human != FALSE);
    case BATTLE_ACADEMY:
        return VisitAcademy(hero, human != FALSE);
    default:
        return FALSE;
    }
}

void RegisterBasicEvents()
{
    Era::RegisterHandler(OnEveryDayBasic, "OnEveryDay");
    Era::RegisterHandler(WinBasic, "OnWinGame");
}

int EvaluateBasicAi(H3MapItem *item, H3Hero *hero)
{
    if (!item || !hero || hero->owner < 0 || hero->owner > 7)
        return 0;
    const int subtype = item->objectSubtype;
    if (subtype == MYSTERIOUS_DWELLING)
    {
        if (state::HeroEver(subtype, hero->id))
            return 0;
        const auto offer = BuildDwellingOffer(hero, item);
        if (offer.stacks.empty() || !util::CanAddArmy(hero->army, offer.stacks) ||
            int64_t(util::Resource(hero->owner, 6)) < int64_t(offer.gold) + 1000 ||
            offer.aiValue < int64_t(offer.gold) * 2)
            return 0;
        return int((std::min)(int64_t(20000), offer.aiValue / 4));
    }
    if (subtype == ALMS_HOUSE)
    {
        if (state::HeroEver(subtype, hero->id) || hero->movement <= 0)
            return 0;
        int gain = 0;
        for (int resource = 0; resource < 7; ++resource)
            if (util::Resource(hero->owner, resource) == 0)
                gain += 100;
        for (int primary = 0; primary < 4; ++primary)
            if (hero->primarySkill[primary] == (primary < 2 ? 0 : 1))
                gain += 700;
        if (hero->experience < 200)
            gain += 1000;
        return gain;
    }
    if (subtype == ARCANE_TOWER)
        return state::ObjectPlayerPeriod(item, subtype, hero->owner, "week", state::Week()) ||
                       hero->experience > (std::numeric_limits<int>::max)() - 1000
                   ? 0
                   : 1000;
    if (subtype == BATTLE_ACADEMY)
        return AcademyAi(hero, GetAcademyOffer()).score;
    if (subtype == ALTAR_OF_TRANSFORMATION)
    {
        if (state::ObjectPlayerPeriod(item, subtype, hero->owner, "week", state::Week()))
            return 0;
        const int town = (std::max)(0, (std::min)(8, hero->hero_class / 2));
        int64_t gain = 0;
        for (int slot = 0; slot < 7; ++slot)
        {
            const int mon = hero->army.type[slot];
            if (!util::ValidCreature(mon) || hero->army.count[slot] <= 0 || mon == creature::GHOST ||
                (mon >= creature::FIRST_EMISSARY && mon <= creature::LAST_EMISSARY) ||
                P_CreatureInformation[mon].town == town ||
                P_CreatureInformation[mon].fightValue > DarknessDragonFightValue())
                continue;
            const int target = TransformTarget(mon, town);
            if (util::ValidCreature(target))
                gain +=
                    int64_t((std::max)(0, P_CreatureInformation[target].aiValue - P_CreatureInformation[mon].aiValue)) *
                    hero->army.count[slot];
        }
        return int((std::min)(int64_t(10000), gain));
    }
    const int skill = MushroomSkill(subtype);
    if (skill >= 0 && hero->secSkill[skill] < 3 &&
        !state::IsStamp(state::HeroKey(subtype, hero->id, "week"), state::Week()))
        return (3 - hero->secSkill[skill]) * (hero->spellPoints > 0 ? 300 : 100);
    return 0;
}

} // namespace handlers
} // namespace wog
