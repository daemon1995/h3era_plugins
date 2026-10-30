#include <ArtificerDialog.cpp>
using namespace wog::artificer;
struct TestHost : Host
{
 Inventory inventory; Options options; State state=State::Fresh(1); Wallet money{100000,1000}; int movement=1000;
 TestHost() { options.hero=options.owner=1; options.mithril=true; }
 bool Allowed(int) const override { return true; } int Value(int) const override { return 4000; }
 bool Fits(int,int slot) const override { return slot==3; }
 Inventory ReadInventory() const override { return inventory; } Options ReadOptions() const override { return options; }
 State ReadState() const override { return state; } Wallet ReadWallet() const override { return money; }
 void SaveState(State value) override { state=value; } void RefreshCatalog() override {}
 bool Remove(Source s) override
 {
  if (inventory.items[s.slot]!=s.item) return false;
  if (s.slot<BACKPACK) inventory.items[s.slot]={};
  else { for (int i=s.slot;i<SLOT_COUNT-1;++i) inventory.items[i]=inventory.items[i+1]; inventory.items.back()={}; }
  return true;
 }
 bool Put(Item item,int slot) override { inventory.items[slot]=item; return true; }
 bool UndoResult() override { return true; } bool Restore(Source s) override { inventory.items[s.slot]=s.item; return true; }
 void Pay(Price p) override { money.gold-=p.gold; money.mithril-=p.mithril; }
 void SpendMovement(int amount) override { movement-=amount; }
 void Bag(int count) { for (int i=0;i<count;++i) inventory.items[BACKPACK+i]={52,100+i}; }
};
static void Click(H3Dlg &dlg,int id) { H3Msg msg; dlg.OnLeftClick(id,msg); }
static int InBag(const TestHost &host) { int n=0; for (int i=BACKPACK;i<SLOT_COUNT;++i) n+=host.inventory.items[i].id>=0; return n; }
static H3DlgText *Caption(H3Dlg &dlg,int id)
{
 for (auto &item:dlg.items) if (item->id==id) if (auto *text=dynamic_cast<H3DlgText*>(item.get())) return text;
 return nullptr;
}
int main()
{
 unsigned checks=0;
#define CHECK(x) do { ++checks; if (!(x)) { std::printf("FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
 H3Hero hero; H3MapItem object;
 TestHost host; host.Bag(64); host.inventory.items[3]={7,77};
 {
  ArtificerDialog dlg(host,&hero,&object);
  CHECK(Caption(dlg,13)->active);
  for (auto &item:dlg.items) if (item->id==7 || item->id==8) CHECK(!item->visible);
  Click(dlg,13); Click(dlg,6); // Cancel bulk sale.
  CHECK(InBag(host)==64 && host.money.gold==100000);
  fakeui::confirm=true; Click(dlg,6);
  CHECK(InBag(host)==0 && host.money.gold==164000 && host.inventory.items[3]==(Item{7,77}));
  CHECK(host.movement==1000 && host.state.Orders()==0 && !dlg.stopped);
 }
 host=TestHost(); host.Bag(13);
 {
  ArtificerDialog dlg(host,&hero,&object); Click(dlg,13); Click(dlg,8); Click(dlg,4); Click(dlg,1002); Click(dlg,5);
  CHECK(InBag(host)==12 && host.money.mithril==1002 && host.money.gold==100000);
  CHECK(host.inventory.items[BACKPACK+7].modifier==108); // Page 2, third item (modifier 107) was sold.
  CHECK(host.state.Orders()==0 && host.movement==1000);
  const auto before=host.money.mithril; Click(dlg,6);
  CHECK(InBag(host)==0 && host.money.mithril==before+24);
 }
 host=TestHost(); host.inventory.items[3]={7,77}; host.Bag(1);
 {
  ArtificerDialog dlg(host,&hero,&object); Click(dlg,5);
  CHECK(host.inventory.items[3].id==8 && host.state.Orders()==1 && host.money.gold==99000);
  dlg.OnKeyPress(eVKey::H3VK_ENTER,eMsgFlag::NONE);
  CHECK(host.inventory.items[3].id==9 && host.state.Orders()==2 && host.money.gold==96000);
  CHECK(!dlg.stopped);
  Click(dlg,13); Click(dlg,5);
  CHECK(InBag(host)==0 && host.state.Orders()==2 && host.movement==800);
  dlg.OnKeyPress(eVKey::H3VK_ESCAPE,eMsgFlag::NONE); CHECK(dlg.stopped);
 }
 host=TestHost(); host.Bag(3);
 {
  ArtificerDialog dlg(host,&hero,&object); Click(dlg,13);
  fakeui::onConfirm=[&] { host.inventory.items[BACKPACK].modifier=999; };
  Click(dlg,6); fakeui::onConfirm={};
  CHECK(InBag(host)==3 && host.money.gold==100000);
  H3DlgItem *art=nullptr;
  for (auto &item:dlg.items) if (item->id==1000 && dynamic_cast<H3DlgDef*>(item.get())) { art=item.get(); break; }
  CHECK(art); dlg.OnRightClick(art); CHECK(fakeui::descriptions==1 && P_WindowManager->resultItemID==123);
 }
 host=TestHost(); host.options.mithril=false; host.Bag(1);
 {
  ArtificerDialog dlg(host,&hero,&object); CHECK(!Caption(dlg,13)->active);
  Click(dlg,13); Click(dlg,6); CHECK(InBag(host)==1 && host.money.gold==100000);
 }
 host=TestHost(); host.inventory.items[3]={7,77}; host.money.gold=0;
 {
  ArtificerDialog dlg(host,&hero,&object); CHECK(!Caption(dlg,5)->active);
  dlg.OnKeyPress(eVKey::H3VK_ENTER,eMsgFlag::NONE);
  CHECK(host.inventory.items[3].id==7 && host.state.Orders()==0);
 }
 std::printf("PASS: %u native Artificer dialog interaction checks\n",checks);
}
