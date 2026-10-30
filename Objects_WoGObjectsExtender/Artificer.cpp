#include "Artificer.h"
#include "WoGObjectResources.h"
#include "ArtificerDialog.h"
#include "WoGObjectsExtender.h"
#include "WoGObjectPolicy.h"
#include <string>
#include <unordered_map>

namespace wog { namespace artificer {
namespace {
bool visiting = false;
unsigned CurrentDay() { return P_Main->date.CurrentDay() + 1; }

std::string Text(const char *key, const std::vector<std::string> &parameters = {})
{
    return Era::tr((std::string("wog_objects.artificer.") + key).c_str(), parameters);
}
std::string Name() { return Text("name"); }
State ReadState(const H3MapItem *mapItem, const H3Position *position = nullptr)
{
    State state{mapItem->setup};
    return state.At(CurrentDay());
}
void StoreState(H3MapItem *mapItem, H3Hero *hero, H3Position position, State state)
{
    mapItem->setup = state.word;
    auto *tile = P_Main->GetMapItem(position);
    if (tile && tile->objectType == mapItem->objectType && tile->objectSubtype == OBJECT_SUBTYPE)
        tile->setup = state.word;
    // A hero tile's setup contains the hero ID. Update only its underlying object storage.
    if (hero && hero->x == int(position.GetX()) && hero->y == int(position.GetY()) && hero->z == position.GetZ() &&
        hero->objectBelow && hero->objectTypeUnder == mapItem->objectType)
        hero->objectBelowSetup = state.word;
}
void __stdcall AfterLoad(Era::TEvent *) { visiting = false; }
void __stdcall BeforeLoad(Era::TEvent *) { visiting = false; }

class NativeHost final : public Host
{
    H3Hero *hero;
    H3MapItem *mapItem;
    bool human;
    H3Position position;
    mutable std::unordered_map<int, bool> allowed;
    Source placed;
    bool hasPlaced = false;
    const H3Artifact *At(int slot) const
    {
        if (slot < 0 || slot >= SLOT_COUNT) return nullptr;
        return slot < BACKPACK ? &hero->bodyArtifacts[slot] : &hero->backpackArtifacts[slot - BACKPACK];
    }
    static H3Artifact Native(Item item) { return H3Artifact(item.id, item.modifier); }
  public:
    NativeHost(H3Hero *hero, H3MapItem *mapItem, bool human, H3Position position)
        : hero(hero), mapItem(mapItem), human(human), position(position) {}
    bool Allowed(int id) const override
    {
        if (id < 7 || id >= H3ArtifactCount::Get()) return false;
        const auto found = allowed.find(id);
        if (found != allowed.end()) return found->second;
        const bool result = !native::ArtifactBanned(id);
        allowed.emplace(id, result);
        return result;
    }
    int Value(int id) const override
    {
        return id >= 7 && id < H3ArtifactCount::Get() ? (std::max)(0, H3ArtifactSetup::Get()[id].cost) : -1;
    }
    bool Fits(int id, int slot) const override { return hero->CanReplaceArtifact(id, slot) != 0; }
    void RefreshCatalog() override { allowed.clear(); }
    Inventory ReadInventory() const override
    {
        Inventory inventory;
        for (int slot = 0; slot < SLOT_COUNT; ++slot)
        {
            const auto *art = At(slot);
            inventory.items[slot] = {art->id, art->subtype};
        }
        // Locked constituents use special negative IDs; native Fits also checks combination locks.
        for (int slot = 0; slot < BACKPACK; ++slot)
            inventory.locked[slot] = inventory.items[slot].id < -1 ||
                (GearSlot(slot) && hero->blockedArtifacts[slot == 18 ? 13 : slot] != 0);
        return inventory;
    }
    Options ReadOptions() const override
    {
        Options options;
        options.hero = hero->id; options.owner = hero->owner; options.human = human;
        options.boosted = WoGObjectsExtender::IsWoGOptionEnabled(102);
        options.enchanted = WoGObjectsExtender::IsWoGOptionEnabled(71);
        options.mithril = WoGObjectsExtender::IsWoGOptionEnabled(36);
        options.necromancer = hero->hero_class == 8 || hero->hero_class == 9;
        options.difficulty = P_Main->playersInfo.difficulty;
        return options;
    }
    Wallet ReadWallet() const override
    {
        if (hero->owner < 0 || hero->owner > 7) return {};
        return {P_Main->players[hero->owner].playerResources.gold,
                native::Mithril(hero->owner)};
    }
    State ReadState() const override { return artificer::ReadState(mapItem, &position); }
    void SaveState(State state) override { StoreState(mapItem, hero, position, state); }
    bool Remove(Source source) override
    {
        const auto *art = At(source.slot);
        if (!art || Item{art->id, art->subtype} != source.item) return false;
        auto copies = [&]() {
            int count = 0;
            for (int slot = 0; slot < SLOT_COUNT; ++slot)
                count += Item{At(slot)->id, At(slot)->subtype} == source.item;
            return count;
        };
        const int before = copies();
        if (source.slot < BACKPACK)
        {
            hero->RemoveArtifact(source.slot); // Same 0x4E2E40 entry as stdlib UnequipArtFromSlot; triggers AE.
        }
        else hero->RemoveBackpackArtifact(source.slot - BACKPACK);
        return copies() == before - 1;
    }
    bool Put(Item item, int destination) override
    {
        const auto artifact = Native(item);
        if (destination < BACKPACK)
        {
            if (At(destination)->id >= 0 || !hero->CanPlaceArtifact(item.id, destination)) return false;
            hero->GiveArtifact(artifact, destination); // 0x4E2C70, same entry as stdlib EquipArtToSlot.
            hasPlaced = At(destination)->id == item.id;
            if (hasPlaced) placed = {destination, {At(destination)->id, At(destination)->subtype}};
        }
        else
        {
            const int count = hero->backpackCount;
            if (count < 0 || count >= 64) return false;
            hero->GiveBackpackArtifact(artifact);
            hasPlaced = hero->backpackCount == count + 1 && At(BACKPACK + count)->id == item.id;
            if (hasPlaced) placed = {BACKPACK + count, {At(BACKPACK + count)->id, At(BACKPACK + count)->subtype}};
        }
        return hasPlaced;
    }
    bool UndoResult() override { return hasPlaced && Remove(placed); }
    bool Restore(Source source) override
    {
        const auto artifact = Native(source.item);
        if (source.slot < BACKPACK && At(source.slot)->id < 0 && hero->CanPlaceArtifact(source.item.id, source.slot))
        {
            hero->GiveArtifact(artifact, source.slot);
            return *At(source.slot) == artifact;
        }
        const int count = hero->backpackCount;
        if (count < 0 || count >= 64) return false;
        // Restore backpack inputs in ascending order after removing them in descending order.
        hero->GiveBackpackArtifact(artifact, source.slot >= BACKPACK ? source.slot - BACKPACK : -1);
        return hero->backpackCount == count + 1;
    }
    void Pay(Price price) override
    {
        const int owner = hero->owner;
        if (price.gold) hero->AddResource(eResource::GOLD, -price.gold);
        if (price.mithril)
        {
            native::SetMithril(owner, native::Mithril(owner) - price.mithril);
        }
    }
    void SpendMovement(int amount) override
    {
        hero->movement = (std::max)(0, hero->movement - amount);
        hero->RecalculateMovement();
    }
};
bool Active(const H3MapItem *item)
{
    return item && item->objectSubtype == OBJECT_SUBTYPE && policy::Handles(item);
}
} // namespace

void RegisterEvents()
{
    Era::RegisterHandler(BeforeLoad, "OnBeforeLoadGame");
    Era::RegisterHandler(AfterLoad, "OnAfterLoadGame");
}
BOOL Initialize(H3MapItem *mapItem) noexcept
{
    if (!mapItem || mapItem->objectSubtype != OBJECT_SUBTYPE) return FALSE;
    mapItem->setup = State::Fresh(CurrentDay()).word;
    return TRUE;
}
BOOL Visit(H3Hero *hero, H3MapItem *mapItem, H3Position position, BOOL isHuman) noexcept
{
    if (!Active(mapItem)) return FALSE;
    if (!hero || hero->owner < 0 || hero->owner > 7 || visiting) return TRUE;
    struct Guard { Guard() { visiting = true; } ~Guard() { visiting = false; } } guard;
    NativeHost host(hero, mapItem, isHuman != 0, position);
    auto state = host.ReadState();
    if (!isHuman && state.AiVisited()) return TRUE;
    if (!isHuman) state.MarkAiVisit();
    host.SaveState(state);
    if (!isHuman && state.Orders() >= 2) return TRUE;
    if (!isHuman)
    {
        while (host.ReadState().Orders() < 2)
        {
            Offer best;
            bool found = false;
            const auto options = host.ReadOptions();
            const auto inventory = host.ReadInventory();
            const auto wallet = host.ReadWallet();
            for (Service service : {Service::Upgrade, Service::Exchange, Service::Combine})
                for (const auto &offer : BuildOffers(inventory, host, options, host.ReadState(), service))
                    if (wallet.CanPay(offer.price) && (!found || offer.aiScore > best.aiScore)) { best = offer; found = true; }
            if (!found || Commit(host, best) != CommitResult::Success) break;
        }
        return TRUE;
    }
    ShowDialog(host, hero, mapItem);
    return TRUE;
}
BOOL SetHint(H3MapItem *mapItem, const H3Hero *hero, BOOL isRightClick) noexcept
{
    if (!Active(mapItem)) return FALSE;
    const auto state = ReadState(mapItem);
    const std::string hint = isRightClick ? Text("hint", {"orders", std::to_string(2 - state.Orders())}) : Name();
    libc::sprintf(h3_TextBuffer, "%s", hint.c_str());
    return TRUE;
}
BOOL SetAiWeight(H3MapItem *mapItem, H3Hero *hero, H3Position position, int &weight) noexcept
{
    if (!Active(mapItem)) return FALSE;
    weight = 0;
    if (!hero || hero->owner < 0 || hero->owner > 7) return TRUE;
    NativeHost host(hero, mapItem, false, position);
    const auto state = host.ReadState();
    if (state.Orders() >= 2 || state.AiVisited()) return TRUE;
    const auto inventory = host.ReadInventory();
    const auto options = host.ReadOptions();
    const auto wallet = host.ReadWallet();
    for (Service service : {Service::Upgrade, Service::Exchange, Service::Combine})
        for (const auto &offer : BuildOffers(inventory, host, options, state, service))
            if (wallet.CanPay(offer.price)) weight = (std::max)(weight, offer.aiScore);
    return TRUE;
}
} }
