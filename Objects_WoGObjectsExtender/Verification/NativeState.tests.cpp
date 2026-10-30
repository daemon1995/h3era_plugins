#include <WoGObjectState.cpp>
#include <cassert>
#include <climits>
using namespace wog::state;
int main()
{
    unsigned checks=0;
#define CHECK(x) do {++checks;assert(x);} while(false)
    const auto hero=HeroKey(57,7,"stolenArtifact");
    IntValue value(hero,-1), other(HeroKey(57,8,"stolenArtifact"),-1);
    CHECK(int(value)==-1 && int(other)==-1);
    value=0; CHECK(int(value)==0 && int(other)==-1);
    value=9; const auto saved=Era::assoc;
    Era::assoc.clear(); CHECK(int(value)==-1); // A live proxy cannot cache values from the previous game.
    Era::assoc=saved; CHECK(int(value)==9); // Reusing it after load resolves current assoc storage.
    IntValue loaded(hero,-1); CHECK(int(loaded)==9);
    loaded=other; CHECK(int(value)==-1);
    value=3; CHECK(--value==2 && ++value==3); value|=4; CHECK(int(value)==7);
    value=other=0; CHECK(int(value)==0 && int(other)==0);
    value=INT_MAX; CHECK(int(value)==INT_MAX);
    H3MapItem tiles[8]; mainData.mainSetup.mapitems=tiles;
    CHECK(Position(&tiles[7]).x==1 && Position(&tiles[7]).y==1 && Position(&tiles[7]).z==1);
    H3MapItem copy;
    CHECK(Position(&copy).x==-1);
    {
        ObjectPositionScope position(&copy,H3Position(20,30,1));
        CHECK(Position(&copy).x==20 && Position(&copy).y==30);
        const auto key=ObjectKey(&copy,57,"cleared"); Set(key,1);
        {ObjectPositionScope inner(&copy,H3Position(21,30,1)); CHECK(Get(ObjectKey(&copy,57,"cleared"))==0);}
        CHECK(Get(ObjectKey(&copy,57,"cleared"))==1);
    }
    CHECK(Position(&copy).x==-1);
    SetHeroEver(3,7); CHECK(HeroEver(3,7) && !HeroEver(3,8) && !HeroEver(24,7));
    Stamp(hero,Day()); CHECK(IsStamp(hero,Day()));
    mainData.date.day=1; CHECK(!IsStamp(hero,Day()));
    for(int subtype:{9,20,65})
    {
        const char *field=subtype==65?"visitedMonth":"week";
        H3MapItem &tile=tiles[0];tile.setup=0;tile.objectSubtype=subtype;
        for(int player=0;player<8;++player)
        {
            CHECK(!ObjectPlayerPeriod(&tile,subtype,player,field,1));
            SetObjectPlayerPeriod(&tile,subtype,player,field,1);
            CHECK(ObjectPlayerPeriod(&tile,subtype,player,field,1));
        }
        for(int player=0;player<8;++player) CHECK(ObjectPlayerPeriod(&tile,subtype,player,field,1));
        SetObjectPlayerPeriod(&tile,subtype,4,field,2);
        CHECK(ObjectPlayerPeriod(&tile,subtype,4,field,2) && !ObjectPlayerPeriod(&tile,subtype,3,field,2));
        CHECK(!ObjectPlayerPeriod(&tile,subtype,4,field,1));
    }
    for(int period:{1,127,128,65535,65536,int(wog::setup::PeriodVisitors::MAX_PERIOD)})
    {
        H3MapItem marks; marks.objectSubtype=9;
        for(int player=0;player<8;++player){SetObjectPlayerPeriod(&marks,9,player,"week",period);CHECK(ObjectPlayerPeriod(&marks,9,player,"week",period));}
        CHECK(!ObjectPlayerPeriod(&marks,9,-1,"week",period) && !ObjectPlayerPeriod(&marks,9,8,"week",period));
        const auto savedWord=marks.setup; SetObjectPlayerPeriod(&marks,9,8,"week",period); CHECK(marks.setup==savedWord);
        SetObjectPlayerPeriod(&marks,9,0,"week",period+1);
        if(period==int(wog::setup::PeriodVisitors::MAX_PERIOD)) CHECK(marks.setup==savedWord);
        else CHECK(ObjectPlayerPeriod(&marks,9,0,"week",period+1) && !ObjectPlayerPeriod(&marks,9,1,"week",period+1) && !ObjectPlayerPeriod(&marks,9,0,"week",period));
    }
    CHECK(nativeVisitReads > 0 && nativeVisitWrites > 0);
    H3Hero visitor;visitor.objectBelow=true;visitor.x=visitor.y=visitor.z=0;
    copy.objectType=141;copy.objectSubtype=9;
    tiles[0].objectType=34;tiles[0].setup=7;
    {ObjectPositionScope scope(&copy,H3Position(0,0,0));SetObjectPlayerPeriod(&copy,9,2,"week",4,&visitor);}
    CHECK(visitor.objectBelowSetup==copy.setup && tiles[0].setup==7);
    SetObjectDay(&copy,999,&visitor);CHECK(ObjectDay(&copy,999) && !ObjectDay(&copy,998));
    CHECK((!wog::setup::DeathChamber{0}.Initialized()));
    for(int difficulty=1;difficulty<=4;++difficulty)
        for(int day:{0,1,65535,65536,1000000})
        {
            const auto value=wog::setup::DeathChamber::At(difficulty,day);
            CHECK(value.Initialized() && value.BaseDifficulty()==difficulty && value.ClearedDay()==day);
        }
    std::printf("PASS: %u actual assoc-state, load replacement, defaults, isolation and temporary map-position checks\n",checks);
}
