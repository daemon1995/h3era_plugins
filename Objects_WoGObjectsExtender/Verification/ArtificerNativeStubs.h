#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>
using BOOL = int; using DWORD = unsigned long; using INT16 = short; using LPCSTR = const char *;
constexpr BOOL TRUE = 1, FALSE = 0;
enum eResource { GOLD = 6 }; enum eArtifact : int {};
struct H3Position
{
    DWORD value = 0;
    H3Position(DWORD value = 0) : value(value) {}
    H3Position(int x, int y, int z) : value(x | (y << 8) | (z << 16)) {}
    unsigned GetX() const { return value & 255; } unsigned GetY() const { return (value >> 8) & 255; }
    unsigned GetZ() const { return (value >> 16) & 1; }
};
struct H3ArtifactSetup
{
    int cost = 1000, position = 9;
    bool disabled = false;
    std::string name = "Artifact";
    static H3ArtifactSetup *Get();
};
struct H3ArtifactCount { static int Get() { return 164; } };
struct H3Artifact
{
    int id = -1, subtype = -1;
    H3Artifact() = default;
    H3Artifact(int id, int modifier) : id(id), subtype(modifier) {}
    bool operator==(const H3Artifact &other) const { return id == other.id && subtype == other.subtype; }
    const char *GetName() const { return H3ArtifactSetup::Get()[id].name.c_str(); }
};
struct H3MapItem { int objectType = 141, objectSubtype = 52; DWORD setup = 0; };
struct H3Hero
{
    int id = 7, owner = 3, hero_class = 0, x = 10, y = 20, z = 0, movement = 1000;
    bool objectBelow = false;
    int objectTypeUnder = -1;
    DWORD objectBelowSetup = 0;
    H3Artifact bodyArtifacts[19], backpackArtifacts[64];
    unsigned char blockedArtifacts[14]{};
    int backpackCount = 0, primary = 0, recalculations = 0;
    void RemoveArtifact(int slot);
    void RemoveBackpackArtifact(int slot);
    void GiveArtifact(const H3Artifact &, int slot);
    void GiveBackpackArtifact(const H3Artifact &, int slot = -1);
    BOOL CanReplaceArtifact(int id, int slot) const;
    BOOL CanPlaceArtifact(int id, int slot) const { return bodyArtifacts[slot].id < 0 && CanReplaceArtifact(id, slot); }
    void AddResource(int resource, int amount) const;
    void RecalculateMovement() { ++recalculations; }
};
struct H3Player {int currentHero=-1;};
struct FakeMain
{
    struct Date { unsigned zeroBasedDay = 0; unsigned CurrentDay() const { return zeroBasedDay; } } date;
    struct { int difficulty = 2; } playersInfo;
    struct Player : H3Player { struct { int gold = 10000; } playerResources; } players[8];
    H3MapItem *tile = nullptr;
    H3MapItem *GetMapItem(H3Position) { return tile; }
    H3Hero *GetHero(int) { return nullptr; }
};
namespace fakeart
{
static FakeMain main;
static std::array<H3ArtifactSetup, 164> artifacts;
static std::array<bool, 164> banned{};
static std::array<int, 1000> options{};
static std::array<int, 8> mithril{};
static std::vector<int> menus;
static std::vector<bool> confirmations;
static std::vector<std::string> messages;
static std::function<void(bool, int)> inventoryHook;
static std::function<void()> confirmHook;
static std::function<void()> backpackHook;
static int legacyFatigue = 1, failGive = 0, failRemove = 0, removals = 0, equips = 0;
static std::map<std::string, int> assoc;
}
static FakeMain *P_Main = &fakeart::main;
static DWORD DwordAt(int address) { assert(address == 0x073764C + 3); return reinterpret_cast<DWORD>(fakeart::mithril.data()); }
#define CDECL_1(result, address, arg) (assert(address == 0x714EA7), static_cast<result>(fakeart::banned[arg]))
inline H3ArtifactSetup *H3ArtifactSetup::Get() { return fakeart::artifacts.data(); }
inline BOOL H3Hero::CanReplaceArtifact(int id, int slot) const
{
    const int position = H3ArtifactSetup::Get()[id].position;
    if (slot < 0 || slot > 18 || (slot >= 13 && slot != 18)) return FALSE;
    if (blockedArtifacts[slot == 18 ? 13 : slot]) return FALSE;
    if (position == 7) return slot == 6 || slot == 7;
    if (position == 9) return (slot >= 9 && slot <= 12) || slot == 18;
    return slot == (position == 8 ? 8 : position - 1);
}
inline void H3Hero::RemoveArtifact(int slot)
{
    if (fakeart::failRemove && ++fakeart::removals == fakeart::failRemove) return;
    primary -= H3ArtifactSetup::Get()[bodyArtifacts[slot].id].cost / 1000;
    bodyArtifacts[slot] = {};
    if (fakeart::inventoryHook) fakeart::inventoryHook(false, slot);
}
inline void H3Hero::RemoveBackpackArtifact(int slot)
{
    if (fakeart::failRemove && ++fakeart::removals == fakeart::failRemove) return;
    assert(slot >= 0 && slot < backpackCount);
    for (int i = slot; i < backpackCount - 1; ++i) backpackArtifacts[i] = backpackArtifacts[i + 1];
    backpackArtifacts[--backpackCount] = {};
    if (fakeart::backpackHook) fakeart::backpackHook();
}
inline void H3Hero::GiveArtifact(const H3Artifact &art, int slot)
{
    if (fakeart::failGive) { --fakeart::failGive; return; }
    assert(CanPlaceArtifact(art.id, slot));
    bodyArtifacts[slot] = art; primary += H3ArtifactSetup::Get()[art.id].cost / 1000;
    ++fakeart::equips;
    if (fakeart::inventoryHook) fakeart::inventoryHook(true, slot);
}
inline void H3Hero::GiveBackpackArtifact(const H3Artifact &art, int slot)
{
    assert(backpackCount < 64);
    if (slot < 0 || slot > backpackCount) slot = backpackCount;
    for (int i = backpackCount; i > slot; --i) backpackArtifacts[i] = backpackArtifacts[i - 1];
    backpackArtifacts[slot] = art; ++backpackCount;
}
inline void H3Hero::AddResource(int resource, int amount) const
{
    assert(resource == 6); fakeart::main.players[owner].playerResources.gold += amount;
}
struct H3String
{
    std::string text;
    static H3String Format(const char *format, ...)
    {
        char buffer[1024]; va_list args; va_start(args, format); vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);
        return {buffer};
    }
    const char *String() const { return text.c_str(); }
};
static char h3_TextBuffer[16384];
namespace libc { using std::sprintf; }
namespace Era
{
static int v[10001]{}; static bool f[1001]{};
struct TEvent {};
static std::map<std::string, void(__stdcall *)(TEvent *)> handlers;
static void RegisterHandler(void(__stdcall *handler)(TEvent *), const char *name) { handlers[name] = handler; }
static int GetAssocVarIntValue(const char *key) {auto it=fakeart::assoc.find(key);return it==fakeart::assoc.end()?0:it->second;}
static void SetAssocVarIntValue(const char *key,int value){fakeart::assoc[key]=value;}
static void *GetRealAddr(void *) { return fakeart::options.data(); }
static std::string tr(const char *key, const std::vector<std::string> &parameters = {})
{
    std::string text = key; for (const auto &part : parameters) text += " " + part; return text;
}
static void ExecErmCmd(const char *) { assert(false && "Artificer must not invoke ERM"); }

}
namespace EraJS {
static int readInt(const char *) { return 0; }
static void read(const std::string &, bool &found) { found = false; }
}
struct H3PictureCategories
{
    static H3PictureCategories Artifact(eArtifact) { return {}; }
    static H3PictureCategories Money(int) { return {}; }
};
struct H3Messagebox
{
    static void Show(const char *text, H3PictureCategories = {}) { fakeart::messages.push_back(text); }
    static BOOL Choice(const char *text, H3PictureCategories = {}, H3PictureCategories = {})
    {
        fakeart::messages.push_back(text);
        if (fakeart::confirmHook) fakeart::confirmHook();
        if (fakeart::confirmations.empty()) return FALSE;
        const bool result = fakeart::confirmations.front(); fakeart::confirmations.erase(fakeart::confirmations.begin()); return result;
    }
};
struct LoHook {};
struct HookContext
{
    size_t ebx = 0, ebp = 0, eax = 0, return_address = 0;
    template <typename T> T Ebx() const { return reinterpret_cast<T>(ebx); }
};
static DWORD DwordAt(size_t address) { return *reinterpret_cast<const DWORD *>(address); }
constexpr int EXEC_DEFAULT = 1, NO_EXEC_DEFAULT = 0;
#define _LHF_(name) int __stdcall name(LoHook *h, HookContext *c)
struct PatcherInstance
{
    std::map<size_t, int(__stdcall *)(LoHook *, HookContext *)> hooks;
    void WriteLoHook(size_t address, int(__stdcall *hook)(LoHook *, HookContext *)) { hooks[address] = hook; }
};
static PatcherInstance fakePatcher;
static PatcherInstance *_PI = &fakePatcher;
struct H3RmgObjectGenerator { int type, subtype; };
namespace extender
{
constexpr int WOG_OBJECT_TYPE = 63, ERA_OBJECT_TYPE = 141;
enum eRmgDlgObjectPage { ePageCommon=0,ePageWogObject = 7 };
struct RMGObjectProperties { int type, subtype; };
struct UniqueObjectInfo { int type, subtype; };
class ObjectExtender
{
  protected:
    PatcherInstance *_pi;
    bool m_isInited = false;
    struct List : std::vector<UniqueObjectInfo> { void Reserve(int amount) { reserve(amount); } } objectSubtypesInfo;
    void ReleaseObjectSubtypesInfo() { objectSubtypesInfo.clear(); }
    virtual void CreatePatches() {}
    void AddUniqueObjectInfo(int type, int subtype, int, eRmgDlgObjectPage) { objectSubtypesInfo.push_back({type,subtype}); }
  public:
    explicit ObjectExtender(PatcherInstance *pi) : _pi(pi) {}
    virtual ~ObjectExtender() = default;
    virtual void AfterLoadingObjectsTxtProc(const INT16 *) {}
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectProperties &, BOOL) const noexcept { return nullptr; }
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *, int, int) const noexcept { return FALSE; }
    virtual BOOL VisitMapItem(H3Hero *, H3MapItem *, H3Position, BOOL) const noexcept { return FALSE; }
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *, const H3Hero *, int, BOOL) const noexcept { return FALSE; }
    virtual BOOL SetAiMapItemWeight(H3MapItem *, H3Hero *, const H3Player *, int &, int *, H3Position) const noexcept { return FALSE; }
    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectProperties &info) { return new H3RmgObjectGenerator{info.type, info.subtype}; }
    const List &GetObjectSubtypesInfo() const { return objectSubtypesInfo; }
};
}
#include "WoGTableStubs.h"

static void NativeObjectVisited(size_t address, H3MapItem *item, int player)
{
    assert(address == 0x4FC620);
    if (player >= 0 && player < 8) item->setup |= 1u << (5 + player);
}
#define THISCALL_2(type,address,item,player) NativeObjectVisited(address,item,player)