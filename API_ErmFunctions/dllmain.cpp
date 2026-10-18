#include "framework.h"
#include "ObjectRendering.h"
#include "ObjectReplacement.h"
#include <algorithm>
#include <new>

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace
{
using namespace erm_functions;
constexpr const char *INSTANCE_NAME = "EraPlugin.API_ErmFunctions.daemon_n";
ObjectTypes objectTypes;
ReplayTimeline replay;
std::unordered_map<std::uint32_t, int> owners;
bool hooksReady = false;
bool mapAvailable = false;

H3MainSetup *Map()
{
    auto main = H3Main::Get();
    if (!mapAvailable || !main || !main->mainSetup.mapitems || main->mainSetup.mapSize <= 0 ||
        main->mainSetup.mapSize > 512)
        return nullptr;
    return &main->mainSetup;
}

bool ValidCoordinates(const H3MainSetup &map, int x, int y, int z)
{
    return x >= 0 && y >= 0 && x < map.mapSize && y < map.mapSize && z >= 0 &&
           z <= (map.hasUnderground ? 1 : 0);
}

std::uint32_t Pack(int x, int y, int z) { return H3Position::Pack(x, y, z); }

void OwnerVarName(std::uint32_t coords, char (&name)[64])
{
    // Missing/zero means neutral. ERA owns the variables and includes them in its save section.
    sprintf_s(name, "API_ErmFunctions.Owner.%u", coords);
}

int ReadOwner(std::uint32_t coords)
{
    if (replay.IsReplaying())
    {
        const auto view = replay.ownerView.find(coords);
        if (view != replay.ownerView.end())
            return view->second;
    }
    auto entry = owners.find(coords);
    if (entry == owners.end())
    {
        char name[64];
        OwnerVarName(coords, name);
        const int encoded = Era::GetAssocVarIntValue(name);
        entry = owners.emplace(coords, encoded >= 0 && encoded <= 8 ? encoded - 1 : NO_OWNER).first;
    }
    return entry->second;
}

bool EntranceForDraw(H3MainSetup &map, const H3ObjectDraw &draw, int &x, int &y, int &z,
                     const H3ObjectAttributes *&attributes)
{
    if (draw.sprite >= map.objectDetails.Count())
        return false;
    auto &details = map.objectDetails[draw.sprite];
    if (details.num >= map.objectAttributes.Count())
        return false;
    attributes = &map.objectAttributes[details.num];
    if (!objectTypes.Find(attributes->type, attributes->subtype))
        return false;
    // Verified at 0x410EDA / 0x4FF280: this=H3ObjectDetails, two int output pointers.
    THISCALL_3(void, 0x4FF280, &details, &x, &y);
    z = details.z;
    return ValidCoordinates(map, x, y, z);
}

bool RegisteredEntrance(H3MainSetup &map, int x, int y, int z)
{
    if (!ValidCoordinates(map, x, y, z))
        return false;
    auto tile = map.GetMapItem(x, y, z);
    if (!tile || !tile->IsEntrance())
        return false;
    if (tile->objectType != eObject::HERO && objectTypes.Find(tile->objectType, tile->objectSubtype))
        return true;
    // A hero replaces the tile's type/subtype. The underlying DEF retains its actual subtype.
    const int realType = tile->GetRealType();
    for (const auto &draw : tile->objectDrawing)
    {
        int objectX = -1, objectY = -1, objectZ = -1;
        const H3ObjectAttributes *attributes = nullptr;
        if (EntranceForDraw(map, draw, objectX, objectY, objectZ, attributes) &&
            attributes->type == realType && objectX == x && objectY == y && objectZ == z)
            return true;
    }
    return false;
}

int NativeActionCount()
{
    auto main = H3Main::Get();
    if (!Map())
        return -1;
    // H3Main's protected replayActions is an H3Vector at +4E7AC.
    // Verified by 0x49D4D8..0x49D4EE and all native append/delete routines.
    const auto actions = reinterpret_cast<const H3Vector<void *> *>(reinterpret_cast<const char *>(main) + 0x4E7AC);
    return actions->Count();
}

void __stdcall ResetMap(Era::TEvent *)
{
    erm_functions::replacement::ResetMap();
    mapAvailable = false;
    objectTypes.Clear();
    owners.clear();
    replay.Clear();
}

void __stdcall MapInitialized(Era::TEvent *)
{
    mapAvailable = true;
    erm_functions::rendering::EnsureHdMinimapHooks();
}

void __stdcall MapLoaded(Era::TEvent *)
{
    // Keep registrations made by ERM initialization; only ownership is loaded from assoc vars.
    owners.clear();
    replay.Clear();
    mapAvailable = true;
    erm_functions::rendering::EnsureHdMinimapHooks();
}

class ErmArgumentsScope
{
    int *arguments;
    std::array<int, 16> saved;
public:
    explicit ErmArgumentsScope(const std::array<int, 16> &payload) : arguments(*Era::GetArgXVars())
    {
        std::copy(arguments, arguments + 16, saved.begin());
        std::copy(payload.begin(), payload.end(), arguments);
    }
    ~ErmArgumentsScope() { std::copy(saved.begin(), saved.end(), arguments); }
};

void ApplyReplayEvent(const ReplayEvent &event, bool undo)
{
    if (event.kind == ReplayEvent::OWNER)
        replay.ownerView[event.coords] = undo ? event.oldOwner : event.newOwner;
    else
    {
        const int function = undo ? event.undoFunction : event.redoFunction;
        if (function)
        {
            ErmArgumentsScope arguments(event.args);
            Era::FireErmEvent(function);
        }
    }
}

class ReplayScope
{
    std::uint64_t generation;
public:
    ReplayScope() : generation(replay.Generation()) { replay.Begin(); }
    ~ReplayScope() { replay.End(generation); }
};

void __stdcall ReplayTurn(HiHook *hook, H3Main *main)
{
    if (replay.IsReplaying())
        return;
    ReplayScope scope;
    THISCALL_1(void, hook->GetDefaultFunc(), main);
}

int __stdcall ClearNativeReplay(HiHook *hook, H3Main *main)
{
    replay.Clear();
    return THISCALL_1(int, hook->GetDefaultFunc(), main);
}

_LHF_(UndoReplayBoundary)
{
    replay.AtBoundary(c->esi + 1, true, ApplyReplayEvent);
    return EXEC_DEFAULT;
}
_LHF_(UndoFirstReplayBoundary)
{
    replay.AtBoundary(0, true, ApplyReplayEvent);
    return EXEC_DEFAULT;
}
_LHF_(RedoReplayBoundary)
{
    replay.AtBoundary(c->ebx, false, ApplyReplayEvent);
    return EXEC_DEFAULT;
}
_LHF_(RedoLastReplayBoundary)
{
    replay.AtBoundary(NativeActionCount(), false, ApplyReplayEvent);
    return EXEC_DEFAULT;
}

void __stdcall OnAfterWog(Era::TEvent *)
{
    erm_functions::replacement::InstallHooks(_PI);
    if (hooksReady)
        return;
    erm_functions::rendering::InstallHooks();
    _PI->WriteHiHook(0x49D410, THISCALL_, ReplayTurn);
    _PI->WriteHiHook(0x49D2A0, THISCALL_, ClearNativeReplay);
    _PI->WriteLoHook(0x49D4F8, UndoReplayBoundary);
    _PI->WriteLoHook(0x49D50D, UndoFirstReplayBoundary);
    _PI->WriteLoHook(0x49D5A5, RedoReplayBoundary);
    _PI->WriteLoHook(0x49D608, RedoLastReplayBoundary);
    hooksReady = true;
    erm_functions::rendering::EnsureHdMinimapHooks();
}
}

namespace erm_functions
{
namespace runtime
{
H3MainSetup *GetMap() { return Map(); }
const bool *FindObjectType(int type, int subtype) { return objectTypes.Find(type, subtype); }
int ReadOwnerAtPosition(std::uint32_t position)
{
    const auto map = Map();
    const auto point = H3Position(position).Unpack();
    const int z = (position >> 26) & 0xF;
    return map && ValidCoordinates(*map, point.x, point.y, z) ? ReadOwner(Pack(point.x, point.y, z)) : NO_OWNER;
}
bool ResolveDrawObject(H3MainSetup &map, const H3ObjectDraw &draw, int &x, int &y, int &z,
                       const H3ObjectAttributes *&attributes)
{
    return EntranceForDraw(map, draw, x, y, z, attributes);
}
}
}

extern "C" int __stdcall Erm_RegisterObjectType(int type, int subtype, int drawOnMinimap)
{
    try
    {
        erm_functions::rendering::EnsureHdMinimapHooks();
        return hooksReady && objectTypes.Register(type, subtype, drawOnMinimap);
    }
    catch (const std::bad_alloc &) { return 0; }
}

extern "C" int __stdcall Erm_GetObjectOwner(int x, int y, int z)
{
    try
    {
        auto map = Map();
        return hooksReady && map && RegisteredEntrance(*map, x, y, z) ? ReadOwner(Pack(x, y, z)) : INVALID_OBJECT;
    }
    catch (const std::bad_alloc &) { return INVALID_OBJECT; }
}

extern "C" int __stdcall Erm_SetObjectOwner(int x, int y, int z, int owner)
{
    try
    {
        auto map = Map();
        if (!hooksReady || !map || !ValidOwner(owner) || !RegisteredEntrance(*map, x, y, z))
            return 0;
        const auto coords = Pack(x, y, z);
        const int previousOwner = ReadOwner(coords);
        if (owner == previousOwner)
            return 1;
        if (replay.IsReplaying())
            replay.ownerView[coords] = owner;
        else
        {
            ReplayEvent event;
            event.kind = ReplayEvent::OWNER;
            event.coords = coords;
            event.oldOwner = previousOwner;
            event.newOwner = owner;
            replay.Add(event, NativeActionCount());
            char name[64];
            OwnerVarName(coords, name);
            Era::SetAssocVarIntValue(name, owner + 1);
            owners[coords] = owner;
        }
        return 1;
    }
    catch (const std::bad_alloc &) { return 0; }
}

extern "C" int __stdcall Erm_RegisterReplayEventArgs(int replayFunction, int undoFunction,
                                                    const int *args, int argCount)
{
    if (!hooksReady || !Map() || replayFunction <= 0 || undoFunction < 0 || argCount < 0 || argCount > 16 ||
        (argCount && !args) || replay.IsReplaying())
        return 0;
    try
    {
        ReplayEvent event;
        event.redoFunction = replayFunction;
        event.undoFunction = undoFunction;
        if (argCount)
            std::copy(args, args + argCount, event.args.begin());
        return replay.Add(event, NativeActionCount());
    }
    catch (const std::bad_alloc &) { return 0; }
}

extern "C" int __stdcall Erm_RegisterReplayEvent(int replayFunction, int undoFunction,
                                                int x1, int x2, int x3, int x4)
{
    const int args[] = {x1, x2, x3, x4};
    return Erm_RegisterReplayEventArgs(replayFunction, undoFunction, args, 4);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        globalPatcher = GetPatcher();
        if (globalPatcher->GetInstance(INSTANCE_NAME))
            return TRUE;
        _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
        if (!_PI)
            return FALSE;
        Era::ConnectEra(module, INSTANCE_NAME);
        Era::RegisterHandler(OnAfterWog, "OnAfterWoG");
        Era::RegisterHandler(ResetMap, "OnGameLeave");
        Era::RegisterHandler(ResetMap, "OnGameLeft");
        Era::RegisterHandler(ResetMap, "OnAbnormalGameLeave");
        Era::RegisterHandler(ResetMap, "OnBeforeLoadGame");
        Era::RegisterHandler(MapInitialized, "OnAfterErmInstructions");
        Era::RegisterHandler(MapInitialized, "OnGameEnter");
        Era::RegisterHandler(MapLoaded, "OnAfterLoadGame");
    }
    return TRUE;
}
