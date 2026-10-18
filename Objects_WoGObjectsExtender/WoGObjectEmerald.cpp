#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include "WoGObjectsExtender.h"
#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <vector>

namespace wog { namespace handlers {
namespace {
using namespace objects;

constexpr int RES_MITHRIL = 7;
constexpr int ENTRY_GEMS = 7;
constexpr int DEFAULT_MAX_SPECIES = 12;

enum Improvement : int
{
    IMP_NONE = -1,
    IMP_ATTACK = 2,
    IMP_DEFENCE = 3,
    IMP_DAMAGE = 4,
    IMP_HEALTH = 5,
    IMP_SPEED = 6,
    IMP_GROWTH = 7,
    IMP_SHOTS = 8,
    IMP_SPELLS = 9,
    IMP_SPECIAL = 10
};

struct CreatureSnapshot
{
    H3CreatureInformation info{};
    bool valid = false;
};
std::map<int, CreatureSnapshot> snapshots;

std::string CreatureKey(const int creature, const char *field)
{
    return H3String::Format("wog_objects.native.%d.creature.%d.%s", EMERALD_TOWER, creature,
                            field ? field : "value").String();
}
int CGet(const int creature, const char *field, const int def = 0)
{
    const int value = state::Get(CreatureKey(creature, field), def);
    return value;
}
void CSet(const int creature, const char *field, const int value)
{
    state::Set(CreatureKey(creature, field), value);
}

int MaxSpecies()
{
    const int configured = EraJS::readInt("wog_objects.native.emerald.maxSpecies");
    return configured > 0 ? configured : DEFAULT_MAX_SPECIES;
}

void EnsureSnapshot(const int creature)
{
    if (!util::ValidCreature(creature) || snapshots[creature].valid)
        return;
    snapshots[creature].info = P_CreatureInformation[creature];
    snapshots[creature].valid = true;
}

void RestoreSnapshots()
{
    for (auto &it : snapshots)
        if (it.second.valid && util::ValidCreature(it.first))
            P_CreatureInformation[it.first] = it.second.info;
}

void ApplySpecial(H3CreatureInformation &info, const int special)
{
    switch (special)
    {
    case 1: info.flyer = 1; break;
    case 2: info.extendedAttack = 1; break;
    case 3:
        info.undead = 1; info.noMorale = 1; info.alive = 0; break;
    case 4:
        info.alive = 1; info.undead = 0; info.noMorale = 0; break;
    case 5: info.noMorale = 1; break;
    case 6: info.doubleAttack = 1; break;
    case 7: info.attackAllAround = 1; break;
    case 8: info.noRetaliation = 1; break;
    case 9: info.mindImmunity = 1; break;
    case 10: info.fireImmunity = 1; break;
    case 11: info.noMeleePenalty = 1; break;
    }
}

void ApplyPersisted(const int creature)
{
    if (!util::ValidCreature(creature) || CGet(creature, "enhancements") <= 0)
        return;
    EnsureSnapshot(creature);
    auto &info = P_CreatureInformation[creature];
    const auto &base = snapshots[creature].info;

    info.attack = base.attack + CGet(creature, "attackCount") * 5;
    info.defence = base.defence + CGet(creature, "defenceCount") * 5;
    info.damageLow = base.damageLow;
    info.damageHigh = base.damageHigh;
    info.hitPoints = base.hitPoints;
    info.speed = base.speed;
    info.grow = base.grow;
    // ERM calculates percentage bonuses from the creature's current value at each visit; replay them in order
    // from the snapshot so repeated 20/25/50% upgrades compound and loading a save stays idempotent.
    for (int i = 0; i < CGet(creature, "damageCount"); ++i)
    {
        const int step = (std::max)(1, info.damageHigh / 5);
        info.damageLow += step; info.damageHigh += step;
    }
    for (int i = 0; i < CGet(creature, "healthCount"); ++i)
        info.hitPoints += (std::max)(1, info.hitPoints / 5);
    for (int i = 0; i < CGet(creature, "speedCount"); ++i)
        info.speed += (std::max)(1, info.speed / 4);
    for (int i = 0; i < CGet(creature, "growthCount"); ++i)
        info.grow += (std::max)(1, info.grow / 2);
    if (CGet(creature, "shotsBought"))
        info.numberShots = (std::max)(24, base.numberShots);
    info.spellCharges = base.spellCharges + CGet(creature, "spellCount");

    const int specialMask = CGet(creature, "specialMask");
    for (int special = 1; special <= 11; ++special)
        if (specialMask & (1 << special))
            ApplySpecial(info, special);

    int ai = base.aiValue;
    for (int i = 0; i < CGet(creature, "ai5"); ++i)
        ai = static_cast<int>(static_cast<long long>(ai) * 105 / 100);
    for (int i = 0; i < CGet(creature, "ai10"); ++i)
        ai = static_cast<int>(static_cast<long long>(ai) * 110 / 100);
    info.aiValue = (std::max)(base.aiValue, ai);
}

void RestoreAndReplay()
{
    RestoreSnapshots();
    for (int i = 0; i < H3CreatureCount::Get(); ++i)
        if (CGet(i, "enhancements") > 0)
            ApplyPersisted(i);
}

int EnhancedSpeciesCount()
{
    int count = 0;
    for (int i = 0; i < H3CreatureCount::Get(); ++i)
        if (CGet(i, "enhancements") > 0)
            ++count;
    return count;
}

std::vector<int> ArmySpecies(const H3Hero *hero)
{
    std::vector<int> result;
    if (!hero)
        return result;
    for (int i = 0; i < 7; ++i)
    {
        const int cr = hero->army.type[i];
        if (!util::ValidCreature(cr) || hero->army.count[i] <= 0)
            continue;
        if (cr >= creature::FIRST_EMISSARY && cr <= creature::LAST_EMISSARY)
            continue;
        if (std::find(result.begin(), result.end(), cr) == result.end())
            result.push_back(cr);
    }
    return result;
}

int ChooseCreature(H3Hero *hero)
{
    const auto list = ArmySpecies(hero);
    H3PictureVector pics;
    for (int cr : list)
        pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(cr), 1));
    const int selected = util::ChoosePictures(util::Tr("emerald.chooseCreature").c_str(), pics);
    return selected >= 0 && selected < static_cast<int>(list.size()) ? list[selected] : -1;
}

int ChooseAiCreature(H3Hero *hero)
{
    int best = -1;
    long long bestValue = -1;
    for (int i = 0; hero && i < 7; ++i)
    {
        const int cr = hero->army.type[i];
        if (!util::ValidCreature(cr) || hero->army.count[i] <= 0 ||
            (cr >= creature::FIRST_EMISSARY && cr <= creature::LAST_EMISSARY))
            continue;
        const long long value = static_cast<long long>(P_CreatureInformation[cr].aiValue) * hero->army.count[i];
        if (value > bestValue)
        {
            bestValue = value;
            best = cr;
        }
    }
    return best;
}

bool MithrilEnabled() { return WoGObjectsExtender::IsWoGOptionEnabled(36); }

std::vector<int> AvailableSpecials(const int creatureId, const bool human = true)
{
    std::vector<int> result;
    if (!util::ValidCreature(creatureId))
        return result;
    const auto &c = P_CreatureInformation[creatureId];
    const int mask = CGet(creatureId, "specialMask");
    auto add = [&](int id, bool allowed) { if (allowed && !(mask & (1 << id))) result.push_back(id); };
    const bool teleporter = creatureId == eCreature::DEVIL || creatureId == eCreature::ARCH_DEVIL ||
                            creatureId == creature::HELL_BARON;
    const bool drainsLife = creatureId == eCreature::VAMPIRE_LORD || creatureId == creature::GHOST ||
                            creatureId == creature::BLOOD_DRAGON;
    add(1, !c.flyer && !teleporter);
    add(2, !c.extendedAttack && !c.attackAllAround);
    if (!CGet(creatureId, "aliveUndeadChosen"))
    {
        add(3, human && !c.undead); // Original script intentionally excludes Undead/Living upgrades for AI.
        add(4, human && !c.alive);
    }
    if (!CGet(creatureId, "noMoraleChosen"))
        add(5, !c.noMorale);
    add(6, !c.doubleAttack);
    add(7, !c.attackAllAround && !c.extendedAttack && !drainsLife);
    add(8, !c.noRetaliation);
    add(9, !c.mindImmunity);
    add(10, !c.fireImmunity);
    add(11, c.shooter && !c.noMeleePenalty);
    return result;
}

int SpecialCost(const int special)
{
    static const int costs[12] = {0, 15, 15, 10, 10, 5, 25, 30, 20, 10, 10, 10};
    return special >= 1 && special <= 11 ? costs[special] : 0;
}

int ChooseSpecial(const int creatureId, const bool human)
{
    auto available = AvailableSpecials(creatureId, human);
    if (available.empty())
        return -1;
    if (!human)
        return available[util::Random(0, static_cast<int>(available.size()) - 1)];

    // Page through abilities with the same selectable-picture mechanism as other native dialogs.
    for (size_t first = 0; first < available.size(); first += 8)
    {
        H3PictureVector pics;
        const size_t last = (std::min)(available.size(), first + 8);
        for (size_t i = first; i < last; ++i)
            pics.Add(H3PictureCategories::Resource(static_cast<eResource>(eResource::GOLD), SpecialCost(available[i])));
        const int index = util::ChoosePictures(util::Tr("emerald.chooseSpecial").c_str(), pics);
        if (index >= 0 && first + static_cast<size_t>(index) < last)
            return available[first + index];
        if (last < available.size() && !H3Messagebox::Choice(util::Tr("common.more").c_str()))
            break;
    }
    return -1;
}

int SpellCost(const H3CreatureInformation &c)
{
    const int charges = c.spellCharges;
    if (charges <= 0 || charges > 12)
        return 0;
    const int level = c.level;
    if (level <= 3) return (std::max)(2, (std::min)(5, 6 - charges));
    if (level == 4) return (std::max)(5, (std::min)(10, 11 - charges));
    if (level == 5) return (std::max)(10, (std::min)(15, 16 - charges));
    return (std::max)(15, (std::min)(20, 21 - charges));
}

int ChooseImprovement(const int creatureId, const bool human, int &special)
{
    special = -1;
    if (!human)
    {
        std::vector<int> options = {IMP_ATTACK, IMP_DEFENCE, IMP_DAMAGE, IMP_HEALTH, IMP_SPEED};
        if (creatureId != creature::GHOST) options.push_back(IMP_GROWTH);
        const auto &c = P_CreatureInformation[creatureId];
        if (c.numberShots > 0 && c.numberShots < 24) options.push_back(IMP_SHOTS);
        if (MithrilEnabled() && SpellCost(c) > 0) options.push_back(IMP_SPELLS);
        if (MithrilEnabled() && !AvailableSpecials(creatureId, false).empty()) options.push_back(IMP_SPECIAL);
        int choice = options[util::Random(0, static_cast<int>(options.size()) - 1)];
        if (choice == IMP_SPECIAL)
        {
            special = ChooseSpecial(creatureId, false);
            if (special < 0) choice = IMP_ATTACK;
        }
        return choice;
    }

    const auto &c = P_CreatureInformation[creatureId];
    std::vector<int> options;
    H3PictureVector pics;
    auto add = [&](int imp, eResource res, int amount) { options.push_back(imp); pics.Add(H3PictureCategories::Resource(res, amount)); };
    add(IMP_ATTACK, eResource::SULFUR, 15);
    add(IMP_DEFENCE, eResource::ORE, 30);
    add(IMP_DAMAGE, eResource::GEMS, 25);
    add(IMP_HEALTH, eResource::CRYSTAL, 25);
    add(IMP_SPEED, eResource::MERCURY, 20);
    if (creatureId != creature::GHOST) add(IMP_GROWTH, eResource::WOOD, 75);
    if (c.numberShots > 0 && c.numberShots < 24) add(IMP_SHOTS, eResource::GOLD, 1000);
    if (MithrilEnabled() && SpellCost(c) > 0) add(IMP_SPELLS, eResource::GOLD, SpellCost(c));

    const int index = util::ChoosePictures(util::Tr("emerald.chooseImprovement").c_str(), pics);
    if (index >= 0 && index < static_cast<int>(options.size()))
        return options[index];
    if (MithrilEnabled() && !AvailableSpecials(creatureId).empty() && H3Messagebox::Choice(util::Tr("emerald.specialQuestion").c_str()))
    {
        special = ChooseSpecial(creatureId, true);
        if (special > 0)
            return IMP_SPECIAL;
    }
    return IMP_NONE;
}

int CountCreature(const H3Hero *hero, const int creatureId)
{
    int total = 0;
    for (int i = 0; hero && i < 7; ++i)
        if (hero->army.type[i] == creatureId && hero->army.count[i] > 0)
            total += hero->army.count[i];
    return total;
}

void RemoveCreature(H3Hero *hero, const int creatureId, int amount)
{
    for (int i = 0; hero && i < 7 && amount > 0; ++i)
    {
        if (hero->army.type[i] != creatureId || hero->army.count[i] <= 0)
            continue;
        const int take = (std::min)(amount, hero->army.count[i]);
        hero->army.count[i] -= take;
        amount -= take;
        if (hero->army.count[i] <= 0)
        {
            hero->army.count[i] = 0;
            hero->army.type[i] = -1;
        }
    }
}

struct Price
{
    int resource = -1;
    int amount = 0;
    int extraWoodOre = 0;
    int extraRare = 0;
    int sacrifice = 0;
};

Price PriceFor(const int creatureId, const int improvement, const int special)
{
    Price p;
    switch (improvement)
    {
    case IMP_ATTACK: p.resource = eResource::SULFUR; p.amount = 15; break;
    case IMP_DEFENCE: p.resource = eResource::ORE; p.amount = 30; break;
    case IMP_DAMAGE: p.resource = eResource::GEMS; p.amount = 25; break;
    case IMP_HEALTH: p.resource = eResource::CRYSTAL; p.amount = 25; break;
    case IMP_SPEED: p.resource = eResource::MERCURY; p.amount = 20; break;
    case IMP_GROWTH: p.resource = eResource::WOOD; p.amount = 75; break;
    case IMP_SHOTS: p.resource = eResource::GOLD; p.amount = 1000; break;
    case IMP_SPELLS: p.resource = RES_MITHRIL; p.amount = SpellCost(P_CreatureInformation[creatureId]); break;
    case IMP_SPECIAL: p.resource = RES_MITHRIL; p.amount = SpecialCost(special); break;
    }
    const int enhancements = CGet(creatureId, "enhancements");
    if (enhancements > 0)
    {
        p.extraWoodOre = enhancements * 20;
        p.extraRare = enhancements * 10;
    }
    const int growth = (std::max)(1, P_CreatureInformation[creatureId].grow);
    p.sacrifice = growth + (std::max)(1, growth / 2);
    return p;
}

bool CanPay(const H3Hero *hero, const Price &p)
{
    if (!hero || hero->owner < 0 || hero->owner > 7 || p.resource < 0)
        return false;
    const auto &r = P_Main->players[hero->owner].playerResources;
    if (p.resource <= 6 && r.asArray[p.resource] < p.amount)
        return false;
    if (p.resource == RES_MITHRIL && IntAt(0x27F9A00 + hero->owner * 4) < p.amount)
        return false;
    if (p.extraWoodOre || p.extraRare)
    {
        const int need[6] = {p.extraWoodOre, p.extraRare, p.extraWoodOre, p.extraRare, p.extraRare, p.extraRare};
        for (int i = 0; i < 6; ++i)
        {
            int available = r.asArray[i];
            if (i == p.resource)
                available -= p.amount;
            if (available < need[i])
                return false;
        }
    }
    return true;
}

void Pay(H3Hero *hero, const Price &p)
{
    hero->AddResource(p.resource, -p.amount);
    if (p.extraWoodOre || p.extraRare)
    {
        hero->AddResource(eResource::WOOD, -p.extraWoodOre);
        hero->AddResource(eResource::ORE, -p.extraWoodOre);
        hero->AddResource(eResource::MERCURY, -p.extraRare);
        hero->AddResource(eResource::SULFUR, -p.extraRare);
        hero->AddResource(eResource::CRYSTAL, -p.extraRare);
        hero->AddResource(eResource::GEMS, -p.extraRare);
    }
}

void RecordImprovement(const int creatureId, const int improvement, const int special)
{
    EnsureSnapshot(creatureId);
    switch (improvement)
    {
    case IMP_ATTACK: CSet(creatureId, "attackCount", CGet(creatureId, "attackCount") + 1); break;
    case IMP_DEFENCE: CSet(creatureId, "defenceCount", CGet(creatureId, "defenceCount") + 1); break;
    case IMP_DAMAGE: CSet(creatureId, "damageCount", CGet(creatureId, "damageCount") + 1); break;
    case IMP_HEALTH: CSet(creatureId, "healthCount", CGet(creatureId, "healthCount") + 1); break;
    case IMP_SPEED: CSet(creatureId, "speedCount", CGet(creatureId, "speedCount") + 1); break;
    case IMP_GROWTH: CSet(creatureId, "growthCount", CGet(creatureId, "growthCount") + 1); break;
    case IMP_SHOTS: CSet(creatureId, "shotsBought", 1); break;
    case IMP_SPELLS: CSet(creatureId, "spellCount", CGet(creatureId, "spellCount") + 1); break;
    case IMP_SPECIAL:
        CSet(creatureId, "specialMask", CGet(creatureId, "specialMask") | (1 << special));
        if (special == 3 || special == 4) CSet(creatureId, "aliveUndeadChosen", 1);
        if (special == 5) CSet(creatureId, "noMoraleChosen", 1);
        break;
    }
    const bool normalAi5 = improvement >= IMP_ATTACK && improvement <= IMP_SPELLS &&
                           improvement != IMP_GROWTH && improvement != IMP_SHOTS;
    if (normalAi5)
        CSet(creatureId, "ai5", CGet(creatureId, "ai5") + 1);
    if (improvement == IMP_SPECIAL)
    {
        if ((special >= 3 && special <= 5) || (special >= 9 && special <= 11))
            CSet(creatureId, "ai5", CGet(creatureId, "ai5") + 1);
        else
            CSet(creatureId, "ai10", CGet(creatureId, "ai10") + 1);
    }
    CSet(creatureId, "enhancements", CGet(creatureId, "enhancements") + 1);
    ApplyPersisted(creatureId);
}

BOOL VisitEmerald(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (!hero || !item)
        return TRUE;
    const std::string monthKey = state::ObjectKey(item, EMERALD_TOWER, "visitedMonth", hero->owner);
    if (state::IsStamp(monthKey, state::Month()))
    {
        if (human) H3Messagebox::Show(util::Tr("emerald.visited").c_str());
        return TRUE;
    }

    auto &resources = P_Main->players[hero->owner].playerResources;
    bool paidEntry = false;
    if (human)
    {
        if (!H3Messagebox::Choice(util::Tr("emerald.appointment").c_str(),
                                  H3PictureCategories::Resource(eResource::GEMS, ENTRY_GEMS)))
            return TRUE;
        if (resources.gems < ENTRY_GEMS)
        {
            H3Messagebox::Show(util::Tr("emerald.noGems").c_str());
            return TRUE;
        }
        hero->AddResource(eResource::GEMS, -ENTRY_GEMS);
        paidEntry = true;
    }

    const int creatureId = human ? ChooseCreature(hero) : ChooseAiCreature(hero);
    if (!util::ValidCreature(creatureId))
    {
        if (paidEntry) hero->AddResource(eResource::GEMS, 6);
        if (human) H3Messagebox::Show(util::Tr("emerald.noCreature").c_str());
        return TRUE;
    }
    if (CGet(creatureId, "enhancements") == 0 && EnhancedSpeciesCount() >= MaxSpecies())
    {
        if (paidEntry) hero->AddResource(eResource::GEMS, 6);
        if (human) H3Messagebox::Show(util::Tr("emerald.speciesLimit", {"max", std::to_string(MaxSpecies())}).c_str());
        return TRUE;
    }

    int special = -1;
    const int improvement = ChooseImprovement(creatureId, human, special);
    if (improvement == IMP_NONE)
    {
        if (paidEntry) hero->AddResource(eResource::GEMS, 6);
        return TRUE;
    }

    if (human)
    {
        const Price price = PriceFor(creatureId, improvement, special);
        if (!CanPay(hero, price) || CountCreature(hero, creatureId) < price.sacrifice)
        {
            hero->AddResource(eResource::GEMS, 6);
            H3Messagebox::Show(util::Tr("emerald.cannotPay", {"creatures", std::to_string(price.sacrifice)}).c_str());
            return TRUE;
        }
        if (!H3Messagebox::Choice(util::Tr("emerald.confirmSacrifice", {"creatures", std::to_string(price.sacrifice)}).c_str(),
                                  H3PictureCategories::Creature(static_cast<eCreature>(creatureId), price.sacrifice)))
        {
            hero->AddResource(eResource::GEMS, 6);
            return TRUE;
        }
        Pay(hero, price);
        RemoveCreature(hero, creatureId, price.sacrifice);
    }

    RecordImprovement(creatureId, improvement, special);
    state::Stamp(monthKey, state::Month());
    if (paidEntry && improvement == IMP_SHOTS)
        hero->AddResource(eResource::GEMS, 6); // Matches the original special refund for the 24-shot purchase.
    if (human)
        H3Messagebox::Show(util::Tr("emerald.done").c_str(), H3PictureCategories::Creature(static_cast<eCreature>(creatureId), 1));
    return TRUE;
}

void __stdcall OnEmeraldLoad(Era::TEvent *) { RestoreAndReplay(); }
void __stdcall OnEmeraldLeave(Era::TEvent *) { RestoreSnapshots(); }

} // namespace

BOOL VisitEmeraldObject(H3Hero *hero, H3MapItem *item, H3Position, BOOL human) noexcept
{
    return item && item->objectSubtype == objects::EMERALD_TOWER ? VisitEmerald(hero, item, human != FALSE) : FALSE;
}

int EmeraldAiWeight(H3MapItem *item, H3Hero *hero) noexcept
{
    if (!item || !hero || item->objectSubtype != objects::EMERALD_TOWER)
        return 0;
    if (state::ObjectPlayerPeriod(item, objects::EMERALD_TOWER, hero->owner, "visitedMonth", state::Month()))
        return 0;
    return ChooseAiCreature(hero) >= 0 ? 1600 : 0;
}

void RegisterEmeraldEvents() noexcept
{
    Era::RegisterHandler(OnEmeraldLoad, "OnAfterLoadGame");
    Era::RegisterHandler(OnEmeraldLeave, "OnGameLeave");
    Era::RegisterHandler(OnEmeraldLeave, "OnGameLeft");
}

}} // namespace wog::handlers
