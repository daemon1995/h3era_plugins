#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <map>
#include <cstdarg>
#include <cstdio>
using INT16 = short;
struct H3Point { int x,y,z; H3Point(int x=0,int y=0,int z=0):x(x),y(y),z(z) {} };
struct H3Position : H3Point
{
    H3Position(int x,int y,int z):H3Point(x,y,z) {}
    int GetX() const {return x;} int GetY() const {return y;} int GetZ() const {return z;}
};
struct H3MapItem {unsigned setup=0;int objectType=141,objectSubtype=9;};
struct H3Hero {bool objectBelow=false;int objectTypeUnder=141,x=0,y=0,z=0;unsigned objectBelowSetup=0;};
struct FakeMain
{
    struct Date { int day=0; int CurrentDay() const {return day;} } date;
    struct Setup { H3MapItem *mapitems=nullptr; int mapSize=2; bool hasUnderground=true;
    H3MapItem *GetMapItem(int x,int y,int z){return x>=0&&x<2&&y>=0&&y<2&&z>=0&&z<2?mapitems+x+2*y+4*z:nullptr;} } mainSetup;
    H3Point GetCoordinates(H3MapItem *item)
    {const auto n=int(item-mainSetup.mapitems);return H3Point(n%2,(n/2)%2,n/4);}
};
static FakeMain mainData; static FakeMain *P_Main=&mainData;
struct H3String
{
    std::string value;
    static H3String Format(const char *format,...)
    {char buffer[256];va_list args;va_start(args,format);vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);return {buffer};}
    const char *String() const {return value.c_str();}
};
namespace Era
{
static std::map<std::string,int> assoc;
static int GetAssocVarIntValue(const char *key) {const auto it=assoc.find(key);return it==assoc.end()?0:it->second;}
static void SetAssocVarIntValue(const char *key,int value) {assoc[key]=value;}
}

#include <cassert>
static int nativeVisitReads = 0, nativeVisitWrites = 0;
template<typename Result> Result NativeVisitCall(unsigned address, const H3MapItem *item, int player)
{
    assert(address == 0x529B70 || address == 0x4FC620);
    if (address == 0x4FC620)
    {
        ++nativeVisitWrites;
        if (player >= 0 && player < 8) const_cast<H3MapItem *>(item)->setup |= 1u << (5 + player);
    }
    else ++nativeVisitReads;
    return static_cast<Result>(player >= 0 && player < 8 && (item->setup & (1u << (5 + player))));
}
#define THISCALL_2(type,address,item,player) NativeVisitCall<type>(address,item,player)