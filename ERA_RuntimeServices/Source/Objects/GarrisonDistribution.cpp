#include "../framework.h"
#include "GarrisonDistribution.h"

// xyz = count triples, output = at least count integers, both caller-owned.
// Returns written indices; negative means unavailable/invalid (no map mutation).
extern "C" int __stdcall Erm_OrderGarrisonSites(const int* xyz,int count,int size,int* output,int capacity,unsigned seed)
{
    if(!xyz||!output||count<0||count>131072||capacity<count||size<8||size>256) return -1;
    try {
        const auto order=erm_functions::distribution::OrderGarrisons(xyz,count,size,seed);
        std::copy(order.begin(),order.end(),output);
        return static_cast<int>(order.size());
    } catch(const std::bad_alloc&) {return -1;}
}
