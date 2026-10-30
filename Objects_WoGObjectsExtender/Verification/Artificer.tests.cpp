#include <WoGObjectPolicy.cpp>
#include <Artificer.cpp>
#include "WoGHandlersStubs.h"
#include <WoGObjectsExtender.cpp>
#include <climits>
namespace wog { namespace marketOfTime {
BOOL Visit(H3Hero *, BOOL) noexcept { return TRUE; }
BOOL SetHint(BOOL) noexcept { return TRUE; }
} }
using namespace wog::artificer;
// The real renderer is built in the DLL. This suite drives the same host through a headless UI seam.
static std::function<void(Host &)> dialogAction;
namespace wog { namespace artificer {
void ShowDialog(Host &host, H3Hero *, const H3MapItem *) { if (dialogAction) dialogAction(host); }
} }
static void Work(Host &host, int orders)
{
    while (orders-- && host.ReadState().Orders() < 2)
    {
        const auto offers = BuildOffers(host.ReadInventory(),host,host.ReadOptions(),host.ReadState(),Service::Upgrade);
        if (offers.empty() || Commit(host,offers.front()) != CommitResult::Success) break;
    }
}
static H3Hero hero;
static H3MapItem object;
static H3Position position(10,20,0);
static void Reset(int type = 141)
{
    fakeart::assoc.clear();
    fakeart::main = {}; fakeart::main.tile = &object;
    fakeart::options.fill(0); fakeart::options[26] = fakeart::options[36] = 1;
    fakeart::mithril.fill(100); fakeart::banned.fill(false);
    fakeart::menus.clear(); fakeart::confirmations.clear(); fakeart::messages.clear();
    fakeart::inventoryHook = {}; fakeart::confirmHook = {}; dialogAction = {}; fakeart::backpackHook = {};
    fakeart::failGive = fakeart::failRemove = fakeart::removals = fakeart::equips = 0;
    fakeart::legacyFatigue = 1;
    hero = {}; object = {}; object.objectType = type;
    object.setup = State::Fresh(1).word;
    for (int art = 0; art < 164; ++art)
    {
        auto &meta = fakeart::artifacts[art];
        meta.disabled = false; meta.cost = 1000 + art * 100; meta.position = 9; meta.name = "Artifact " + std::to_string(art);
    }
    for (int art = 7; art <= 12; ++art) fakeart::artifacts[art].position = 4;
    for (int art = 13; art <= 18; ++art) fakeart::artifacts[art].position = 5;
    for (int art = 19; art <= 24; ++art) fakeart::artifacts[art].position = 1;
    for (int art = 25; art <= 30; ++art) fakeart::artifacts[art].position = 6;
    fakeart::artifacts[31].position = 6; fakeart::artifacts[32].position = 8;
    fakeart::artifacts[33].position = 3; fakeart::artifacts[34].position = 5;
    fakeart::artifacts[35].position = 4; fakeart::artifacts[36].position = 1;
    fakeart::artifacts[37].position = 7; fakeart::artifacts[38].position = 4;
    fakeart::artifacts[39].position = 5; fakeart::artifacts[40].position = 6;
    fakeart::artifacts[41].position = 8; fakeart::artifacts[42].position = 2;
    fakeart::artifacts[78].position = 2;
    for (int art : {45,94,95,96,153}) fakeart::artifacts[art].position = 7;
    fakeart::artifacts[108].position = 3; fakeart::artifacts[108].cost = 7000;
    fakeart::artifacts[153].cost = 12000;
    for (int art = 46; art <= 51; ++art) fakeart::artifacts[art].cost = 1000;
    fakeart::artifacts[7].cost = 1000; fakeart::artifacts[8].cost = 2000; fakeart::artifacts[9].cost = 3000;
}
static void Equip(int art, int slot, int modifier = -1) { hero.GiveArtifact(H3Artifact(art,modifier),slot); }
static void Bag(int art, int modifier = -1) { hero.GiveBackpackArtifact(H3Artifact(art,modifier)); }
static std::vector<Offer> Offers(Service service, bool human = true)
{
    NativeHost host(&hero,&object,human,position);
    return BuildOffers(host.ReadInventory(),host,host.ReadOptions(),host.ReadState(),service);
}
static CommitResult Apply(const Offer &offer, bool human = true)
{
    NativeHost host(&hero,&object,human,position);
    return Commit(host,offer);
}
static void DumpRules()
{
    for (int art = -1; art <= 200; ++art) for (int boosted = 0; boosted < 2; ++boosted) for (int enchanted = 0; enchanted < 2; ++enchanted)
    {
        Options options; options.boosted = boosted != 0; options.enchanted = enchanted != 0;
        const auto recipe = UpgradeRecipe(art,options);
        std::printf("U %d %d %d %d %d %d\n",art,boosted,enchanted,recipe.result.id,recipe.price.gold,recipe.price.mithril);
    }
    for (int art = -1; art <= 200; ++art) for (int enchanted = 0; enchanted < 2; ++enchanted)
    {
        Options options; options.enchanted = enchanted != 0;
        const auto recipes = ExchangeRecipes(art,options);
        for (const auto &recipe : recipes)
            std::printf("E %d %d %d %d %d\n",art,enchanted,recipe.result.id,recipe.price.gold,recipe.price.mithril);
    }
    for (int recipe = 0; recipe <= 4; ++recipe) for (int enchanted = 0; enchanted < 2; ++enchanted)
    {
        Options options; options.enchanted = enchanted != 0;
        const auto value = CombinationRecipe(recipe,options);
        std::printf("C %d %d %d %d %d %d",recipe,enchanted,value.result.id,value.count,value.price.gold,value.price.mithril);
        for (int part : value.parts) std::printf(" %d",part);
        std::puts(h3_NullString);
    }
}
int main(int argc, char **argv)
{
    if (argc > 1 && std::strcmp(argv[1],"--rules") == 0) { DumpRules(); return 0; }
    unsigned checks = 0;
#define CHECK(value) do { ++checks; if (!(value)) { std::printf("FAIL %d: %s\n",__LINE__,#value); return 1; } } while(false)
    auto state = State::Fresh(7); state.visited=8; state.MarkAiVisit(); state.CompleteOrder();
    CHECK(state.Valid() && state.Orders() == 1 && state.AiVisited() && state.visited == 8);
    CHECK(state.At(7).word == state.word);
    CHECK(state.At(8).Orders() == 0 && !state.At(8).AiVisited() && state.At(8).visited == 8);
    CHECK(State{state.word}.At(7).word == state.word);
    CHECK(State{123}.At(1).Orders() == 0);
    Wallet overflow{INT_MAX,INT_MAX}; CHECK(!overflow.CanPay({-1,0}));
    CHECK(ExchangeRecipes(-1,{}).empty());
    auto &parent = static_cast<extender::ObjectExtender &>(wog::WoGObjectsExtender::Get());
    std::array<INT16,232> maxima{};
    parent.AfterLoadingObjectsTxtProc(maxima.data());
    CHECK(parent.GetObjectSubtypesInfo().size() == 89);
    wog::policy::InitializeDefaults();
    CHECK(wog::policy::Handles(63,52) && wog::policy::Handles(141,52));
    for(const auto *entry=wog::objects::DescriptorsBegin();entry!=wog::objects::DescriptorsEnd();++entry)
    {
        CHECK(wog::policy::Handles(63,entry->subtype));
        for(int type:{63,141})
        {
            auto *generator=parent.CreateRMGObjectGen({type,entry->subtype},FALSE);
            CHECK((generator!=nullptr)==(type==141));delete generator;
        }
        H3MapItem ported;ported.objectType=141;ported.objectSubtype=entry->subtype;
        CHECK(parent.VisitMapItem(&hero,&ported,position,TRUE));
        int weight=-1;CHECK(parent.SetAiMapItemWeight(&ported,&hero,nullptr,weight,nullptr,position)&&weight==100);
    }
    for (int type : {63,141})
    {
        Reset(type); Equip(7,3);
        dialogAction = [](Host &host) { Work(host,2); };
        CHECK(parent.VisitMapItem(&hero,&object,position,TRUE));
        CHECK(hero.bodyArtifacts[3].id == 9 && hero.movement == 800);
        CHECK(fakeart::main.players[3].playerResources.gold == 6000);
        CHECK(State{object.setup}.Orders() == 2 && State{object.setup}.visited == 8);
        CHECK(hero.primary == 3); // Native remove/equip applies bonuses once; no manual stat correction.
        CHECK(parent.SetHintInH3TextBuffer(&object,&hero,3,TRUE) && std::strstr(h3_TextBuffer,"orders 0"));
        CHECK(bool(parent.InitNewGameMapItemSetup(&object,0,0)) == (type==141));
        CHECK(State{object.setup}.Orders() == (type==141?0:2));
        for (BOOL preview : {FALSE,TRUE})
        {
            auto *generator = parent.CreateRMGObjectGen({type,52},preview);
            CHECK((generator != nullptr) == (type == 141)); delete generator;
        }
    }
    Reset(); Equip(7,3); fakeart::menus = {0,0}; fakeart::confirmations = {false};
    CHECK(Visit(&hero,&object,position,TRUE));
    CHECK(hero.bodyArtifacts[3].id == 7 && hero.movement == 1000 && State{object.setup}.Orders() == 0);
    CHECK(fakeart::main.players[3].playerResources.gold == 10000);
    Reset(); Equip(7,3); auto offer = Offers(Service::Upgrade).front();
    fakeart::main.players[3].playerResources.gold = 999;
    CHECK(Apply(offer) == CommitResult::NoResources && hero.bodyArtifacts[3].id == 7);
    fakeart::main.players[3].playerResources.gold = 1000;
    CHECK(Apply(offer) == CommitResult::Success && fakeart::main.players[3].playerResources.gold == 0);
    Reset(); Equip(11,3); offer = Offers(Service::Upgrade).front(); fakeart::mithril[3] = 0;
    CHECK(Apply(offer) == CommitResult::NoResources && hero.bodyArtifacts[3].id == 11);
    fakeart::mithril[3] = 1; hero.movement = 30;
    CHECK(Apply(offer) == CommitResult::Success && fakeart::mithril[3] == 0 && hero.movement == 0);
    Reset(); Equip(7,3); offer = Offers(Service::Upgrade).front(); fakeart::banned[8] = true;
    CHECK(Apply(offer) == CommitResult::Changed && hero.bodyArtifacts[3].id == 7);
    CHECK(Offers(Service::Upgrade).empty());
    Reset(); Equip(37,6,111); Equip(37,7,222);
    auto offers = Offers(Service::Upgrade); CHECK(offers.size() == 2);
    CHECK(Apply(offers[1]) == CommitResult::Success);
    CHECK(hero.bodyArtifacts[6] == H3Artifact(37,111) && hero.bodyArtifacts[7].id == -1 && hero.bodyArtifacts[3].id == 38);
    Reset(); Equip(37,6,111); offer = Offers(Service::Upgrade).front(); hero.bodyArtifacts[6].subtype = 222;
    CHECK(Apply(offer) == CommitResult::Changed && hero.bodyArtifacts[6].subtype == 222);
    Reset(); Equip(7,3); offer = Offers(Service::Upgrade).front(); object.setup = State::Fresh(2).word; fakeart::main.date.zeroBasedDay = 1;
    CHECK(Apply(offer) == CommitResult::Changed);
    Reset(); Equip(7,3); offer = Offers(Service::Upgrade).front(); hero.owner = 4;
    CHECK(Apply(offer) == CommitResult::Changed && fakeart::main.players[4].playerResources.gold == 10000);
    Reset(); Equip(41,8); Equip(78,1);
    for (int i = 0; i < 64; ++i) Bag(52);
    CHECK(Offers(Service::Upgrade).empty());
    Reset(); Equip(94,6); Equip(95,7); Bag(37,22); Bag(45,33);
    for (int i = 2; i < 64; ++i) Bag(52);
    offers = Offers(Service::Combine); CHECK(offers.size() == 1 && offers[0].destination == BACKPACK);
    CHECK(Apply(offers[0]) == CommitResult::Success && hero.backpackCount == 63);
    CHECK(hero.backpackArtifacts[62].id == 153 && fakeart::mithril[3] == 95);
    Reset(); Equip(7,3,91); offer = Offers(Service::Upgrade).front(); fakeart::failGive = 1;
    CHECK(Apply(offer) == CommitResult::InventoryFailure);
    CHECK(hero.bodyArtifacts[3] == H3Artifact(7,91) && hero.primary == 1 && hero.movement == 1000);
    CHECK(fakeart::main.players[3].playerResources.gold == 10000 && State{object.setup}.Orders() == 0);
    Reset(); for (int id = 46; id <= 51; ++id) Bag(id,id);
    offer = Offers(Service::Combine).front(); fakeart::failRemove = 2;
    CHECK(Apply(offer) == CommitResult::InventoryFailure && hero.backpackCount == 6);
    for (int i = 0; i < 6; ++i) CHECK(hero.backpackArtifacts[i] == H3Artifact(46+i,46+i));
    Reset(); Equip(7,3); offer = Offers(Service::Upgrade).front();
    fakeart::inventoryHook = [&](bool equip,int) { if (equip) fakeart::main.players[3].playerResources.gold = 0; };
    CHECK(Apply(offer) == CommitResult::InventoryFailure && hero.bodyArtifacts[3].id == 7 && hero.movement == 1000);
    CHECK(State{object.setup}.Orders() == 0);
    Reset(); Bag(52,1); Bag(52,2); Bag(53,3);
    object.setup = State::Fresh(1).word; auto oneOrder = State{object.setup}; oneOrder.CompleteOrder(); object.setup = oneOrder.word;
    offer = Offers(Service::SellGold)[1]; const int reward = -offer.price.gold;
    CHECK(Apply(offer) == CommitResult::Success && hero.backpackCount == 2);
    CHECK(hero.backpackArtifacts[0] == H3Artifact(52,1) && hero.backpackArtifacts[1] == H3Artifact(53,3));
    CHECK(fakeart::main.players[3].playerResources.gold == 10000 + reward && State{object.setup}.Orders() == 1 && hero.movement == 1000);
    offer = Offers(Service::SellMithril).front(); const int mithril = -offer.price.mithril;
    CHECK(Apply(offer) == CommitResult::Success && fakeart::mithril[3] == 100+mithril);
    Reset(); Equip(7,3); fakeart::main.players[3].playerResources.gold = 500;
    int weight = -1; CHECK(SetAiWeight(&object,&hero,position,weight) && weight > 0);
    CHECK(Visit(&hero,&object,position,FALSE));
    CHECK(hero.bodyArtifacts[3].id == 8 && fakeart::main.players[3].playerResources.gold == 0 && hero.movement == 900);
    CHECK(State{object.setup}.Orders() == 1 && State{object.setup}.AiVisited());
    fakeart::main.players[3].playerResources.gold = 10000;
    CHECK(SetAiWeight(&object,&hero,position,weight) && weight == 0);
    CHECK(Visit(&hero,&object,position,FALSE) && hero.bodyArtifacts[3].id == 8);
    fakeart::main.date.zeroBasedDay = 7;
    CHECK(SetAiWeight(&object,&hero,position,weight) && weight > 0);
    Reset(); for (int id = 46; id <= 51; ++id) Bag(id);
    CHECK(Visit(&hero,&object,position,FALSE));
    CHECK(hero.bodyArtifacts[2].id == 108 && fakeart::main.players[3].playerResources.gold == 8750);
    CHECK(hero.movement == 900 && State{object.setup}.Orders() == 1 && hero.backpackCount == 0);
    Reset(63); Equip(7,3); fakeart::options[26] = 0;
    wog::policy::InitializeDefaults();
    Era::SetAssocVarIntValue(wog::policy::Key(63,52).c_str(),0);
    CHECK(!Visit(&hero,&object,position,FALSE) && hero.bodyArtifacts[3].id == 7);
    weight=123;CHECK(!SetAiWeight(&object,&hero,position,weight) && weight==123);
    CHECK(!SetHint(&object,&hero,TRUE));
    object.objectType = 141;
    CHECK(Visit(&hero,&object,position,FALSE) && hero.bodyArtifacts[3].id != 7);
    Reset(63); Equip(7,3); object.setup = 1; fakeart::legacyFatigue = 3;
    Era::handlers.at("OnAfterLoadGame")(nullptr);
    CHECK(Visit(&hero,&object,position,TRUE) && State{object.setup}.Orders() == 0); // Legacy PO fatigue is deliberately not imported.
    fakeart::main.date.zeroBasedDay = 1;
    CHECK(Offers(Service::Upgrade).size() == 1 && State{object.setup}.Orders() == 0); // Reading does not mutate the save word.
    Reset(63); object.setup = 0; fakeart::main.date.zeroBasedDay = 2;
    CHECK(Offers(Service::Combine).empty());
    NativeHost host(&hero,&object,true,position); CHECK(host.ReadState().Orders() == 0);
    Era::v[1] = 9876; Era::f[1] = true; host.RefreshCatalog(); host.Allowed(8); host.ReadWallet();
    CHECK(Era::v[1] == 9876 && Era::f[1]);
    Reset(); Equip(7,3); hero.blockedArtifacts[3] = 1;
    CHECK(Offers(Service::Upgrade).empty());
    Reset(); Equip(86,9); fakeart::banned[87] = fakeart::banned[89] = true;
    offers = Offers(Service::Exchange); CHECK(offers.size() == 1 && offers[0].result.id == 88);
    Reset(); Equip(11,3); oneOrder = State::Fresh(1); oneOrder.CompleteOrder(); object.setup = oneOrder.word;
    fakeart::main.players[3].playerResources.gold = 20000; fakeart::mithril[3] = 2;
    offer = Offers(Service::Upgrade).front();
    CHECK(offer.price.gold == 15000 && offer.price.mithril == 2);
    CHECK(Apply(offer) == CommitResult::Success && fakeart::main.players[3].playerResources.gold == 5000 && fakeart::mithril[3] == 0);
    Reset(); for (int id = 46; id <= 51; ++id) Bag(id); fakeart::banned[108] = true;
    CHECK(Offers(Service::Combine).empty());
    fakeart::banned[108] = false; hero.hero_class = 8;
    CHECK(Offers(Service::Combine,false).empty());
    Reset(); Equip(7,3); H3MapItem heroTile; heroTile.objectType = 34; heroTile.setup = hero.id;
    fakeart::main.tile = &heroTile; hero.objectBelow = true; hero.objectTypeUnder = 141; hero.objectBelowSetup = object.setup;
    dialogAction = [](Host &host) { Work(host,1); };
    CHECK(Visit(&hero,&object,position,TRUE));
    CHECK(heroTile.setup == unsigned(hero.id) && State{hero.objectBelowSetup}.Orders() == 1 && State{object.setup}.Orders() == 1);
    Reset(63); Equip(7,3); dialogAction = [](Host &host) { Work(host,1); };
    std::array<size_t,6> frame{}; static_assert(sizeof(size_t) == 4,"Hooks require x86");
    frame[2] = reinterpret_cast<size_t>(&hero); frame[3] = reinterpret_cast<size_t>(&object); frame[4] = position.value; frame[5] = TRUE;
    HookContext context; context.ebp = reinterpret_cast<size_t>(frame.data());
    CHECK(fakePatcher.hooks.at(0x705B18)(nullptr,&context) == NO_EXEC_DEFAULT);
    CHECK(context.eax == 1 && context.return_address == 0x705B1D && State{object.setup}.Orders() == 1);
    context.ebx = reinterpret_cast<size_t>(&object);
    CHECK(fakePatcher.hooks.at(0x40C5A1)(nullptr,&context) == NO_EXEC_DEFAULT && context.return_address == 0x40D09B);
    CHECK(fakePatcher.hooks.at(0x414F66)(nullptr,&context) == NO_EXEC_DEFAULT && context.return_address == 0x4159C0);
    // Sales cover every eligible backpack slot, including after both work orders are spent.
    Reset(); Equip(7,3,456);
    for (int i=0; i<64; ++i) Bag(52,100+i);
    auto tired = State::Fresh(1); tired.CompleteOrder(); tired.CompleteOrder(); object.setup = tired.word;
    NativeHost saleHost(&hero,&object,true,position);
    auto batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    CHECK(batch.valid && batch.offers.size() == 64 && Offers(Service::SellGold).size() == 64);
    CHECK(Offers(Service::Upgrade).empty());
    const int totalGold = -batch.price.gold;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::Success);
    CHECK(hero.backpackCount == 0 && hero.bodyArtifacts[3] == H3Artifact(7,456));
    CHECK(fakeart::main.players[3].playerResources.gold == 10000+totalGold && hero.movement == 1000);
    CHECK(State{object.setup}.Orders() == 2 && State{object.setup}.visited == 8);
    Reset(); for (int i=0; i<12; ++i) Bag(52,i);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellMithril);
    const int totalMithril = -batch.price.mithril;
    CHECK(batch.valid && CommitSaleBatch(saleHost,batch) == CommitResult::Success);
    CHECK(fakeart::mithril[3] == 100+totalMithril && hero.backpackCount == 0 && State{object.setup}.Orders() == 0);
    Reset(); Bag(52,1); Bag(53,2);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    hero.backpackArtifacts[0].subtype = 99;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::Changed && hero.backpackCount == 2);
    CHECK(fakeart::main.players[3].playerResources.gold == 10000);
    Reset(); Bag(52,1);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    Bag(53,2); // A newly added artifact must require a new confirmation too.
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::Changed && hero.backpackCount == 2);
    Reset(); Bag(52,1);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    fakeart::artifacts[52].cost += 4000;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::Changed && hero.backpackCount == 1);
    Reset(); Bag(52,1);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    fakeart::options[36] = 0;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::Changed && Offers(Service::SellGold).empty());
    Reset(); Bag(52,1);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    hero.owner = 4;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::Changed && hero.backpackCount == 1);
    Reset(); Bag(52,1);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    fakeart::main.players[3].playerResources.gold = INT_MAX;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::NoResources && hero.backpackCount == 1);
    Reset(); for (int i=0; i<64; ++i) Bag(52,i);
    fakeart::artifacts[52].cost = INT_MAX;
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    CHECK(!batch.valid && CommitSaleBatch(saleHost,batch) == CommitResult::Changed && hero.backpackCount == 64);
    Reset(); Bag(52,1); Bag(53,2); Bag(54,3); Bag(55,4);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    fakeart::failRemove = 3;
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::InventoryFailure && hero.backpackCount == 4);
    for (int i=0; i<4; ++i) CHECK(hero.backpackArtifacts[i] == H3Artifact(52+i,i+1));
    CHECK(fakeart::main.players[3].playerResources.gold == 10000 && hero.movement == 1000 && State{object.setup}.Orders() == 0);
    Reset(); Bag(52,1); Bag(53,2);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    fakeart::backpackHook = []() { fakeart::main.players[3].playerResources.gold = INT_MAX; };
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::InventoryFailure && hero.backpackCount == 2);
    CHECK(hero.backpackArtifacts[0] == H3Artifact(52,1) && hero.backpackArtifacts[1] == H3Artifact(53,2));
    CHECK(State{object.setup}.Orders() == 0 && hero.movement == 1000);
    Reset(); Bag(52,1);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    fakeart::backpackHook = []() { fakeart::options[36] = 0; };
    CHECK(CommitSaleBatch(saleHost,batch) == CommitResult::InventoryFailure && hero.backpackCount == 1);
    CHECK(fakeart::main.players[3].playerResources.gold == 10000);
    Reset(); Bag(1,123); Bag(0); Bag(52,7); Equip(13,4);
    batch = BuildSaleBatch(saleHost.ReadInventory(),saleHost,saleHost.ReadOptions(),saleHost.ReadState(),Service::SellGold);
    CHECK(batch.offers.size() == 1 && CommitSaleBatch(saleHost,batch) == CommitResult::Success);
    CHECK(hero.backpackCount == 2 && hero.backpackArtifacts[0] == H3Artifact(1,123) && hero.bodyArtifacts[4].id == 13);
    Reset(); tired = State::Fresh(1); tired.CompleteOrder(); tired.CompleteOrder(); object.setup = tired.word;
    bool dialogOpened = false; dialogAction = [&](Host &) { dialogOpened = true; };
    CHECK(Visit(&hero,&object,position,TRUE) && dialogOpened);
    std::printf("PASS: %u Artificer state, native adapter, transaction, bulk sale and dual-type checks\n", checks);
}
