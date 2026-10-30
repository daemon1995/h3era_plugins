#pragma once
#include <map>
using INT16 = short;
using LPCSTR = const char *;
using H3Position = unsigned long;
struct LoHook {};
struct HookContext
{
    size_t ebx = 0;
    size_t ebp = 0;
    size_t eax = 0;
    size_t return_address = 0;
    template <typename T> T Ebx() const { return reinterpret_cast<T>(ebx); }
};
static DWORD DwordAt(size_t address) { return *reinterpret_cast<const DWORD *>(address); }
constexpr int EXEC_DEFAULT = 1;
constexpr int NO_EXEC_DEFAULT = 0;
#define _LHF_(name) int __stdcall name(LoHook *h, HookContext *c)
using FakeHook = int(__stdcall *)(LoHook *, HookContext *);
struct PatcherInstance
{
    std::map<size_t, FakeHook> hooks;
    void WriteLoHook(size_t address, FakeHook hook) { hooks[address] = hook; }
};
static PatcherInstance fakePatcher;
static PatcherInstance *_PI = &fakePatcher;
struct H3RmgObjectGenerator { int type; int subtype; };

namespace extender
{
constexpr int WOG_OBJECT_TYPE = 63;
constexpr int ERA_OBJECT_TYPE = 141;
enum eRmgDlgObjectPage { ePageUnknown = -1, ePageCommon=0, ePageWogObject = 7 };
struct UniqueObjectInfo { int type; int subtype; int aiScoutingWeight; eRmgDlgObjectPage page; };
struct RMGObjectProperties
{
    int type;
    int subtype;
    RMGObjectProperties(int type, int subtype) : type(type), subtype(subtype) {}
};
class ObjectExtender
{
  protected:
    struct InfoList : std::vector<UniqueObjectInfo>
    {
        void Reserve(size_t size) { reserve(size); }
    } objectSubtypesInfo;
    BOOL m_isInited = FALSE;
    PatcherInstance *_pi;
    void ReleaseObjectSubtypesInfo() { objectSubtypesInfo.clear(); }
    virtual void CreatePatches() { m_isInited = TRUE; }

  public:
    explicit ObjectExtender(PatcherInstance *pi) : _pi(pi) {}
    virtual ~ObjectExtender() {}
    virtual void AfterLoadingObjectsTxtProc(const INT16 *) {}
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectProperties &, BOOL) const noexcept { return nullptr; }
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *, int, int) const noexcept { return FALSE; }
    virtual BOOL VisitMapItem(H3Hero *, H3MapItem *, H3Position, BOOL) const noexcept { return FALSE; }
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *, const H3Hero *, int, BOOL) const noexcept { return FALSE; }
    virtual BOOL SetAiMapItemWeight(H3MapItem *, H3Hero *, const H3Player *, int &, int *, H3Position) const noexcept
    { return FALSE; }
    void AddUniqueObjectInfo(int type, int subtype, int aiScoutingWeight, eRmgDlgObjectPage page)
    { objectSubtypesInfo.push_back({type, subtype, aiScoutingWeight, page}); }
    const InfoList &GetObjectSubtypesInfo() const { return objectSubtypesInfo; }
    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectProperties &info)
    { return new H3RmgObjectGenerator{info.type, info.subtype}; }
};
}
