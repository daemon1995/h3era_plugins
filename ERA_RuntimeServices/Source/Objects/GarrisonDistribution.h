#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace erm_functions { namespace distribution {
// Pure ordering of caller-owned candidate coordinates. Never accesses the game.
inline std::vector<int> OrderGarrisons(const int* xyz, int count, int size, std::uint32_t seed)
{
    const int grid=(std::max)(2,(std::min)(7,size/36));
    std::vector<std::vector<int>> buckets(grid*grid*2);
    for(int i=0;i<count;++i)
    {
        const int x=xyz[i*3],y=xyz[i*3+1],z=xyz[i*3+2];
        if(x<0||y<0||x>=size||y>=size||z<0||z>1) continue;
        buckets[z*grid*grid+(y*grid/size)*grid+x*grid/size].push_back(i);
    }
    std::uint32_t state=seed ? seed : 0x9e3779b9u;
    const auto random=[&]() {state^=state<<13;state^=state>>17;state^=state<<5;return state;};
    const auto shuffle=[&](std::vector<int>& list) {
        for(std::size_t n=list.size();n>1;--n) std::swap(list[n-1],list[random()%n]);
    };
    std::vector<int> active;
    for(unsigned i=0;i<buckets.size();++i)
        if(!buckets[i].empty()) {shuffle(buckets[i]);active.push_back(i);}
    std::vector<int> result;result.reserve(count);
    // One candidate per nonempty sector per round; randomize sector order each
    // round so even a limit smaller than a round does not favor an edge/level.
    while(!active.empty())
    {
        shuffle(active);
        for(int b:active) {result.push_back(buckets[b].back());buckets[b].pop_back();}
        active.erase(std::remove_if(active.begin(),active.end(),[&](int b){return buckets[b].empty();}),active.end());
    }
    return result;
}
} }
