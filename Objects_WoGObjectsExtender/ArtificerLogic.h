#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace wog { namespace artificer {
constexpr int OBJECT_SUBTYPE = 52;
constexpr int WOG_OPTION = 26;
constexpr const char *LEGACY_HANDLER_REGISTRY_KEY = "wog_objects.artificer.handlesLegacy";
constexpr int SLOT_COUNT = 83;
constexpr int BACKPACK = 19;
constexpr int MOVEMENT_COST = 100;
constexpr int AI_SCOUTING_WEIGHT = 50;
enum class Service { Upgrade, Exchange, Combine, SellGold, SellMithril };

struct Item
{
    int id = -1;
    int modifier = -1;
    bool operator==(const Item &other) const { return id == other.id && modifier == other.modifier; }
    bool operator!=(const Item &other) const { return !(*this == other); }
};
struct Source { int slot = -1; Item item; };
struct Price
{
    int gold = 0, mithril = 0;
    bool operator==(const Price &other) const { return gold == other.gold && mithril == other.mithril; }
};
struct Wallet
{
    int gold = 0, mithril = 0;
    bool CanPay(Price price) const
    {
        const int64_t remainingGold = int64_t(gold) - price.gold;
        const int64_t remainingMithril = int64_t(mithril) - price.mithril;
        return remainingGold >= 0 && remainingMithril >= 0 &&
               remainingGold <= (std::numeric_limits<int>::max)() &&
               remainingMithril <= (std::numeric_limits<int>::max)();
    }
};

// Stable save format in H3MapItem::setup; no compiler-specific bit fields.
struct State
{
    uint32_t word = 0;
    static State Fresh(unsigned day) { return {0xC0000000u | (day & 0xFFFFu)}; } // owner 8, initialized
    bool Valid() const { return (word & 0x80000000u) && ((word >> 27) & 15u) <= 8 && Orders() <= 2; }
    unsigned Day() const { return word & 0xFFFFu; }
    int Orders() const { return (word >> 16) & 3u; }
    bool AiVisited() const { return (word & (1u << 18)) != 0; }
    unsigned Visitors() const { return (word >> 19) & 255u; }
    State At(unsigned day) const
    {
        if (!Valid()) return Fresh(day);
        State result = *this;
        if (Day() != (day & 0xFFFFu)) result.word &= ~(3u << 16);
        if ((Day() ? Day() - 1 : 0) / 7 != (day ? day - 1 : 0) / 7) result.word &= ~(1u << 18);
        result.word = (result.word & ~0xFFFFu) | (day & 0xFFFFu);
        return result;
    }
    void MarkVisitor(int owner, bool ai)
    {
        if (owner >= 0 && owner < 8) word |= 1u << (19 + owner);
        if (ai) word |= 1u << 18;
    }
    void CompleteOrder() { word = (word & ~(3u << 16)) | (unsigned(Orders() + 1) << 16); }
};
static_assert(sizeof(State) == 4, "Artificer state must fit mapItem->setup");

struct Options
{
    bool human = true, boosted = false, enchanted = false, mithril = false, necromancer = false;
    int difficulty = 1, hero = -1, owner = -1;
};
struct Inventory
{
    std::array<Item, SLOT_COUNT> items{};
    std::array<bool, BACKPACK> locked{};
    int Count(int id) const
    {
        int result = 0;
        for (const auto &item : items) result += item.id == id;
        return result;
    }
};
struct Catalog
{
    virtual ~Catalog() = default;
    virtual bool Allowed(int id) const = 0;
    virtual int Value(int id) const = 0;
    virtual bool Fits(int id, int slot) const = 0;
};
struct Recipe
{
    Item result;
    std::array<int, 6> parts{};
    int count = 0;
    Price price;
};
inline Recipe UpgradeRecipe(int art, const Options &options)
{
    Recipe recipe;
    int target = -1;
    for (int first : {7, 13, 19, 25, 31}) if (art >= first && art < first + 5) target = art + 1;
    for (int first : {37, 41}) if (art >= first && art < first + 3) target = art + 1;
    for (int first : {54, 57, 60, 63, 66, 73, 76}) if (art >= first && art < first + 2) target = art + 1;
    if (art >= 118 && art <= 121) target = art + 1;
    switch (art)
    {
    case 45: target = 108; break;
    case 69: case 97: target = 99; break;
    case 70: target = 98; break;
    case 71: target = 123; break;
    case 83: target = 126; break;
    case 94: case 95: target = 96; break;
    case 116: target = 115; break;
    case 117: target = 116; break;
    }
    if (target < 0) return recipe;
    recipe.result.id = target;
    recipe.parts[0] = art; recipe.count = 1;
    switch (art)
    {
    case 7: case 13: case 19: case 25: case 76: case 118: recipe.price.gold = 1000; break;
    case 8: case 14: case 20: case 26: case 54: case 57: case 60: case 63: case 66: case 73: case 119: recipe.price.gold = 1500; break;
    case 9: case 15: case 21: case 27: case 31: case 37: case 41: case 77: case 120: recipe.price.gold = 2000; break;
    case 10: case 16: case 22: case 28: case 117: case 121: recipe.price.gold = 2500; break;
    case 38: case 42: case 55: case 58: case 61: case 64: case 67: case 74: case 116: recipe.price.gold = 3000; break;
    case 32: case 39: case 43: case 94: case 95: recipe.price.gold = 4000; break;
    case 45: case 71: recipe.price.gold = 5000; break;
    case 33: recipe.price.gold = 6000; break;
    case 11: case 17: case 23: case 29: case 70: recipe.price.gold = 7500; break;
    case 34: case 69: case 97: recipe.price.gold = 8000; break;
    case 35: case 83: recipe.price.gold = 10000; break;
    }
    switch (art)
    {
    case 11: case 17: case 23: case 29: case 45: recipe.price.mithril = 1; break;
    case 31: recipe.price.mithril = 2; break;
    case 70: recipe.price.mithril = 3; break;
    case 32: recipe.price.mithril = 4; break;
    case 83: recipe.price.mithril = 5; break;
    case 33: recipe.price.mithril = 6; break;
    case 34: recipe.price.mithril = 8; break;
    case 35: recipe.price.mithril = 10; break;
    }
    if (options.enchanted && (art == 76 || art == 77)) recipe.price.gold = art == 76 ? 1500 : 3000;
    if (options.boosted && (art == 57 || art == 58)) recipe.price.gold += 1000;
    return recipe;
}
inline std::vector<Recipe> ExchangeRecipes(int art, const Options &options)
{
    if (art < 7) return {};
    static const int groups[][4] = {{46,47,48,-1},{49,50,51,-1},{52,53,-1,-1},{79,80,81,82},
        {84,85,-1,-1},{86,87,88,89},{94,95,-1,-1},{103,104,-1,-1},{109,110,111,113},{112,114,-1,-1}};
    static const int prices[] = {1000,1000,1000,2500,1500,10000,2500,1000,2500,1500};
    std::vector<Recipe> result;
    for (int group = 0; group < 10; ++group)
    {
        if (std::find(groups[group], groups[group] + 4, art) == groups[group] + 4) continue;
        for (int target : groups[group])
        {
            if (target < 0 || target == art) continue;
            Recipe recipe;
            recipe.result.id = target; recipe.parts[0] = art; recipe.count = 1;
            recipe.price.gold = prices[group];
            if (group == 3) recipe.price.mithril = options.enchanted ? 3 : 1;
            if (group == 5) recipe.price.mithril = 10;
            if (group == 7) { recipe.price.mithril = 1; if (options.enchanted) recipe.price.gold = 7500; }
            result.push_back(recipe);
        }
        break;
    }
    return result;
}
inline Recipe CombinationRecipe(int number, const Options &options)
{
    Recipe result;
    if (number == 1) { result.result.id = 153; result.parts = {{37,45,0,0,0,0}}; result.count = 2; result.price = {options.enchanted ? 7500 : 5000, 5}; }
    if (number == 2) { result.result.id = 108; result.parts = {{46,47,48,49,50,51}}; result.count = 6; result.price = {2500,0}; }
    if (number == 3) { result.result.id = 93; result.parts = {{79,80,81,82,0,0}}; result.count = 4; result.price = {10000,10}; }
    return result;
}
inline Price Quote(Price base, const Options &options, State state)
{
    const int multiplier = state.Orders() + 1;
    base.gold *= multiplier;
    base.mithril *= multiplier;
    if (!options.human) { base.gold /= (std::max)(1, options.difficulty); base.mithril = 0; }
    return base;
}
inline bool GearSlot(int slot) { return (slot >= 0 && slot <= 12) || slot == 18; }
struct Offer
{
    Service service = Service::Upgrade;
    std::array<Source, 6> sources{};
    int count = 0, destination = -1;
    Item result;
    Price price;
    unsigned day = 0;
    int orders = 0, hero = -1, owner = -1, aiScore = 0;
    bool Sale() const { return service == Service::SellGold || service == Service::SellMithril; }
    bool Same(const Offer &other) const
    {
        if (service != other.service || count != other.count || destination != other.destination ||
            result != other.result || !(price == other.price) || day != other.day || orders != other.orders ||
            hero != other.hero || owner != other.owner) return false;
        for (int i = 0; i < count; ++i)
            if (sources[i].slot != other.sources[i].slot || sources[i].item != other.sources[i].item) return false;
        return true;
    }
};
inline int Destination(const Inventory &inventory, const Catalog &catalog, const Offer &offer)
{
    auto removed = [&](int slot) {
        for (int i = 0; i < offer.count; ++i) if (offer.sources[i].slot == slot) return true;
        return false;
    };
    // Keep the source slot when possible, including the right/left ring and all miscellaneous slots.
    for (int i = 0; i < offer.count; ++i)
    {
        const int slot = offer.sources[i].slot;
        if (GearSlot(slot) && !inventory.locked[slot] && catalog.Fits(offer.result.id, slot)) return slot;
    }
    for (int slot = 0; slot < BACKPACK; ++slot)
        if (GearSlot(slot) && !inventory.locked[slot] && inventory.items[slot].id < 0 && catalog.Fits(offer.result.id, slot)) return slot;
    // Backpack insertion appends after removing inputs; use a stable sentinel, not a shifting slot.
    for (int slot = BACKPACK; slot < SLOT_COUNT; ++slot)
        if (inventory.items[slot].id < 0 || removed(slot)) return BACKPACK;
    return -1;
}
inline std::vector<Offer> BuildOffers(const Inventory &inventory, const Catalog &catalog,
                                    const Options &options, State state, Service service)
{
    std::vector<Offer> offers;
    const bool sale = service == Service::SellGold || service == Service::SellMithril;
    if ((!sale && state.Orders() >= 2) || options.owner < 0 || options.owner > 7) return offers;
    auto add = [&](const Recipe &recipe, int sourceSlot) {
        if (recipe.result.id < 0) return;
        Offer offer;
        offer.service = service; offer.result = recipe.result; offer.price = Quote(recipe.price, options, state);
        offer.hero = options.hero; offer.owner = options.owner; offer.day = state.Day(); offer.orders = state.Orders();
        for (int part = 0; part < recipe.count; ++part)
        {
            int found = -1;
            for (int slot = 0; slot < SLOT_COUNT; ++slot)
            {
                if (sourceSlot >= 0 && slot != sourceSlot) continue;
                if (slot < BACKPACK && (!GearSlot(slot) || inventory.locked[slot])) continue;
                if (inventory.items[slot].id != recipe.parts[part]) continue;
                bool used = false;
                for (int i = 0; i < offer.count; ++i) used |= offer.sources[i].slot == slot;
                if (!used) { found = slot; break; }
            }
            if (found < 0) return;
            offer.sources[offer.count++] = {found, inventory.items[found]};
        }
        if (!catalog.Allowed(recipe.result.id)) return;
        offer.destination = Destination(inventory, catalog, offer);
        if (offer.destination < 0) return;
        if (!options.human)
        {
            if (offer.destination >= BACKPACK || (options.necromancer && recipe.result.id == 108)) return;
            if (service == Service::Exchange)
            {
                const int source = recipe.parts[0];
                if (inventory.Count(source) < 2 || inventory.Count(recipe.result.id) ||
                    (source >= 109 && source <= 114) || source == 84) return;
                offer.aiScore = (std::max)(1, catalog.Value(source) / 4);
            }
            else
            {
                int64_t wornValue = 0;
                for (int i = 0; i < offer.count; ++i)
                    if (offer.sources[i].slot < BACKPACK) wornValue += catalog.Value(offer.sources[i].item.id);
                const int64_t gain = int64_t(catalog.Value(recipe.result.id)) - wornValue;
                if (gain <= 0) return;
                offer.aiScore = int((std::min)(gain, int64_t((std::numeric_limits<int>::max)())));
            }
        }
        offers.push_back(offer);
    };
    if (service == Service::Combine)
        for (int recipe = 1; recipe <= 3; ++recipe) add(CombinationRecipe(recipe, options), -1);
    else if (service == Service::Upgrade || service == Service::Exchange)
    {
        for (int slot = 0; slot < BACKPACK; ++slot)
        {
            if (!GearSlot(slot) || inventory.locked[slot]) continue;
            if (service == Service::Upgrade) add(UpgradeRecipe(inventory.items[slot].id, options), slot);
            else for (const auto &recipe : ExchangeRecipes(inventory.items[slot].id, options)) add(recipe, slot);
        }
    }
    else if (options.human && options.mithril)
    {
        for (int slot = BACKPACK; slot < SLOT_COUNT; ++slot)
        {
            const auto &item = inventory.items[slot];
            if (item.id < 7 || item.id >= 1000 || catalog.Value(item.id) < 0) continue;
            Offer offer;
            offer.service = service; offer.sources[0] = {slot, item}; offer.count = 1;
            offer.hero = options.hero; offer.owner = options.owner; offer.day = state.Day(); offer.orders = state.Orders();
            if (service == Service::SellGold) offer.price.gold = -catalog.Value(item.id) / 4;
            else offer.price.mithril = -(catalog.Value(item.id) / 4000 + 1);
            offers.push_back(offer);
        }
    }
    return offers;
}

struct Host : Catalog
{
    virtual Inventory ReadInventory() const = 0;
    virtual Options ReadOptions() const = 0;
    virtual Wallet ReadWallet() const = 0;
    virtual State ReadState() const = 0;
    virtual void SaveState(State state) = 0;
    virtual void RefreshCatalog() = 0;
    virtual bool Remove(Source source) = 0;
    virtual bool Put(Item item, int destination) = 0;
    virtual bool UndoResult() = 0;
    virtual bool Restore(Source source) = 0;
    virtual void Pay(Price price) = 0;
    virtual void SpendMovement(int amount) = 0;
};
enum class CommitResult { Success, Changed, NoResources, InventoryFailure, RecoveryFailure };

struct SaleBatch
{
    std::vector<Offer> offers;
    Price price;
    bool valid = false;
};
inline SaleBatch BuildSaleBatch(const Inventory &inventory, const Catalog &catalog, const Options &options,
                                State state, Service service)
{
    SaleBatch batch;
    if (service != Service::SellGold && service != Service::SellMithril) return batch;
    batch.offers = BuildOffers(inventory, catalog, options, state, service);
    int64_t gold = 0, mithril = 0;
    for (const auto &offer : batch.offers) { gold += offer.price.gold; mithril += offer.price.mithril; }
    // Prices are negative payouts. Never truncate a large modded inventory into a positive charge.
    const int64_t maximum = (std::numeric_limits<int>::max)();
    if (-gold > maximum || -mithril > maximum || batch.offers.empty()) return batch;
    batch.price = {int(gold), int(mithril)};
    batch.valid = true;
    return batch;
}
inline CommitResult CommitSaleBatch(Host &host, const SaleBatch &quoted)
{
    if (!quoted.valid || quoted.offers.empty()) return CommitResult::Changed;
    host.RefreshCatalog();
    const auto options = host.ReadOptions();
    const auto state = host.ReadState();
    const auto current = BuildSaleBatch(host.ReadInventory(), host, options, state, quoted.offers.front().service);
    if (!current.valid || !(current.price == quoted.price) || current.offers.size() != quoted.offers.size())
        return CommitResult::Changed;
    for (size_t i = 0; i < current.offers.size(); ++i)
        if (!current.offers[i].Same(quoted.offers[i])) return CommitResult::Changed;
    if (!host.ReadWallet().CanPay(quoted.price)) return CommitResult::NoResources;
    std::vector<Source> sources;
    for (const auto &offer : current.offers) sources.push_back(offer.sources[0]);
    std::sort(sources.begin(), sources.end(), [](const Source &a, const Source &b) { return a.slot > b.slot; });
    size_t removed = 0;
    auto recover = [&]() {
        bool restored = true;
        while (removed) restored &= host.Restore(sources[--removed]);
        return restored ? CommitResult::InventoryFailure : CommitResult::RecoveryFailure;
    };
    for (; removed < sources.size(); ++removed) if (!host.Remove(sources[removed])) return recover();
    const auto after = host.ReadOptions();
    const auto afterState = host.ReadState();
    if (!host.ReadWallet().CanPay(quoted.price) || after.owner != options.owner || after.hero != options.hero ||
        !after.human || !after.mithril || afterState.Day() != state.Day() || afterState.Orders() != state.Orders())
        return recover();
    host.Pay(quoted.price);
    auto completed = host.ReadState();
    completed.MarkVisitor(options.owner, false);
    host.SaveState(completed);
    return CommitResult::Success;
}
inline CommitResult Commit(Host &host, const Offer &offer)
{
    host.RefreshCatalog();
    const auto state = host.ReadState();
    const auto options = host.ReadOptions();
    const auto current = BuildOffers(host.ReadInventory(), host, options, state, offer.service);
    const auto match = std::find_if(current.begin(), current.end(), [&](const Offer &candidate) { return candidate.Same(offer); });
    if (match == current.end()) return CommitResult::Changed;
    if (!host.ReadWallet().CanPay(offer.price)) return CommitResult::NoResources;
    auto ordered = offer.sources;
    std::sort(ordered.begin(), ordered.begin() + offer.count, [](const Source &a, const Source &b) { return a.slot > b.slot; });
    int removed = 0;
    auto recover = [&]() {
        bool restored = true;
        while (removed) restored &= host.Restore(ordered[--removed]);
        return restored ? CommitResult::InventoryFailure : CommitResult::RecoveryFailure;
    };
    for (; removed < offer.count; ++removed) if (!host.Remove(ordered[removed])) return recover();
    if (!offer.Sale() && !host.Put(offer.result, offer.destination)) return recover();
    // AE handlers can change resources, ownership, bans or the map clock while native inventory methods run.
    host.RefreshCatalog();
    const auto after = host.ReadState();
    const auto ownerAfter = host.ReadOptions();
    if (!host.ReadWallet().CanPay(offer.price) || after.Day() != offer.day || after.Orders() != offer.orders ||
        ownerAfter.owner != offer.owner || ownerAfter.hero != offer.hero ||
        ownerAfter.boosted != options.boosted || ownerAfter.enchanted != options.enchanted ||
        ownerAfter.mithril != options.mithril || ownerAfter.difficulty != options.difficulty ||
        (!offer.Sale() && !host.Allowed(offer.result.id)))
    {
        if (!offer.Sale() && !host.UndoResult()) return CommitResult::RecoveryFailure;
        return recover();
    }
    host.Pay(offer.price);
    auto completed = host.ReadState();
    if (!offer.Sale()) { host.SpendMovement(MOVEMENT_COST); completed.CompleteOrder(); }
    completed.MarkVisitor(options.owner, !options.human);
    host.SaveState(completed);
    return CommitResult::Success;
}
} }
