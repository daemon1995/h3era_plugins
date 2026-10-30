#include <algorithm>
#include <array>
#include <limits>
#include <vector>

#include "WoGObjectCaveData.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include "WoGObjectChoiceDialogs.h"

namespace wog
{
namespace handlers
{
namespace
{
using namespace objects;
int SpellPrice(const int spell)
{
    return spell >= 0 && spell < 70 ? cave_data::schoolSpellPrices[spell] : 0;
}
bool HasBook(const H3Hero *hero)
{
    return hero->bodyArtifacts[17].id == eArtifact::SPELLBOOK;
}
bool CanStudySpell(const H3Hero *hero, const int spell)
{
    return spell >= 0 && spell < (std::min)(70, int(H3SpellCount::Get())) && !P_Game->disabledSpells[spell] &&
           !hero->learnedSpells[spell] && P_Spell[spell].level <= hero->secSkill[eSecondary::WISDOM] + 2;
}
int SecondaryCost(const H3Hero *hero, const int skill)
{
    if (hero->secSkill[skill])
        return 3000;
    int occupied = 0;
    for (int i = 0; i < 28; ++i)
        occupied += hero->secSkill[i] > 0;
    int limit = ByteAt(0x4A7DD0);
    if (limit < 1 || limit > 28)
        limit = 8;
    return 3000 + (std::max)(0, occupied - limit + 1) * 50000;
}
bool SchoolVisited(const H3Hero *hero)
{
    return state::IsStamp(state::HeroKey(SCHOOL_OF_WIZARDRY, hero->id, "day"), state::Day());
}
void MarkSchool(const H3Hero *hero)
{
    state::Stamp(state::HeroKey(SCHOOL_OF_WIZARDRY, hero->id, "day"), state::Day());
}
void PrepareSchoolOffer()
{
    const int day = state::Day();
    if (state::IsStamp("wog_objects.native.62.offer.day", day))
        return;
    const int skills[] = {7, 8, 11, 14, 15, 16, 17, 18, 24, 25, 7, 14, 15, 16, 17};
    state::Set("wog_objects.native.62.offer.skill", skills[util::Random(0, 14)]);
    const int level = (day * 11 / 7) % 4 + 1;
    std::vector<int> spells;
    for (int id = 0; id < (std::min)(70, int(H3SpellCount::Get())); ++id)
        if (P_Spell[id].level == level && !P_Game->disabledSpells[id])
            spells.push_back(id);
    state::Set("wog_objects.native.62.offer.spell",
               spells.empty() ? -1 : spells[util::Random(0, int(spells.size()) - 1)]);
    state::Stamp("wog_objects.native.62.offer.day", day);
}
struct Study
{
    int kind = 0, id = -1, cost = 0, source = -1, modifier = -1;
    int gain = 0;
};
std::vector<Study> SchoolOffers(H3Hero *hero, const bool human, const int kind = -1)
{
    std::vector<Study> out;
    if (SchoolVisited(hero))
        return out;
    PrepareSchoolOffer();
    const int skill = state::Get("wog_objects.native.62.offer.skill", 7);
    const int spell = state::Get("wog_objects.native.62.offer.spell", -1);
    for (int primary = 2; primary < 4; ++primary)
        if ((kind < 0 || kind == primary - 2) && static_cast<unsigned char>(hero->primarySkill[primary]) < 125)
        {
            const int cost = human ? (std::max)(0, util::CleanPrimary(hero, primary) - 1) * 500 : 0;
            out.push_back({primary - 2, primary, cost, -1, -1, 900});
        }
    if ((kind < 0 || kind == 2) && skill >= 0 && skill < 28 && hero->secSkill[skill] < 3)
    {
        const int gain = (skill >= 14 && skill <= 17 ? 2400 : skill == 7 ? 2800 : 1200) * (3 - hero->secSkill[skill]);
        out.push_back({2, skill, SecondaryCost(hero, skill), -1, -1, gain});
    }
    const int book = HasBook(hero) ? 0 : 250;
    if ((kind < 0 || kind == 3) && CanStudySpell(hero, spell))
        out.push_back({3, spell, SpellPrice(spell) * 5 + book, -1, -1, 800 * P_Spell[spell].level});
    if ((kind < 0 || kind == 4) && hero->secSkill[eSecondary::WISDOM] == 3)
        for (int slot = 0; slot < 83; ++slot)
        {
            const auto &art = slot < 19 ? hero->bodyArtifacts[slot] : hero->backpackArtifacts[slot - 19];
            if (art.id == eArtifact::SPELL_SCROLL && CanStudySpell(hero, art.subtype))
                out.push_back({4, art.subtype, SpellPrice(art.subtype) + book, slot, art.subtype,
                               1000 * P_Spell[art.subtype].level});
        }
    return out;
}
bool ApplyStudy(H3Hero *hero, const Study &offer, const bool human, const bool continuingCopy = false)
{
    if ((!continuingCopy && SchoolVisited(hero)) || util::Resource(hero->owner, 6) < offer.cost)
        return false;
    if (offer.kind <= 1)
    {
        if (offer.cost != (human ? (std::max)(0, util::CleanPrimary(hero, offer.id) - 1) * 500 : 0))
            return false;
        if (!util::GivePrimary(hero, offer.id, 1))
            return false;
    }
    else if (offer.kind == 2)
    {
        if (hero->secSkill[offer.id] >= 3 || SecondaryCost(hero, offer.id) != offer.cost ||
            !util::LearnSkill(hero, offer.id, hero->secSkill[offer.id] + 1))
            return false;
    }
    else
    {
        if (!CanStudySpell(hero, offer.id))
            return false;
        H3Artifact input;
        if (offer.kind == 4)
        {
            input = offer.source < 19 ? hero->bodyArtifacts[offer.source] : hero->backpackArtifacts[offer.source - 19];
            if (input.id != eArtifact::SPELL_SCROLL || input.subtype != offer.modifier)
                return false;
        }
        if (!HasBook(hero))
        {
            if (hero->bodyArtifacts[17].id >= 0)
                return false;
            hero->GiveArtifact(H3Artifact(eArtifact::SPELLBOOK, -1), 17);
            if (!HasBook(hero))
                return false;
        }
        util::LearnSpell(hero, offer.id);
        if (!hero->learnedSpells[offer.id])
            return false;
        if (offer.kind == 4)
        {
            if (offer.source < 19)
                hero->RemoveArtifact(offer.source);
            else
                hero->RemoveBackpackArtifact(offer.source - 19);
        }
    }
    util::ChangeResource(hero, eResource::GOLD, -offer.cost);
    MarkSchool(hero);

    return true;
}
int StudyScore(const H3Hero *hero, const Study &offer)
{
    if (offer.cost > util::Resource(hero->owner, 6))
        return 0;
    // An AI never pays a huge overflow premium for a ninth skill while free useful primary training exists.
    if (offer.cost > 10000)
        return 0;
    return (std::max)(0, offer.gain - offer.cost / 3);
}
BOOL VisitSchool(H3Hero *hero, const bool human)
{
    if (SchoolVisited(hero))
    {
        if (human)
            H3Messagebox::Show(util::Tr("school.visited").c_str());
        return TRUE;
    }
    if (!human)
    {
        const auto offers = SchoolOffers(hero, false);
        const Study *best = nullptr;
        int score = 0;
        for (const auto &offer : offers)
            if (StudyScore(hero, offer) > score)
            {
                score = StudyScore(hero, offer);
                best = &offer;
            }
        if (best)
            ApplyStudy(hero, *best, false);
        return TRUE;
    }
    PrepareSchoolOffer();
    H3PictureVector choices;
    choices.Add(H3PictureCategories::PrimarySkill(ePrimary::SPELL_POWER, 1));
    choices.Add(H3PictureCategories::PrimarySkill(ePrimary::KNOWLEDGE, 1));
    choices.Add(H3PictureCategories::SecSkill(
        static_cast<eSecondary>(state::Get("wog_objects.native.62.offer.skill", 7)), eSecSkillLevel::EXPERT));
    const int spell = state::Get("wog_objects.native.62.offer.spell", -1);
    choices.Add(spell >= 0 ? H3PictureCategories::Spell(static_cast<eSpell>(spell))
                           : H3PictureCategories::Artifact(eArtifact::SPELLBOOK));
    choices.Add(H3PictureCategories::Artifact(eArtifact::SPELL_SCROLL));
    const int kind = util::ChoosePictures(util::Tr("school.menu").c_str(), choices);
    if (kind < 0)
        return TRUE;
    bool copied = false;
    for (;;)
    {
        auto offers = SchoolOffers(hero, true, kind);
        // Copying several different scrolls is one school service, as in the source script.
        if (copied)
        {
            const std::string stamp = state::HeroKey(SCHOOL_OF_WIZARDRY, hero->id, "day");
            state::Set(stamp, 0);
            offers = SchoolOffers(hero, true, kind);
            state::Stamp(stamp, state::Day());
        }
        if (offers.empty())
        {
            if (!copied)
                H3Messagebox::Show(util::Tr("school.unavailable").c_str());
            break;
        }
        std::sort(offers.begin(), offers.end(), [](const Study &a, const Study &b) { return a.source > b.source; });
        for (auto offer : offers)
        {
            if (offer.kind >= 3)
                offer.cost = SpellPrice(offer.id) * (offer.kind == 3 ? 5 : 1) + (HasBook(hero) ? 0 : 250);
            const int owner = hero->owner, day = state::Day();
            const std::string name = offer.kind <= 1   ? P_PrimarySkillName[offer.id]
                                     : offer.kind == 2 ? P_SecondarySkillInfo[offer.id].name
                                                       : P_Spell[offer.id].name;
            const bool accept = H3Messagebox::Choice(
                util::Tr("school.confirm", {"name", name, "gold", std::to_string(offer.cost)}).c_str());
            if (!accept)
                continue;
            if (hero->owner != owner || state::Day() != day || !ApplyStudy(hero, offer, true, copied))
            {
                H3Messagebox::Show(util::Tr("school.changed").c_str());
                continue;
            }
            copied = kind == 4;
            if (!copied)
                return TRUE;
        }
        break;
    }
    return TRUE;
}

struct Summon
{
    int town = -1, level = -1, upgraded = 0, sourceSlot = -1, sourceHero = -1, creature = -1, count = 0;
    std::array<int, 7> cost{};
    int64_t gain = 0;
};
H3Town *Town(const int number)
{
    for (auto &town : P_Main->towns)
        if (town.number == number)
            return &town;
    return nullptr;
}
bool SummonAffordable(const H3Hero *hero, const Summon &offer)
{
    if (!util::CanAddArmy(hero->army, {{offer.creature, offer.count}}))
        return false;
    for (int i = 0; i < 7; ++i)
        if (util::Resource(hero->owner, i) < offer.cost[i])
            return false;
    return true;
}
std::vector<Summon> SummonOffers(H3Hero *hero, H3MapItem *item, const bool includeGarrison)
{
    std::vector<Summon> out;
    if (state::ObjectDay(item, state::Day()))
        return out;
    for (auto &town : P_Main->towns)
    {
        if (town.owner != hero->owner)
            continue;
        const auto &types = town.GetCreatureTypes();
        for (int level = 0; level < 7; ++level)
        {
            const int up = town.recruits[1][level] > 0 ? 1 : 0;
            const int count = town.recruits[up][level], type = up ? types.upgrade[level] : types.base[level];
            if (count <= 0 || !util::ValidCreature(type))
                continue;
            Summon offer;
            offer.town = town.number;
            offer.level = level;
            offer.upgraded = up;
            offer.creature = type;
            offer.count = count;
            bool valid = true;
            for (int i = 0; i < 7; ++i)
            {
                const int64_t cost = int64_t(P_CreatureInformation[type].cost.asArray[i]) * count;
                if (cost < 0 || cost > (std::numeric_limits<int>::max)())
                {
                    valid = false;
                    break;
                }
                offer.cost[i] = int(cost);
            }
            offer.gain = int64_t((std::max)(0, P_CreatureInformation[type].aiValue)) * count;
            if (valid && (includeGarrison || SummonAffordable(hero, offer)))
                out.push_back(offer);
        }
        if (!includeGarrison)
            continue;
        H3Hero *garrison = town.GetGarrisonHero();
        if (garrison == hero)
            continue;
        const H3Army &army = garrison ? garrison->army : town.guards;
        for (int slot = 0; slot < 7; ++slot)
        {
            if (!util::ValidCreature(army.type[slot]) || army.count[slot] <= 0)
                continue;
            Summon offer;
            offer.town = town.number;
            offer.sourceSlot = slot;
            offer.sourceHero = garrison ? garrison->id : -1;
            offer.creature = army.type[slot];
            offer.count = army.count[slot];
            offer.gain = int64_t(P_CreatureInformation[offer.creature].aiValue) * offer.count;
            if (SummonAffordable(hero, offer))
                out.push_back(offer);
        }
    }
    return out;
}
bool ApplySummon(H3Hero *hero, H3MapItem *item, const Summon &offer)
{
    auto *town = Town(offer.town);
    if (!town || town->owner != hero->owner || !SummonAffordable(hero, offer) ||
        state::ObjectDay(item, state::Day()))
        return false;
    H3Hero *garrison = offer.sourceHero >= 0 ? P_Main->GetHero(offer.sourceHero) : nullptr;
    H3Army *source = offer.sourceSlot >= 0 ? (garrison ? &garrison->army : &town->guards) : nullptr;
    if (source)
    {
        if (town->garrisonHero != offer.sourceHero || source->type[offer.sourceSlot] != offer.creature ||
            source->count[offer.sourceSlot] != offer.count)
            return false;
    }
    else if (town->recruits[offer.upgraded][offer.level] != offer.count)
        return false;
    if (source)
    {
        if (!util::TransferArmyStack(*source, offer.sourceSlot, hero->army))
            return false;
    }
    else
    {
        if (!util::AddArmy(hero->army, {{offer.creature, offer.count}}))
            return false;
        town->recruits[0][offer.level] = town->recruits[1][offer.level] = 0;
    }
    for (int resource = 0; resource < 7; ++resource)
        if (offer.cost[resource])
            util::ChangeResource(hero, static_cast<eResource>(resource), -offer.cost[resource]);
    state::SetObjectDay(item, state::Day(), hero);
    return true;
}
BOOL VisitSummoning(H3Hero *hero, H3MapItem *item, const bool human)
{
    auto offers = SummonOffers(hero, item, human);
    if (offers.empty())
    {
        if (human)
            H3Messagebox::Show(util::Tr("summon.unavailable").c_str());
        return TRUE;
    }
    if (!human)
    {
        const auto best = std::max_element(offers.begin(), offers.end(),
                                           [](const Summon &a, const Summon &b) { return a.gain < b.gain; });
        ApplySummon(hero, item, *best);
        return TRUE;
    }

    std::vector<SummoningOffer> rows;
    for (const auto &offer : offers)
    {
        SummoningOffer row;
        row.creature = offer.creature; row.count = offer.count;
        const auto *town = Town(offer.town);
        row.town = town ? town->name.String() : h3_NullString;
        if (offer.sourceSlot >= 0) row.price = util::Tr("summon.garrison");
        else
        {
            std::string price;
            for (int resource = 0; resource < 7; ++resource)
            {
                const int amount = P_CreatureInformation[offer.creature].cost.asArray[resource];
                if (amount > 0) price += (price.empty() ? h3_NullString : ", ") + std::to_string(amount) + " " + P_ResourceName[resource];
            }
            row.price = util::Tr("summon.unitPrice", {"price", price});
        }
        rows.push_back(std::move(row));
    }
    ShowSummoningOffers(rows, [&](int index) {
        if (index < 0 || index >= int(offers.size()) || state::ObjectDay(item, state::Day())) return false;
        const auto &offer = offers[index];
        auto *town = Town(offer.town);
        if (!town || town->owner != hero->owner) return false;
        if (offer.sourceSlot >= 0)
        {
            const auto message = util::Tr("summon.transport", {"town", town->name.String(), "creature",
                P_CreatureInformation[offer.creature].namePlural, "count", std::to_string(offer.count)});
            return H3Messagebox::Choice(message.c_str()) && ApplySummon(hero, item, offer);
        }
        if (town->recruits[offer.upgraded][offer.level] <= 0) return false;
        const int before = town->recruits[offer.upgraded][offer.level];
        // The exact town-dwelling constructor supplies its types and stock pointers. Native hiring
        // handles the amount slider, every resource, army capacity and cancellation.
        H3RecruitMgr manager(*town, offer.level + offer.upgraded * 7);
        manager.army = &hero->army;
        const int previous = P_WindowManager->resultItemID;
        manager.Run();
        P_WindowManager->resultItemID = previous;
        const bool hired = town->recruits[offer.upgraded][offer.level] < before;
        if (hired) state::SetObjectDay(item, state::Day(), hero);
        return hired;
    });

    return TRUE;
}
void __stdcall SchoolDay(Era::TEvent *)
{
    PrepareSchoolOffer();
}
} // namespace

BOOL VisitLearningObject(H3Hero *hero, H3MapItem *item, const BOOL human)
{
    if (!hero || !item || hero->owner < 0 || hero->owner > 7)
        return TRUE;
    return item->objectSubtype == SCHOOL_OF_WIZARDRY ? VisitSchool(hero, human != FALSE)
                                                     : VisitSummoning(hero, item, human != FALSE);
}
int LearningAiWeight(H3MapItem *item, H3Hero *hero)
{
    if (item->objectSubtype == SUMMONING_STONES)
    {
        int64_t best = 0;
        for (const auto &offer : SummonOffers(hero, item, false))
            best = (std::max)(best, offer.gain);
        return int((std::min)(best, int64_t(20000)));
    }
    int best = 0;
    for (const auto &offer : SchoolOffers(hero, false))
        best = (std::max)(best, StudyScore(hero, offer));
    return best;
}
void RegisterLearningEvents()
{
    Era::RegisterHandler(SchoolDay, "OnEveryDay");
}
} // namespace handlers
} // namespace wog
