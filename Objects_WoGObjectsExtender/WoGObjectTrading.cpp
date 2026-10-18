#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include "WoGObjectsExtender.h"
#include <algorithm>
#include <vector>

namespace wog { namespace handlers {
namespace {
using namespace objects;

enum RewardKind { GOLD, RESOURCE, CREATURES, ARTIFACT, PRIMARY, SPELL_POINTS, SPELL, EXPERIENCE, REVEAL };
struct JunkTrade { int art; RewardKind kind; int a; int b; };

const JunkTrade kTrades[] = {
    {7, CREATURES, 15, 50}, {13, CREATURES, 17, 15}, {15, ARTIFACT, 16, 0}, {19, RESOURCE, 5, 5},
    {25, PRIMARY, 2, 1}, {46, GOLD, 1500, 0}, {47, GOLD, 1500, 0}, {48, GOLD, 1500, 0},
    {49, GOLD, 1500, 0}, {50, GOLD, 1500, 0}, {51, GOLD, 1500, 0}, {52, REVEAL, 30, 0},
    {53, GOLD, 1830, 0}, {54, GOLD, 600, 0}, {55, GOLD, 2500, 0}, {56, GOLD, 2000, 0},
    {58, GOLD, 4000, 0}, {63, CREATURES, 5, 15}, {66, RESOURCE, 4, 10}, {67, RESOURCE, 5, 10},
    {68, RESOURCE, 1, 10}, {71, RESOURCE, 0, 15}, {72, CREATURES, creature::SUPREME_ARCHANGEL, 2},
    {73, SPELL_POINTS, 60, 0}, {76, SPELL, 54, 0}, {83, GOLD, 2500, 0}, {84, GOLD, 2000, 0},
    {85, GOLD, 500, 0}, {92, EXPERIENCE, 2000, 0}, {93, ARTIFACT, 79, 0}, {106, SPELL, 17, 0},
    {118, RESOURCE, 2, 15}
};

void RevealRadius(const H3Hero *hero, const int radius)
{
    if (!hero || hero->owner < 0 || hero->owner >= 8) return;
    const int size = H3MapSize::Get();
    for (int y = (std::max)(0, static_cast<int>(hero->y) - radius); y <= (std::min)(size - 1, static_cast<int>(hero->y) + radius); ++y)
        for (int x = (std::max)(0, static_cast<int>(hero->x) - radius); x <= (std::min)(size - 1, static_cast<int>(hero->x) + radius); ++x)
            if ((x - hero->x) * (x - hero->x) + (y - hero->y) * (y - hero->y) <= radius * radius)
                H3TileVision::RevealTile(x, y, hero->z, hero->owner);
    if (P_AdventureManager) P_AdventureManager->FullUpdate();
}

bool CanReceiveTrade(const H3Hero *hero, const JunkTrade &t)
{
    if (t.kind != CREATURES) return true;
    std::vector<std::pair<int, int>> s(1, std::make_pair(t.a, t.b));
    return util::CanAddArmy(hero->army, s);
}
void ApplyTrade(H3Hero *hero, const JunkTrade &t)
{
    switch (t.kind)
    {
    case GOLD: hero->AddResource(eResource::GOLD, t.a); break;
    case RESOURCE: hero->AddResource(t.a, t.b); break;
    case CREATURES:
        util::AddArmy(hero->army, {std::make_pair(t.a, t.b)}); break;
    case ARTIFACT: if (util::ValidArtifact(t.a)) hero->GiveArtifact(H3Artifact(t.a, -1)); break;
    case PRIMARY: if (t.a >= 0 && t.a < 4) hero->primarySkill[t.a] += static_cast<INT8>(t.b); break;
    case SPELL_POINTS: hero->spellPoints = static_cast<INT16>((std::min)(32767, static_cast<int>(hero->spellPoints) + t.a)); break;
    case SPELL:
        if (hero->bodyArtifacts[17].id >= 0 && t.a >= 0 && t.a < (std::min)(70, static_cast<int>(H3SpellCount::Get()))) hero->learnedSpells[t.a] = TRUE;
        break;
    case EXPERIENCE: hero->experience += t.a; break;
    case REVEAL: RevealRadius(hero, t.a); break;
    }
}
std::string TradeRewardText(const JunkTrade &t)
{
    switch (t.kind)
    {
    case GOLD: return util::Tr("junk.rewardGold", {"value", std::to_string(t.a)});
    case RESOURCE: return util::Tr("junk.rewardResource", {"value", std::to_string(t.b), "id", std::to_string(t.a)});
    case CREATURES: return util::Tr("junk.rewardCreatures", {"value", std::to_string(t.b)});
    case ARTIFACT: return util::Tr("junk.rewardArtifact");
    case PRIMARY: return util::Tr("junk.rewardPrimary");
    case SPELL_POINTS: return util::Tr("junk.rewardMana", {"value", std::to_string(t.a)});
    case SPELL: return util::Tr("junk.rewardSpell");
    case EXPERIENCE: return util::Tr("junk.rewardExperience", {"value", std::to_string(t.a)});
    case REVEAL: return util::Tr("junk.rewardReveal", {"value", std::to_string(t.a)});
    default: return std::string();
    }
}
BOOL VisitJunkMerchant(H3Hero *hero, const bool human)
{
    int traded = 0;
    // Spell scrolls are real H3Artifact id 1 + spell subtype in native hero data.
    for (int spell = 0, spellCount = (std::min)(70, static_cast<int>(H3SpellCount::Get())); spell < spellCount; ++spell)
    {
        // The ERM script processes each spell id once, therefore at most one scroll of a given spell per visit.
        if (util::CountBackpackArtifact(hero, 1, spell) <= 0)
            continue;
        const bool accept = !human || H3Messagebox::Choice(util::Tr("junk.scroll", {"spell", std::to_string(spell), "gold", "500"}).c_str(),
            H3PictureCategories::Spell(static_cast<eSpell>(spell)));
        if (accept && util::RemoveBackpackArtifact(hero, 1, spell))
        {
            hero->AddResource(eResource::GOLD, 500);
            ++traded;
        }
    }
    for (const JunkTrade &t : kTrades)
    {
        // Same rule for regular artifacts: the original FU640 checks/removes one copy only.
        if (util::CountBackpackArtifact(hero, t.art) <= 0 || !CanReceiveTrade(hero, t))
            continue;
        // Preserve source AI rule: it refuses rewards consisting of creatures, but accepts other positive trades.
        const bool accept = human ? H3Messagebox::Choice(
                util::Tr("junk.offer", {"reward", TradeRewardText(t)}).c_str(), H3PictureCategories::Artifact(static_cast<eArtifact>(t.art)))
            : t.kind != CREATURES;
        if (accept && util::RemoveBackpackArtifact(hero, t.art))
        {
            ApplyTrade(hero, t);
            ++traded;
        }
    }
    if (human) H3Messagebox::Show(util::Tr(traded ? "junk.done" : "junk.nothing", {"count", std::to_string(traded)}).c_str());
    return TRUE;
}

struct SlotResult { int a, b, c; bool win; };
SlotResult SpinResource(const bool mithril)
{
    const int maxId = mithril ? 7 : 6;
    SlotResult r{util::Random(0, maxId), util::Random(0, maxId), util::Random(0, maxId), false};
    if (util::Random(0, 15) == 0) r.a = r.c;
    if (util::Random(0, 15) == 0) r.b = r.a;
    if (util::Random(0, 15) == 0) r.c = r.b;
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
SlotResult SpinArtifact(const std::vector<int> &pool)
{
    if (pool.empty()) return {-1,-1,-1,false};
    auto pick = [&]() { int id = pool[util::Random(0, static_cast<int>(pool.size()) - 1)];
        if (H3ArtifactSetup::Get()[id].type == eArtifactType::RELIC && util::Random(0, 1) == 0)
            id = pool[util::Random(0, static_cast<int>(pool.size()) - 1)];
        return id; };
    SlotResult r{pick(), pick(), pick(), false};
    int copyChance = 8 - static_cast<int>(pool.size()) / 100;
    copyChance = (std::max)(4, (std::min)(8, copyChance));
    if (util::Random(0, copyChance) == 0) r.a = r.c;
    if (util::Random(0, copyChance) == 0) r.b = r.a;
    if (util::Random(0, copyChance) == 0) r.c = r.b;
    r.win = r.a == r.b && r.b == r.c;
    return r;
}
BOOL VisitPalace(H3Hero *hero, const bool human)
{
    if (hero->movement <= 300)
    {
        if (human) H3Messagebox::Show(util::Tr("palace.noMoves").c_str());
        return TRUE;
    }
    hero->movement -= 300;
    hero->RecalculateMovement();
    auto &res = P_Main->players[hero->owner].playerResources;
    const std::vector<int> arts = PalaceArtifacts();
    int plays = 0, wins = 0;
    while (plays < 100 && res.gold >= 100)
    {
        bool artifactGame = false;
        if (human)
        {
            const auto pick = H3Messagebox::Choose(util::Tr("palace.choose").c_str(),
                H3PictureCategories::Resource(eResource::GOLD, 100),
                H3PictureCategories::Artifact(static_cast<eArtifact>(arts.empty() ? 7 : arts.front())));
            if (pick == H3Messagebox::NONE) break;
            artifactGame = pick == H3Messagebox::RIGHT;
        }
        else
        {
            // AI keeps a reserve and limits exposure to ten spins per visit.
            if (plays >= 10 || res.gold < 5000) break;
            artifactGame = !arts.empty() && util::Random(0, 1) != 0;
        }
        hero->AddResource(eResource::GOLD, -100);
        ++plays;
        if (!artifactGame)
        {
            const bool mithril = WoGObjectsExtender::IsWoGOptionEnabled(36);
            const SlotResult spin = SpinResource(mithril);
            if (spin.win)
            {
                ++wins;
                if (spin.a == 6)
                {
                    const int prize = state::Day() * 100 + 4000 + util::Random(0, 100);
                    hero->AddResource(eResource::GOLD, prize);
                    if (human) H3Messagebox::Show(util::Tr("palace.winGold", {"value", std::to_string(prize)}).c_str());
                }
                else if (spin.a >= 0 && spin.a < 7)
                {
                    const int prize = state::Day() / 7 + 4 + util::Random(0, 4);
                    hero->AddResource(spin.a, prize);
                    if (human) H3Messagebox::Show(util::Tr("palace.winResource", {"value", std::to_string(prize)}).c_str());
                }
                else
                {
                    // Mithril is resource #7 in WoG, outside H3Resources. Use ERA's stable named variable shared with WoG scripts.
                    const int prize = state::Day() / 7 + 4 + util::Random(0, 4);
                    hero->AddResource(7, prize);
                }
            }
        }
        else
        {
            const SlotResult spin = SpinArtifact(arts);
            if (spin.win && util::ValidArtifact(spin.a))
            {
                ++wins; hero->GiveArtifact(H3Artifact(spin.a, -1));
                if (human) H3Messagebox::Show(util::Tr("palace.winArtifact").c_str(), H3PictureCategories::Artifact(static_cast<eArtifact>(spin.a)));
            }
        }
        if (human && !H3Messagebox::Choice(util::Tr("palace.again").c_str())) break;
    }
    if (human && plays) H3Messagebox::Show(util::Tr("palace.summary", {"plays", std::to_string(plays), "wins", std::to_string(wins)}).c_str());
    return TRUE;
}

} // namespace

BOOL VisitTrading(H3Hero *hero, H3MapItem *, const int subtype, const BOOL human)
{
    if (!hero || hero->owner < 0 || hero->owner >= 8) return TRUE;
    switch (subtype)
    {
    case JUNK_MERCHANT: return VisitJunkMerchant(hero, human != FALSE);
    case PALACE_OF_DREAMS: return VisitPalace(hero, human != FALSE);
    default: return FALSE;
    }
}

}} // namespace wog::handlers
