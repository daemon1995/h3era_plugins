#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <vector>

namespace wog { namespace handlers {
namespace {
using namespace objects;

struct DwellingOffer
{
    std::vector<std::pair<int, int>> stacks;
    int gold = 0;
    long long aiValue = 0;
};

uint32_t Mix(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x;
}
int StableRandom(uint32_t &seed, const int minValue, const int maxValue)
{
    seed = Mix(seed + 0x9E3779B9U);
    const unsigned range = static_cast<unsigned>(maxValue - minValue + 1);
    return minValue + static_cast<int>(seed % range);
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
    uint32_t seed = Mix(static_cast<uint32_t>((p.x + 257 * p.y + 65537 * p.z) ^ (hero->id * 7919) ^
                                              (state::Day() * 104729)));
    std::map<int, int> merged;
    for (int i = 0; i < 7; ++i)
    {
        const int level = StableRandom(seed, 0, maxLevel);
        bool upgraded = StableRandom(seed, 0, 1) != 0;
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
        result.gold += util::CreatureGoldCost(s.first) * s.second;
        result.aiValue += static_cast<long long>((std::max)(0, P_CreatureInformation[s.first].aiValue)) * s.second;
    }
    return result;
}

BOOL VisitMysteriousDwelling(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (state::HeroEver(MYSTERIOUS_DWELLING, hero->id))
    {
        if (human) H3Messagebox::Show(util::Tr("dwelling.visited", {"hero", hero->name}).c_str());
        return TRUE;
    }
    const DwellingOffer offer = BuildDwellingOffer(hero, item);
    if (offer.stacks.empty())
        return TRUE;
    auto &res = P_Main->players[hero->owner].playerResources;
    if (res.gold < offer.gold || !util::CanAddArmy(hero->army, offer.stacks))
    {
        if (human) H3Messagebox::Show(util::Tr("dwelling.cannotBuy", {"gold", std::to_string(offer.gold)}).c_str());
        return TRUE;
    }
    bool accept = true;
    if (human)
    {
        H3PictureVector pics;
        for (const auto &s : offer.stacks)
            pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(s.first), static_cast<UINT16>((std::min)(65535, s.second))));
        const std::string text = util::Tr("dwelling.offer", {"gold", std::to_string(offer.gold)});
        H3Messagebox::Display(text.c_str(), pics);
        accept = H3Messagebox::Choice(util::Tr("dwelling.buy", {"gold", std::to_string(offer.gold)}).c_str());
    }
    else
    {
        // Same offer as a human receives. AI buys only efficient reinforcements and keeps a small reserve.
        const long long priceValue = static_cast<long long>(offer.gold) * 2;
        accept = offer.aiValue >= priceValue && res.gold - offer.gold >= 1000;
    }
    if (!accept)
        return TRUE;
    if (!util::AddArmy(hero->army, offer.stacks))
        return TRUE;
    hero->AddResource(eResource::GOLD, -offer.gold);
    state::SetHeroEver(MYSTERIOUS_DWELLING, hero->id);
    return TRUE;
}

BOOL VisitAlmsHouse(H3Hero *hero, const bool human)
{
    if (state::HeroEver(ALMS_HOUSE, hero->id))
    {
        if (human) H3Messagebox::Show(util::Tr("alms.visited").c_str());
        return TRUE;
    }
    if (hero->movement <= 0)
    {
        if (human) H3Messagebox::Show(util::Tr("alms.tired").c_str());
        return TRUE;
    }
    int benefits = 0;
    auto &r = P_Main->players[hero->owner].playerResources;
    const int gifts[7] = {1, 1, 1, 1, 1, 1, 100};
    for (int i = 0; i < 7; ++i)
        if (r.asArray[i] == 0) { hero->AddResource(i, gifts[i]); ++benefits; }
    if (hero->primarySkill[0] == 0) { ++hero->primarySkill[0]; ++benefits; }
    if (hero->primarySkill[1] == 0) { ++hero->primarySkill[1]; ++benefits; }
    if (hero->primarySkill[2] == 1) { ++hero->primarySkill[2]; ++benefits; }
    if (hero->primarySkill[3] == 1) { ++hero->primarySkill[3]; ++benefits; }
    if (hero->experience < 200) { hero->experience += 1000; ++benefits; }
    state::SetHeroEver(ALMS_HOUSE, hero->id);
    if (benefits > 0)
    {
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
    case MAGIC_FIRE_MUSHROOM: return 14;
    case MAGIC_AIR_MUSHROOM: return 15;
    case MAGIC_WATER_MUSHROOM: return 16;
    case MAGIC_EARTH_MUSHROOM: return 17;
    default: return -1;
    }
}
BOOL VisitMushroom(H3Hero *hero, const int subtype, const bool human)
{
    const int skill = MushroomSkill(subtype);
    if (skill < 0 || skill >= 28)
        return TRUE;
    if (hero->secSkill[skill] >= 3)
    {
        if (human) H3Messagebox::Show(util::Tr("mushroom.expert").c_str());
        return TRUE;
    }
    const std::string stamp = state::HeroKey(subtype, hero->id, "week");
    if (state::IsStamp(stamp, state::Week()))
    {
        if (human) H3Messagebox::Show(util::Tr("mushroom.used").c_str());
        return TRUE;
    }
    const int oldLevel = hero->secSkill[skill];
    state::Set(state::HeroKey(subtype, hero->id, "oldLevel"), oldLevel);
    state::Set(state::HeroKey(subtype, hero->id, "addedSkill"), 0);
    state::Stamp(stamp, state::Week());
    state::Set(state::HeroKey(subtype, hero->id, "expiresWeek"), state::Week() + 1);
    if (oldLevel == 0 && hero->secSkillCount < 8)
    {
        hero->LearnSecondarySkill(skill, 1);
        state::Set(state::HeroKey(subtype, hero->id, "addedSkill"), 1);
    }
    // The original ERM intentionally works even with all 8 secondary-skill slots filled.
    // In that case the mastery byte is temporarily raised without inserting a visible skill slot.
    hero->secSkill[skill] = 3;
    if (human) H3Messagebox::Show(util::Tr("mushroom.granted").c_str(), H3PictureCategories::SecSkill(static_cast<eSecondary>(skill), eSecSkillLevel::EXPERT));
    return TRUE;
}

BOOL VisitArcaneTower(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (state::ObjectPlayerPeriod(item, ARCANE_TOWER, hero->owner, "week", state::Week()))
    {
        if (human) H3Messagebox::Show(util::Tr("arcane.visited").c_str());
        return TRUE;
    }
    if (!human)
    {
        hero->experience += 1000; // Preserve the source AI branch, but with native state.
        state::SetObjectPlayerPeriod(item, ARCANE_TOWER, hero->owner, "week", state::Week());
        return TRUE;
    }
    const int wanted = util::ChoosePrimary(util::Tr("arcane.chooseGain").c_str());
    if (wanted < 0)
        return TRUE;
    const int paid = util::ChoosePrimary(util::Tr("arcane.choosePay").c_str(), wanted);
    if (paid < 0)
        return TRUE;
    int trades = static_cast<unsigned char>(hero->primarySkill[paid]);
    if (paid > 1)
        --trades;
    if (trades < 1 || trades > 249)
    {
        H3Messagebox::Show(util::Tr("arcane.cannotPay").c_str());
        return TRUE;
    }
    int made = 0;
    while (trades-- > 0)
    {
        if (!H3Messagebox::Choice(util::Tr("arcane.confirm", {"left", std::to_string(trades + 1)}).c_str(),
                                  H3PictureCategories::PrimarySkill(static_cast<ePrimary>(wanted), 1),
                                  H3PictureCategories::PrimarySkill(static_cast<ePrimary>(paid), 1)))
            break;
        --hero->primarySkill[paid];
        ++hero->primarySkill[wanted];
        ++made;
    }
    if (made)
        state::SetObjectPlayerPeriod(item, ARCANE_TOWER, hero->owner, "week", state::Week());
    return TRUE;
}

int DarknessDragonFightValue()
{
    return util::ValidCreature(creature::DARKNESS_DRAGON)
               ? P_CreatureInformation[creature::DARKNESS_DRAGON].fightValue
               : 1000000;
}
int BloodDragonFightValue()
{
    return util::ValidCreature(creature::BLOOD_DRAGON)
               ? P_CreatureInformation[creature::BLOOD_DRAGON].fightValue
               : DarknessDragonFightValue();
}
int RandomNeutralForLevel(const int level)
{
    // Exact non-upgraded neutral table from WOG_14_GetNeutralMon. Extended IDs are validated at runtime.
    if (level == 0 && util::ValidCreature(eCreature::HALFLING)) return eCreature::HALFLING;
    if (level == 1 && util::ValidCreature(eCreature::ROGUE)) return eCreature::ROGUE;
    if (level == 2 && util::ValidCreature(eCreature::MUMMY)) return eCreature::MUMMY;
    if (level == 3 && util::ValidCreature(eCreature::SHARPSHOOTER)) return eCreature::SHARPSHOOTER;
    if (level == 4)
    {
        const int candidates[] = {creature::ARCTIC_SHARPSHOOTER, creature::LAVA_SHARPSHOOTER, eCreature::GOLD_GOLEM};
        std::vector<int> valid;
        for (int id : candidates) if (util::ValidCreature(id)) valid.push_back(id);
        if (!valid.empty()) return valid[util::Random(0, static_cast<int>(valid.size()) - 1)];
    }
    if (level == 5)
    {
        // Standard WoG: Sorceress=193. If another creature pack omits it, choose among the valid source alternatives.
        const int candidates[] = {eCreature::ENCHANTER, creature::SORCERESS, eCreature::DIAMOND_GOLEM};
        std::vector<int> valid;
        for (int id : candidates) if (util::ValidCreature(id)) valid.push_back(id);
        if (!valid.empty()) return valid[util::Random(0, static_cast<int>(valid.size()) - 1)];
    }
    if (level == 6 && util::ValidCreature(creature::GORYNYCH)) return creature::GORYNYCH;
    if (level == 7 && util::ValidCreature(eCreature::FAERIE_DRAGON)) return eCreature::FAERIE_DRAGON;

    // Compatibility fallback for creature packs that remove a standard WoG neutral.
    int best = -1;
    for (int i = 0; i < H3CreatureCount::Get(); ++i)
        if (P_CreatureInformation[i].town < 0 && P_CreatureInformation[i].level == (std::min)(6, level))
            best = i;
    return best;
}
int TransformTarget(const int source, int town)
{
    if (!util::ValidCreature(source)) return source;
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
        if (human) H3Messagebox::Show(util::Tr("altar.visited").c_str());
        return TRUE;
    }
    const int ceiling = DarknessDragonFightValue();
    bool any = false;
    for (int i = 0; i < 7; ++i)
    {
        const int mon = hero->army.type[i];
        if (!util::ValidCreature(mon) || hero->army.count[i] <= 0) continue;
        if (mon == creature::GHOST || (mon >= creature::FIRST_EMISSARY && mon <= creature::LAST_EMISSARY)) continue;
        if (P_CreatureInformation[mon].fightValue <= ceiling) any = true;
    }
    if (!any)
    {
        if (human) H3Messagebox::Show(util::Tr("altar.noMon").c_str());
        return TRUE;
    }
    int chance = 100;
    int town = (std::max)(0, (std::min)(8, hero->hero_class / 2));
    if (human)
    {
        const int mana = (std::max)(0, (std::min)(100, static_cast<int>(hero->spellPoints)));
        std::vector<int> investments;
        const int raw[] = {mana, mana * 3 / 4, mana * 2 / 3, mana / 2, mana / 3, mana / 4, mana / 10, 0};
        for (int value : raw)
            if ((value > 0 || investments.empty() || investments.back() != 0) &&
                std::find(investments.begin(), investments.end(), value) == investments.end())
                investments.push_back(value);
        if (investments.empty() || investments.back() != 0)
            investments.push_back(0);

        H3PictureVector pics;
        for (int value : investments)
            pics.Add(H3PictureCategories::Resource(eResource::GOLD, value)); // numeric selector; the text identifies Mana.
        const int selected = util::ChoosePictures(util::Tr("altar.chooseMana", {"mana", std::to_string(mana)}).c_str(), pics);
        if (selected < 0 || selected >= static_cast<int>(investments.size()))
            return TRUE;

        chance = investments[selected]; // Original ERM: success chance equals the actual mana invested (0..100).
        if (chance > 0)
        {
            const int selectedTown = util::ChooseTown(util::Tr("altar.chooseTown").c_str(), town);
            if (selectedTown < -1)
                return TRUE; // Native bugfix: cancel does not consume mana.
            town = selectedTown; // -1 is the explicit Neutral destination from the ERM dialog.
            hero->spellPoints -= static_cast<INT16>(chance);
        }
        else
        {
            // The ERM left townType uninitialized for the zero-mana option. Define it as fully random instead.
            town = -2;
        }
    }
    for (int i = 0; i < 7; ++i)
    {
        const int mon = hero->army.type[i];
        if (!util::ValidCreature(mon) || hero->army.count[i] <= 0) continue;
        if (mon == creature::GHOST || (mon >= creature::FIRST_EMISSARY && mon <= creature::LAST_EMISSARY)) continue;
        if (P_CreatureInformation[mon].fightValue > ceiling) continue;
        // Source skips creature types already belonging to the chosen destination, preserving upgrades/stack experience.
        if (town >= -1 && P_CreatureInformation[mon].town == town) continue;
        int selectedTown = (town >= -1 && util::Random(0, 100) <= chance) ? town : util::Random(-1, 8);
        const int target = TransformTarget(mon, selectedTown);
        if (util::ValidCreature(target)) hero->army.type[i] = target;
    }
    state::SetObjectPlayerPeriod(item, ALTAR_OF_TRANSFORMATION, hero->owner, "week", state::Week());
    return TRUE;
}

struct AcademyOffer { int skill; int artifact; int price; };
AcademyOffer GetAcademyOffer()
{
    const int day = state::Day();
    const std::string stamp = "wog_objects.native.32.offer.day";
    if (!state::IsStamp(stamp, day))
    {
        static const int skills[] = {1, 6, 9, 10, 19, 20, 22, 23};
        const int skill = skills[util::Random(0, 7)];
        static const int arts[] = {7,8,9,10,11,18,19,20,21,22,31,38,39,40,60,91,146,147,148,152,154};
        std::vector<int> enabled;
        for (int id : arts) if (util::ArtifactEnabled(id)) enabled.push_back(id);
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

bool AcademyVisited(const H3Hero *hero) { return state::IsStamp(state::HeroKey(BATTLE_ACADEMY, hero->id, "day"), state::Day()); }
void MarkAcademy(H3Hero *hero) { state::Stamp(state::HeroKey(BATTLE_ACADEMY, hero->id, "day"), state::Day()); }
BOOL VisitAcademy(H3Hero *hero, const bool human)
{
    if (AcademyVisited(hero))
    {
        if (human) H3Messagebox::Show(util::Tr("academy.visited").c_str());
        return TRUE;
    }
    const AcademyOffer offer = GetAcademyOffer();
    auto &res = P_Main->players[hero->owner].playerResources;
    if (!human)
    {
        // Improved AI: evaluate all source-legal services instead of choosing a random source-script branch.
        // Attack/defence are free for AI in the original ERM and therefore remain the default value choice.
        int bestPrimary = hero->primarySkill[0] <= hero->primarySkill[1] ? 0 : 1;
        long long bestGain = 1;
        int action = 0; // 0 primary, 1 secondary, 2 artifact
        const int secCost = AcademySecondaryCost(hero, offer.skill);
        if (offer.skill >= 0 && offer.skill < 28 && hero->secSkill[offer.skill] < 3 && res.gold >= secCost)
        {
            const long long gain = 2500LL * (3 - hero->secSkill[offer.skill]);
            if (gain > bestGain * (1 + secCost / 1000)) { bestGain = gain; action = 1; }
        }
        if (offer.artifact >= 0 && res.gold >= offer.price)
        {
            const long long gain = (std::max)(1, H3ArtifactSetup::Get()[offer.artifact].cost);
            if (gain * 2 > bestGain * (1 + offer.price / 1000)) action = 2;
        }
        if (action == 1)
        {
            hero->AddResource(eResource::GOLD, -secCost);
            if (!hero->secSkill[offer.skill]) hero->LearnSecondarySkill(offer.skill, 1); else ++hero->secSkill[offer.skill];
        }
        else if (action == 2)
        {
            hero->AddResource(eResource::GOLD, -offer.price);
            hero->GiveArtifact(H3Artifact(offer.artifact, -1));
        }
        else
            ++hero->primarySkill[bestPrimary];
        MarkAcademy(hero);
        return TRUE;
    }
    if (H3Messagebox::Choice(util::Tr("academy.primaryQuestion").c_str()))
    {
        const auto picked = H3Messagebox::Choose(util::Tr("academy.choosePrimary").c_str(),
            H3PictureCategories::PrimarySkill(ePrimary::ATTACK, 1),
            H3PictureCategories::PrimarySkill(ePrimary::DEFENSE, 1));
        const int skill = picked == H3Messagebox::LEFT ? 0 : (picked == H3Messagebox::RIGHT ? 1 : -1);
        if (skill >= 0)
        {
            const int clean = static_cast<unsigned char>(hero->primarySkill[skill]);
            const int cost = clean == 0 ? 0 : clean * 500;
            if (res.gold >= cost && (cost == 0 || H3Messagebox::Choice(util::Tr("academy.pay", {"gold", std::to_string(cost)}).c_str())))
            {
                if (cost) hero->AddResource(eResource::GOLD, -cost);
                ++hero->primarySkill[skill]; MarkAcademy(hero); return TRUE;
            }
        }
    }
    const int secondaryCost = AcademySecondaryCost(hero, offer.skill);
    if (offer.skill >= 0 && offer.skill < 28 && hero->secSkill[offer.skill] < 3 && res.gold >= secondaryCost &&
        H3Messagebox::Choice(util::Tr("academy.secondaryQuestion", {"gold", std::to_string(secondaryCost)}).c_str(),
                             H3PictureCategories::SecSkill(static_cast<eSecondary>(offer.skill),
                             static_cast<eSecSkillLevel>((std::max)(1, static_cast<int>(hero->secSkill[offer.skill]))))))
    {
        hero->AddResource(eResource::GOLD, -secondaryCost);
        if (!hero->secSkill[offer.skill]) hero->LearnSecondarySkill(offer.skill, 1); else ++hero->secSkill[offer.skill];
        MarkAcademy(hero); return TRUE;
    }
    if (offer.artifact >= 0 && res.gold >= offer.price &&
        H3Messagebox::Choice(util::Tr("academy.artifactQuestion", {"gold", std::to_string(offer.price)}).c_str(),
                             H3PictureCategories::Artifact(static_cast<eArtifact>(offer.artifact))))
    {
        hero->AddResource(eResource::GOLD, -offer.price);
        hero->GiveArtifact(H3Artifact(offer.artifact, -1));
        MarkAcademy(hero);
    }
    return TRUE;
}

void RestoreExpiredMushrooms()
{
    if (!P_Main) return;
    const int subs[] = {MAGIC_AIR_MUSHROOM, MAGIC_WATER_MUSHROOM, MAGIC_EARTH_MUSHROOM, MAGIC_FIRE_MUSHROOM};
    for (int id = 0; id < H3HeroCount::Get(); ++id)
    {
        H3Hero *hero = P_Main->GetHero(id);
        if (!hero || hero->owner < 0) continue;
        for (int subtype : subs)
        {
            const int expires = state::Get(state::HeroKey(subtype, id, "expiresWeek"), 0);
            if (!expires || state::Week() < expires) continue;
            const int skill = MushroomSkill(subtype);
            const int oldLevel = state::Get(state::HeroKey(subtype, id, "oldLevel"), 0);
            const bool addedSkill = state::Get(state::HeroKey(subtype, id, "addedSkill"), 0) != 0;
            if (oldLevel == 0 && addedSkill)
                hero->UnlearnSkill(skill);
            else
                hero->secSkill[skill] = static_cast<INT8>(oldLevel);
            state::Set(state::HeroKey(subtype, id, "addedSkill"), 0);
            state::Set(state::HeroKey(subtype, id, "expiresWeek"), 0);
        }
    }
}

_ERH_(OnEveryDayBasic) { RestoreExpiredMushrooms(); }

} // namespace

// Exposed to dispatcher in WoGObjectHandlers.cpp.
BOOL VisitBasic(H3Hero *hero, H3MapItem *item, const int subtype, const BOOL human)
{
    if (!hero || !item || hero->owner < 0 || hero->owner >= 8) return TRUE;
    switch (subtype)
    {
    case MYSTERIOUS_DWELLING: return VisitMysteriousDwelling(hero, item, human != FALSE);
    case ARCANE_TOWER: return VisitArcaneTower(hero, item, human != FALSE);
    case MAGIC_AIR_MUSHROOM:
    case MAGIC_WATER_MUSHROOM:
    case MAGIC_EARTH_MUSHROOM:
    case MAGIC_FIRE_MUSHROOM: return VisitMushroom(hero, subtype, human != FALSE);
    case ALTAR_OF_TRANSFORMATION: return VisitAltar(hero, item, human != FALSE);
    case ALMS_HOUSE: return VisitAlmsHouse(hero, human != FALSE);
    case BATTLE_ACADEMY: return VisitAcademy(hero, human != FALSE);
    default: return FALSE;
    }
}

void RegisterBasicEvents() { _REH_(OnEveryDayBasic); }

}} // namespace wog::handlers
