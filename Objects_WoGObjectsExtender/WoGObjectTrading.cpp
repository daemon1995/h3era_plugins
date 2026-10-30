#include <algorithm>
#include <limits>
#include <vector>

#include "WoGObjectHandlers.h"
#include "WoGObjectChoiceDialogs.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include "WoGObjectsExtender.h"

namespace wog
{
namespace handlers
{
namespace
{
using namespace objects;

enum RewardKind
{
    GOLD,
    RESOURCE,
    CREATURES,
    ARTIFACT,
    PRIMARY,
    SPELL_POINTS,
    SPELL,
    EXPERIENCE,
    REVEAL
};
struct JunkTrade
{
    int art;
    RewardKind kind;
    int a;
    int b;
};

const JunkTrade kTrades[] = {{7, CREATURES, 15, 50},
                             {13, CREATURES, 17, 15},
                             {15, ARTIFACT, 16, 0},
                             {19, RESOURCE, 5, 5},
                             {25, PRIMARY, 2, 1},
                             {46, GOLD, 1500, 0},
                             {47, GOLD, 1500, 0},
                             {48, GOLD, 1500, 0},
                             {49, GOLD, 1500, 0},
                             {50, GOLD, 1500, 0},
                             {51, GOLD, 1500, 0},
                             {52, REVEAL, 30, 0},
                             {53, GOLD, 1830, 0},
                             {54, GOLD, 600, 0},
                             {55, GOLD, 2500, 0},
                             {56, GOLD, 2000, 0},
                             {58, GOLD, 4000, 0},
                             {63, CREATURES, 5, 15},
                             {66, RESOURCE, 4, 10},
                             {67, RESOURCE, 5, 10},
                             {68, RESOURCE, 1, 10},
                             {71, RESOURCE, 0, 15},
                             {72, CREATURES, creature::SUPREME_ARCHANGEL, 2},
                             {73, SPELL_POINTS, 60, 0},
                             {76, SPELL, 54, 0},
                             {83, GOLD, 2500, 0},
                             {84, GOLD, 2000, 0},
                             {85, GOLD, 500, 0},
                             {92, EXPERIENCE, 2000, 0},
                             {93, ARTIFACT, 79, 0},
                             {106, SPELL, 17, 0},
                             {118, RESOURCE, 2, 15}};

void RevealRadius(const H3Hero *hero, const int radius)
{
    if (!hero || hero->owner < 0 || hero->owner >= 8)
        return;
    const int size = H3MapSize::Get();
    for (int y = (std::max)(0, static_cast<int>(hero->y) - radius);
         y <= (std::min)(size - 1, static_cast<int>(hero->y) + radius); ++y)
        for (int x = (std::max)(0, static_cast<int>(hero->x) - radius);
             x <= (std::min)(size - 1, static_cast<int>(hero->x) + radius); ++x)
            if ((x - hero->x) * (x - hero->x) + (y - hero->y) * (y - hero->y) <= radius * radius)
                H3TileVision::RevealTile(x, y, hero->z, hero->owner);
    if (P_AdventureManager)
        P_AdventureManager->FullUpdate();
}

bool CanReceiveTrade(const H3Hero *hero, const JunkTrade &t)
{
    switch (t.kind)
    {
    case PRIMARY:
        return t.a >= 0 && t.a < 4 && int(static_cast<unsigned char>(hero->primarySkill[t.a])) + t.b <= 125;
    case ARTIFACT:
        return hero->backpackCount <= 64 && util::ArtifactEnabled(t.a); // The offered backpack item frees its slot.
    case SPELL:
        return t.a >= 0 && t.a < int(H3SpellCount::Get()) && hero->bodyArtifacts[17].id >= 0 &&
               !hero->learnedSpells[t.a] && !P_Game->disabledSpells[t.a];
    case SPELL_POINTS:
        return hero->spellPoints < 32767;
    case GOLD:
        return t.a <= (std::numeric_limits<int>::max)() - util::Resource(hero->owner, 6);
    case RESOURCE:
        return t.b <= (std::numeric_limits<int>::max)() - util::Resource(hero->owner, t.a);
    case EXPERIENCE:
        return t.a <= (std::numeric_limits<int>::max)() - hero->experience;
    default:
        break;
    }
    if (t.kind != CREATURES)
        return true;
    std::vector<std::pair<int, int>> s(1, std::make_pair(t.a, t.b));
    return util::CanAddArmy(hero->army, s);
}
void ApplyTrade(H3Hero *hero, const JunkTrade &t)
{
    switch (t.kind)
    {
    case GOLD:
        hero->AddResource(eResource::GOLD, t.a);
        break;
    case RESOURCE:
        hero->AddResource(t.a, t.b);
        break;
    case CREATURES:
        util::AddArmy(hero->army, {std::make_pair(t.a, t.b)});
        break;
    case ARTIFACT:
        if (util::ValidArtifact(t.a))
            hero->GiveArtifact(H3Artifact(t.a, -1));
        break;
    case PRIMARY:
        util::GivePrimary(hero, t.a, t.b);
        break;
    case SPELL_POINTS:
        hero->spellPoints = static_cast<INT16>((std::min)(32767, static_cast<int>(hero->spellPoints) + t.a));
        break;
    case SPELL:
        if (hero->bodyArtifacts[17].id >= 0 && t.a >= 0 && t.a < (std::min)(70, static_cast<int>(H3SpellCount::Get())))
            util::LearnSpell(hero, t.a);
        break;
    case EXPERIENCE:
        util::GiveExperience(hero, t.a);
        break;
    case REVEAL:
        RevealRadius(hero, t.a);
        break;
    }
}
std::string TradeRewardText(const JunkTrade &t)
{
    switch (t.kind)
    {
    case GOLD:
        return util::Tr("junk.rewardGold", {"value", std::to_string(t.a)});
    case RESOURCE:
        return std::to_string(t.b) + " " + P_ResourceName[t.a];
    case CREATURES:
        return util::ValidCreature(t.a) ? std::to_string(t.b) + " " + P_CreatureInformation[t.a].namePlural :
            util::Tr("junk.rewardCreatures", {"value", std::to_string(t.b)});
    case ARTIFACT:
        return util::ValidArtifact(t.a) ? H3Artifact(t.a, -1).GetName() : util::Tr("junk.rewardArtifact");
    case PRIMARY:
        return "+" + std::to_string(t.b) + " " + P_PrimarySkillName[t.a];
    case SPELL_POINTS:
        return util::Tr("junk.rewardMana", {"value", std::to_string(t.a)});
    case SPELL:
        return H3Spell::Get()[t.a].name;
    case EXPERIENCE:
        return util::Tr("junk.rewardExperience", {"value", std::to_string(t.a)});
    case REVEAL:
        return util::Tr("junk.rewardReveal", {"value", std::to_string(t.a)});
    default:
        return std::string();
    }
}
BOOL VisitJunkMerchant(H3Hero *hero, const bool human)
{
    if (human)
    {
        std::vector<JunkOffer> offers;
        for (const auto &t : kTrades)
        {
            if (!util::ValidArtifact(t.art)) continue;
            JunkOffer offer;
            offer.artifact = t.art; offer.reward = TradeRewardText(t);
            offer.rewardIcon = t.kind == ARTIFACT ? 1 : (t.kind == GOLD || t.kind == RESOURCE) ? 2 :
                t.kind == CREATURES ? 3 : t.kind == PRIMARY ? 4 : t.kind == SPELL ? 5 :
                t.kind == SPELL_POINTS ? 6 : t.kind == REVEAL ? 7 : 0;
            offer.rewardFrame = t.kind == GOLD ? 6 : t.kind == CREATURES ? t.a + 2 :
                t.kind == SPELL_POINTS ? 5 : t.kind == REVEAL ? 14 : t.a;
            offers.push_back(offer);
        }
        bool hasScroll = false;
        for (int spell = 0; spell < (std::min)(70, int(H3SpellCount::Get())); ++spell)
            if (util::CountBackpackArtifact(hero, eArtifact::SPELL_SCROLL, spell) > 0)
            {
                JunkOffer offer;
                offer.artifact = eArtifact::SPELL_SCROLL; offer.subtype = spell;
                offer.reward = util::Tr("junk.rewardGold", {"value", "500"});
                offer.rewardIcon = 2; offer.rewardFrame = 6; offers.push_back(offer);
                hasScroll = true;
            }
        if (!hasScroll)
        {
            JunkOffer offer;
            offer.artifact = eArtifact::SPELL_SCROLL; offer.rewardIcon = 2; offer.rewardFrame = 6;
            offer.reward = util::Tr("junk.rewardGold", {"value", "500"}); offers.push_back(offer);
        }
        const auto find = [](int artifact) -> const JunkTrade * {
            for (const auto &t : kTrades) if (t.art == artifact) return &t;
            return nullptr;
        };
        const auto refresh = [&](std::vector<JunkOffer> &list) {
            for (auto &offer : list)
            {
                offer.count = util::CountBackpackArtifact(hero, offer.artifact, offer.subtype);
                const auto *t = find(offer.artifact);
                const bool rewardFits = offer.artifact == eArtifact::SPELL_SCROLL ?
                    util::Resource(hero->owner, 6) <= (std::numeric_limits<int>::max)() - 500 :
                    t && CanReceiveTrade(hero, *t);
                offer.available = offer.count > 0 && rewardFits &&
                    (offer.artifact != eArtifact::SPELL_SCROLL || offer.subtype >= 0);
            }
        };
        const auto trade = [&](const JunkOffer &offer) {
            if (util::CountBackpackArtifact(hero, offer.artifact, offer.subtype) <= 0)
                return false;
            const auto *t = find(offer.artifact);
            if (offer.artifact == eArtifact::SPELL_SCROLL)
            {
                if (util::Resource(hero->owner, 6) > (std::numeric_limits<int>::max)() - 500 ||
                    !util::RemoveBackpackArtifact(hero, offer.artifact, offer.subtype)) return false;
                hero->AddResource(eResource::GOLD, 500);
            }
            else
            {
                if (!t || !CanReceiveTrade(hero, *t) || !util::RemoveBackpackArtifact(hero, offer.artifact)) return false;
                ApplyTrade(hero, *t);
            }
            return true;
        };
        ShowJunkMerchant(std::move(offers), refresh, trade);
        return TRUE;
    }
    // Spell scrolls are real H3Artifact id 1 + spell subtype in native hero data.
    for (int spell = 0, spellCount = (std::min)(70, static_cast<int>(H3SpellCount::Get())); spell < spellCount; ++spell)
    {
        while (util::CountBackpackArtifact(hero, 1, spell) > 0 &&
               util::Resource(hero->owner, 6) <= (std::numeric_limits<int>::max)() - 500 &&
               util::RemoveBackpackArtifact(hero, 1, spell))
            hero->AddResource(eResource::GOLD, 500);
    }
    for (const JunkTrade &t : kTrades)
    {
        // Preserve source AI rule: it refuses rewards consisting of creatures, but accepts other positive trades.
        while (t.kind != CREATURES && util::CountBackpackArtifact(hero, t.art) > 0 && CanReceiveTrade(hero, t) &&
               util::RemoveBackpackArtifact(hero, t.art))
            ApplyTrade(hero, t);
    }
    return TRUE;
}


class PalaceRandom
{
    std::string key;
    int seed;
  public:
    PalaceRandom(const H3MapItem *item, const H3Hero *hero)
    {
        const auto position = state::Position(item);
        key = state::ObjectKey(item, PALACE_OF_DREAMS, ("random.hero." + std::to_string(hero->id)).c_str(), hero->owner);
        const uint32_t initial = uint32_t(state::MapRandomSeed()) ^ uint32_t(position.x) * 0x9E3779B9U ^
            uint32_t(position.y) * 0x85EBCA6BU ^ uint32_t(position.z) * 0xC2B2AE35U ^ uint32_t(hero->id) * 0x27D4EB2FU;
        seed = state::Get(key, int(initial));
    }
    int Next(int minimum, int maximum)
    {
        const int value = Era::SplitMix32(&seed, minimum, maximum);
        state::Set(key, seed);
        return value;
    }
};
struct SlotResult
{
    int a, b, c;
    bool win;
};
SlotResult SpinResource(const bool mithril, PalaceRandom &random)
{
    const int maxId = mithril ? 7 : 6;
    SlotResult r{random.Next(0, maxId), random.Next(0, maxId), random.Next(0, maxId), false};
    if (random.Next(0, 15) == 0)
        r.a = r.c;
    if (random.Next(0, 15) == 0)
        r.b = r.a;
    if (random.Next(0, 15) == 0)
        r.c = r.b;
    r.win = r.a == r.b && r.b == r.c;
    return r;
}
std::vector<int> PalaceArtifacts()
{
    std::vector<int> out;
    for (int i = 7; i < H3ArtifactCount::Get(); ++i)
        if (util::ArtifactEnabled(i) && H3ArtifactSetup::Get()[i].type > eArtifactType::SPECIAL)
            out.push_back(i);
    return out;
}
SlotResult SpinArtifact(const std::vector<int> &pool, PalaceRandom &random)
{
    if (pool.empty())
        return {-1, -1, -1, false};
    auto pick = [&]() {
        int id = pool[random.Next(0, static_cast<int>(pool.size()) - 1)];
        if (H3ArtifactSetup::Get()[id].type == eArtifactType::RELIC && random.Next(0, 1) == 0)
            id = pool[random.Next(0, static_cast<int>(pool.size()) - 1)];
        return id;
    };
    SlotResult r{pick(), pick(), pick(), false};
    int copyChance = 8 - static_cast<int>(pool.size()) / 100;
    copyChance = (std::max)(4, (std::min)(8, copyChance));
    if (random.Next(0, copyChance) == 0)
        r.a = r.c;
    if (random.Next(0, copyChance) == 0)
        r.b = r.a;
    if (random.Next(0, copyChance) == 0)
        r.c = r.b;
    r.win = r.a == r.b && r.b == r.c;
    return r;
}
BOOL VisitPalace(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (!human && state::IsStamp(state::HeroKey(PALACE_OF_DREAMS, hero->id, "aiDay"), state::Day()))
        return TRUE;
    if (util::Resource(hero->owner, 6) < 100)
    {
        if (human) H3Messagebox::Show(util::Tr("palace.noGold").c_str());
        return TRUE;
    }
    if (hero->movement <= 300)
    {
        if (human)
            H3Messagebox::Show(util::Tr("palace.noMoves").c_str());
        return TRUE;
    }
    hero->movement -= 300;
    if (!human)
        state::Stamp(state::HeroKey(PALACE_OF_DREAMS, hero->id, "aiDay"), state::Day());
    hero->RecalculateMovement();
    auto &res = P_Main->players[hero->owner].playerResources;
    const std::vector<int> arts = PalaceArtifacts();
    PalaceRandom random(item, hero);
    int plays = 0, wins = 0;
    while (plays < 100 && res.gold >= 100)
    {
        bool artifactGame = false;
        if (human)
        {
            const int pick = ChoosePalaceGame(!arts.empty() && hero->backpackCount < 64);
            if (pick < 0)
                break;
            artifactGame = pick == 1;
        }
        else
        {
            // AI keeps a reserve and limits exposure to ten spins per visit.
            if (plays >= 10 || res.gold < 5000)
                break;
            artifactGame = !arts.empty() && random.Next(0, 1) != 0;
        }
        hero->AddResource(eResource::GOLD, -100);
        ++plays;
        std::array<int,3> reels{};
        std::string result = util::Tr("palace.loss");
        if (!artifactGame)
        {
            const bool mithril = WoGObjectsExtender::IsWoGOptionEnabled(36);
            const SlotResult spin = SpinResource(mithril, random);
            reels = {{spin.a, spin.b, spin.c}};
            if (spin.win)
            {
                ++wins;
                if (spin.a == 6)
                {
                    const int prize = state::Day() * 100 + 4000 + random.Next(0, 100);
                    hero->AddResource(eResource::GOLD, prize);
                    result = util::Tr("palace.winGold", {"value", std::to_string(prize)});
                }
                else if (spin.a >= 0 && spin.a < 7)
                {
                    const int prize = state::Day() / 7 + 4 + random.Next(0, 4);
                    hero->AddResource(spin.a, prize);
                    result = util::Tr("palace.winResource", {"value", std::to_string(prize)}) + " " + P_ResourceName[spin.a];
                }
                else
                {
                    // Mithril is outside H3Resources; util resolves WoG's current eight-player array.
                    const int prize = state::Day() / 7 + 4 + random.Next(0, 4);
                    util::ChangeResource(hero, eResource::MITHRIL, prize);
                    result = util::Tr("palace.winMithril", {"value", std::to_string(prize)});
                }
            }
        }
        else
        {
            const SlotResult spin = SpinArtifact(arts, random);
            reels = {{spin.a, spin.b, spin.c}};
            if (spin.win && util::ValidArtifact(spin.a) && hero->backpackCount < 64)
            {
                ++wins;
                hero->GiveArtifact(H3Artifact(spin.a, -1));
                result = util::Tr("palace.winArtifact") + "\n" + H3Artifact(spin.a, -1).GetName();
            }
        }
        if (human && !ShowPalaceSpin(artifactGame, reels, result, plays < 100 && res.gold >= 100))
            break;
    }
    if (human && plays)
        H3Messagebox::Show(
            util::Tr("palace.summary", {"plays", std::to_string(plays), "wins", std::to_string(wins)}).c_str());
    return TRUE;
}

} // namespace

BOOL VisitTrading(H3Hero *hero, H3MapItem *item, const int subtype, const BOOL human)
{
    if (!hero || hero->owner < 0 || hero->owner >= 8)
        return TRUE;
    switch (subtype)
    {
    case JUNK_MERCHANT:
        return VisitJunkMerchant(hero, human != FALSE);
    case PALACE_OF_DREAMS:
        return VisitPalace(hero, item, human != FALSE);
    default:
        return FALSE;
    }
}

int JunkAiWeight(H3Hero *hero)
{
    if (!hero || util::Resource(hero->owner, 6) > (std::numeric_limits<int>::max)() - 500)
        return 0;
    for (int spell = 0; spell < (std::min)(70, int(H3SpellCount::Get())); ++spell)
        if (util::CountBackpackArtifact(hero, eArtifact::SPELL_SCROLL, spell) > 0)
            return 1000;
    for (const auto &trade : kTrades)
        if (trade.kind != CREATURES && util::CountBackpackArtifact(hero, trade.art) > 0 && CanReceiveTrade(hero, trade))
            return 1000;
    return 0;
}

} // namespace handlers
} // namespace wog
