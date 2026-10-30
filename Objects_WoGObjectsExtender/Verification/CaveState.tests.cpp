#include "../WoGObjectCaveState.h"
#include "../WoGObjectCaveData.h"
#include <cassert>
#include <cstdio>
using wog::cave::State;
int main()
{
    unsigned checks=0;
    for(int branches=0;branches<4;++branches)
        for(int slot=0;slot<4;++slot)
            for(int value=0;value<128;++value)
            {
                const int rooms[4]={13,37,54,1};auto state=State::Fresh(branches,rooms);
                state.Room(slot,value);assert(state.Initialized()&&state.Count()==branches+1);checks+=2;
                for(int other=0;other<4;++other){assert(state.Room(other)==(other==slot?value:rooms[other]));++checks;}
                const auto saved=state.word;state.ClearEntrance();assert(state.EntranceCleared());++checks;
                state.Finish(slot);assert(state.Room(slot)==(value|64));++checks;
                assert(State{saved}.Room(slot)==value);++checks;
            }
    for(auto &row:wog::cave_data::positions)for(int hex:row){assert(hex>=0&&hex<187);++checks;}
    for(int price:wog::cave_data::schoolSpellPrices){assert(price>0);++checks;}
    std::printf("PASS: %u cave state, save-word and battlefield-table checks\n",checks);
}
