#pragma once
#include "Plugin.h"
#include <algorithm>
#include <cstddef>

//=================================================
namespace WoG
{

enum eExpType
{
    CE_UNKNOWN = 0x0,
    CE_FIRST = 0x1,
    CE_HERO = CE_FIRST,
    CE_MAP = 0x2,
    CE_TOWN = 0x3,
    CE_MINE = 0x4,
    CE_HORN = 0x5,
    CE_LAST = CE_HORN,
};

#pragma pack(push, 1)
struct CrExpModStr
{
    unsigned __int32 Act : 1;
    unsigned __int32 _unused : 31;
    char Type;
    char Mod;
    char Lvls[11];
};
#pragma pack(pop)

static_assert(sizeof(CrExpModStr) == 17, "WoG experience modifier row must have 17-byte stride");
static_assert(alignof(CrExpModStr) == 1, "WoG modifier row uses packed layout");
static_assert(offsetof(CrExpModStr, Type) == 4, "WoG modifier type offset");
static_assert(offsetof(CrExpModStr, Mod) == 5, "WoG modifier kind offset");
static_assert(offsetof(CrExpModStr, Lvls) == 6, "WoG modifier levels offset");

struct CrExpBon
{
    static constexpr DWORD CR_EXP_BONUS_TABLE = 0x0847D98;
    static constexpr int CR_EXP_BONUS_LINES = 20;

  public:
    inline static void MakeCurrent(int stackId, int MType)
    {
        CDECL_2(void, 0x728120, stackId, MType);
    }
    inline static CrExpModStr *GetBonusLine(int lineId)
    {
        return reinterpret_cast<WoG::CrExpModStr *>(CR_EXP_BONUS_TABLE + lineId * 17);
    }
};

#pragma pack(push, 1)
struct CrExpo
{
    DWORD experience; // тек. опыт на 1 существо
    DWORD number;     // число существ
    struct
    {
        unsigned __int32 Act : 1;
        unsigned __int32 Type : 4;
        unsigned __int32 creatureId : 8;
        unsigned __int32 mHasArt : 1;
        unsigned __int32 mArt : 2;
        unsigned __int32 mCopyArt : 2;
        unsigned __int32 mSubArt : 4;
        unsigned __int32 _un : 10;
    } flags;

    union UniData {
        DWORD uniData;
        DWORD mixPos;
        struct
        {
            __int16 id;
            __int16 slot;
        } hero;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            unsigned __int32 _un : 15;
        } map;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } town;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } mine;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } garrison;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } anyGarrison;
    } place;

  public:
    static inline CrExpo *Get()
    {
        return reinterpret_cast<CrExpo *>(0x860550);
    }
    static inline CrExpo *GetFromCombatCreatureIndex(const int index)
    {
        return CDECL_1(CrExpo *, 0x728110, index);
    }
    static inline CrExpo *Find(const eExpType type, const UniData data)
    {
        return CDECL_2(CrExpo *, 0x0718617, type, data.uniData);
    }
    inline void Clear()
    {
        return THISCALL_1(void, 0x0718377, this);
    };
    inline void Clamp()
    {
        experience = std::min<DWORD>(experience, GetCreatureExpLimit(flags.creatureId));
    }
    inline void Clamp(const int monId)
    {
        experience = std::min<DWORD>(experience, GetCreatureExpLimit(monId));
    }
    static inline int SetNewAndClamp(const eExpType type, const UniData data, const int creatureId,
                                     const int creaturesNum, const int expo)
    {
        return CDECL_5(int, 0x0718AD0, type, data.uniData, creatureId, creaturesNum, expo);
    }
    static inline int GetCreatureExpByRankAndExp(const int monId, const int exp, const int additional)
    {
        return CDECL_3(int, 0x0727E40, monId, exp, additional);
    }
    static inline int GetCreatureExpLimit(const int monId)
    {
        return CDECL_1(int, 0x0727E20, monId);
    }
    static inline float GetCreatureUpgradeMultiplier(const int monId)
    {
        return CDECL_1(float, 0x0727E00, monId);
    }
};
#pragma pack(pop)

}

static_assert(sizeof(WoG::CrExpo) == 0x10, "WoG CrExpo ABI size");
static_assert(alignof(WoG::CrExpo) == 1, "WoG CrExpo must be tightly packed");
static_assert(offsetof(WoG::CrExpo, number) == 0x04, "WoG CrExpo creature count offset");
static_assert(offsetof(WoG::CrExpo, flags) == 0x08, "WoG CrExpo flags offset");
static_assert(offsetof(WoG::CrExpo, place) == 0x0C, "WoG CrExpo location offset");
