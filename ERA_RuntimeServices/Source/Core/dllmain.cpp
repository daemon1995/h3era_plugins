#include "../Config/ConfigServices.h"
#include "../Graphics/GraphicsResources.h"
#include "../Objects/ObjectGlow.h"
#include "../Objects/ObjectRendering.h"
#include "../Objects/ObjectReplacement.h"
#include "../framework.h"
#include "HookGroup.h"
#include <algorithm>
#include <new>

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

extern "C" int __stdcall Erm_SetObjectOwner(int x, int y, int z, int owner) noexcept;

namespace
{
using namespace erm_functions;
constexpr const char *INSTANCE_NAME = "EraPlugin.ERA_RuntimeServices.daemon_n";
constexpr const char *OWNER_EVENT = "ERA_RuntimeServices.ObjectOwner.v1";
using FireRemoteEvent = int(__stdcall *)(int, const char *, const void *, int, void *, void *);
struct OwnerMessage
{
    int version, x, y, z, owner;
};
static_assert(sizeof(OwnerMessage) == 20, "Ownership network ABI");
ObjectTypes objectTypes;
ReplayTimeline replay;
std::vector<OwnerMessage> deferredOwners;
std::vector<OwnerReplayRecord> loadedOwnerReplay;
int loadedNativeCount = -1;
constexpr const char *OWNER_REPLAY_SECTION = "ERA_RuntimeServices.OwnerReplay.v1";
std::unordered_map<std::uint32_t, int> owners;
bool hooksReady = false;
bool replayReady = false;
bool mapAvailable = false;
std::uint64_t mapGeneration = 0;

H3MainSetup *Map() noexcept
{
    if (!mapAvailable)
        return nullptr;
    auto &map = H3Main::Get()->mainSetup;
    return map.mapitems ? &map : nullptr;
}

bool ValidCoordinates(const H3MainSetup &map, int x, int y, int z) noexcept
{
    return x >= 0 && y >= 0 && x < map.mapSize && y < map.mapSize && z >= 0 && z <= (map.hasUnderground ? 1 : 0);
}

std::uint32_t Pack(int x, int y, int z) noexcept
{
    return H3Position::Pack(x, y, z);
}

void OwnerVarName(std::uint32_t coords, char (&name)[64]) noexcept
{
    // Missing/zero means neutral. ERA owns the variables and includes them in its save section.
    sprintf_s(name, "ERA_RuntimeServices.Owner.%u", coords);
}

int ReadOwner(std::uint32_t coords) noexcept
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
        const int owner = encoded >= 0 && encoded <= 8 ? encoded - 1 : NO_OWNER;
        try { owners.emplace(coords, owner); }
        catch (const std::bad_alloc &) {}
        return owner;
    }
    return entry->second;
}

bool EntranceForDraw(H3MainSetup &map, const H3ObjectDraw &draw, int &x, int &y, int &z,
                     const H3ObjectAttributes *&attributes, bool registeredOnly = true) noexcept
{
    if (draw.sprite >= map.objectDetails.Count())
        return false;
    auto &details = map.objectDetails[draw.sprite];
    if (details.num >= map.objectAttributes.Count())
        return false;
    attributes = &map.objectAttributes[details.num];
    if (registeredOnly && !objectTypes.Find(attributes->type, attributes->subtype))
        return false;
    // Verified at 0x410EDA / 0x4FF280: this=H3ObjectDetails, two int output pointers.
    THISCALL_3(void, 0x4FF280, &details, &x, &y);
    z = details.z;
    return ValidCoordinates(map, x, y, z);
}

bool RegisteredEntrance(H3MainSetup &map, int x, int y, int z) noexcept
{
    if (!ValidCoordinates(map, x, y, z))
        return false;
    auto tile = map.GetMapItem(x, y, z);
    if (!tile || !tile->IsEntrance())
        return false;
    if (tile->objectType != eObject::HERO && objectTypes.Find(tile->objectType, tile->objectSubtype))
        return true;
    // A remote hero can have objectBelow=0 even though the object's DEF is
    // still present at its entrance. GetRealType() then returns HERO, so use
    // the registered DEF and its exact entrance instead of the hero's flags.
    for (const auto &draw : tile->objectDrawing)
    {
        int objectX = -1, objectY = -1, objectZ = -1;
        const H3ObjectAttributes *attributes = nullptr;
        if (EntranceForDraw(map, draw, objectX, objectY, objectZ, attributes) &&
            (tile->objectType == eObject::HERO || attributes->type == tile->objectType) && objectX == x &&
            objectY == y && objectZ == z)
            return true;
    }
    return false;
}

int NativeActionCount() noexcept
{
    auto main = H3Main::Get();
    if (!Map())
        return -1;
    // H3Main's protected replayActions is an H3Vector at +4E7AC.
    // Verified by 0x49D4D8..0x49D4EE and all native append/delete routines.
    const auto actions = reinterpret_cast<const H3Vector<void *> *>(reinterpret_cast<const char *>(main) + 0x4E7AC);
    return actions->Count();
}

void RedrawOwnership() noexcept
{
    // A remote FU:D changes no native map item, so the game does not know
    // that its already drawn DEF/minimap color has become stale.
    auto map = Map();
    auto adventure = H3AdventureManager::Get();
    if (map && adventure && adventure->map == map && adventure->dlg && !IntAt(0x698A44))
    {
        // Replay temporarily switches the active player. FullUpdate would
        // rebuild hero/town lists and the info panel for that foreign player.
        // Reuse only its map/minimap calls, with updateInfoPanel disabled.
        const auto position = adventure->screenPosition.Mixed();
        const int x = (int(position & 0x3FF) ^ 0x200) - 0x200;
        const int y = (int((position >> 16) & 0x3FF) ^ 0x200) - 0x200;
        THISCALL_6(void, 0x40F350, adventure, x, y, int(adventure->screenPosition.GetZ()), 0, 0);
        THISCALL_7(void, 0x412BA0, adventure, position, 0, 1, 0, 0, 0);
        H3WindowManager::Get()->H3Redraw(0, 0, H3GameWidth::Get(), H3GameHeight::Get());
    }
}

void ApplyReceivedOwner(const OwnerMessage &message) noexcept
{
    Erm_SetObjectOwner(message.x, message.y, message.z, message.owner);
}

void __stdcall ReceiveObjectOwner(Era::TEvent *event) noexcept
{
    if (!event || !event->Data || event->DataSize != sizeof(OwnerMessage))
        return;
    OwnerMessage message;
    std::memcpy(&message, event->Data, sizeof(message));
    if (message.version != 1)
        return;
    if (replay.IsReplaying())
    {
        // A peer can keep playing while this client watches a replay.
        // Apply to the live map after the temporary replay view is gone.
        try { deferredOwners.push_back(message); }
        catch (const std::bad_alloc &) {}
        return;
    }
    ApplyReceivedOwner(message);
}

void __stdcall ResetMap(Era::TEvent *) noexcept
{
    ++mapGeneration;
    erm_functions::replacement::ResetMap();
    erm_functions::glow::ResetMap();
    mapAvailable = false;
    objectTypes.Clear();
    owners.clear();
    deferredOwners.clear();
    loadedOwnerReplay.clear();
    loadedNativeCount = -1;
    replay.Clear();
}

void __stdcall MapInitialized(Era::TEvent *) noexcept
{
    mapAvailable = true;
    erm_functions::rendering::EnsureHdMinimapHooks();
}

void __stdcall MapLoaded(Era::TEvent *) noexcept
{
    // Keep registrations made by ERM initialization; only ownership is loaded from assoc vars.
    owners.clear();
    deferredOwners.clear();
    mapAvailable = true;
    try
    {
        if (loadedNativeCount == NativeActionCount())
            replay.RestoreOwners(loadedOwnerReplay, loadedNativeCount);
    }
    catch (const std::bad_alloc &) {}
    loadedOwnerReplay.clear();
    loadedNativeCount = -1;
    erm_functions::rendering::EnsureHdMinimapHooks();
}

void __stdcall SaveOwnerReplay(Era::TEvent *) noexcept
{
    int count = 0;
    if (!replay.IsReplaying())
        for (const auto &event : replay.Events())
            if (event.kind == ReplayEvent::OWNER)
                ++count;
    if (count > 1000000)
        return;
    // Stream records directly; no temporary vector allocation during saving.
    int header[] = {1, NativeActionCount(), count, sizeof(OwnerReplayRecord)};
    Era::WriteSavegameSection(sizeof(header), header, OWNER_REPLAY_SECTION);
    if (count)
        for (const auto &event : replay.Events())
            if (event.kind == ReplayEvent::OWNER)
            {
                OwnerReplayRecord record{event.boundary, event.coords, event.oldOwner, event.newOwner};
                Era::WriteSavegameSection(sizeof(record), &record, OWNER_REPLAY_SECTION);
            }
}

void __stdcall LoadOwnerReplay(Era::TEvent *) noexcept
{
    loadedOwnerReplay.clear();
    loadedNativeCount = -1;
    int header[4]{};
    if (Era::ReadSavegameSection(sizeof(header), header, OWNER_REPLAY_SECTION) != sizeof(header))
        return; // Older saves have no ownership history to reconstruct.
    if (header[0] != 1 || header[1] < 0 || header[2] < 0 || header[2] > 1000000 ||
        header[3] != sizeof(OwnerReplayRecord))
        return;
    try
    {
        std::vector<OwnerReplayRecord> records(header[2]);
        const int bytes = int(records.size() * sizeof(OwnerReplayRecord));
        if (bytes && Era::ReadSavegameSection(bytes, records.data(), OWNER_REPLAY_SECTION) != bytes)
            return;
        loadedOwnerReplay.swap(records);
        loadedNativeCount = header[1];
    }
    catch (const std::bad_alloc &) {}
}

class ErmArgumentsScope
{
    int *arguments;
    std::array<int, 16> saved;

  public:
    explicit ErmArgumentsScope(const std::array<int, 16> &payload) noexcept : arguments(*Era::GetArgXVars())
    {
        std::copy(arguments, arguments + 16, saved.begin());
        std::copy(payload.begin(), payload.end(), arguments);
    }
    ~ErmArgumentsScope() noexcept
    {
        std::copy(saved.begin(), saved.end(), arguments);
    }
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
    ReplayScope() noexcept : generation(replay.Generation())
    {
        replay.Begin();
    }
    ~ReplayScope() noexcept
    {
        replay.End(generation);
    }
};

void __stdcall ReplayTurn(HiHook *hook, H3Main *main)
{
    if (replay.IsReplaying())
        return;
    {
        ReplayScope scope;
        THISCALL_1(void, hook->GetDefaultFunc(), main);
    }
    auto received = std::move(deferredOwners);
    deferredOwners.clear();
    const auto generation = mapGeneration;
    for (const auto &message : received)
    {
        if (generation != mapGeneration)
            break;
        ApplyReceivedOwner(message);
    }
}

int __stdcall ClearNativeReplay(HiHook *hook, H3Main *main)
{
    replay.Clear();
    return THISCALL_1(int, hook->GetDefaultFunc(), main);
}

int __stdcall PruneNativeReplay(HiHook *hook, H3Main *main, int player)
{
    const int before = NativeActionCount();
    const auto generation = replay.Generation();
    const int result = THISCALL_2(int, hook->GetDefaultFunc(), main, player);
    const int after = NativeActionCount();
    if (generation == replay.Generation() && before >= 0 && after >= 0 && after < before)
        replay.RemoveNativePrefix(before - after);
    return result;
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
    // Boundary zero precedes the first native action. Other boundaries are
    // applied after the preceding action, before the next animation/message pump.
    if (c->ebx == 0)
        replay.AtBoundary(0, false, ApplyReplayEvent);
    return EXEC_DEFAULT;
}
_LHF_(RedoCompletedReplayBoundary)
{
    bool ownerChanged = false;
    replay.AtBoundary(c->ebx + 1, false, [&ownerChanged](const ReplayEvent &event, bool undo) {
        ApplyReplayEvent(event, undo);
        ownerChanged |= event.kind == ReplayEvent::OWNER;
    });
    if (ownerChanged)
        RedrawOwnership();
    return EXEC_DEFAULT;
}
_LHF_(RedoLastReplayBoundary)
{
    // With no native actions boundary zero has not passed through the loop.
    if (NativeActionCount() == 0)
        replay.AtBoundary(0, false, ApplyReplayEvent);
    return EXEC_DEFAULT;
}

int EntranceIdentity(H3MainSetup &map, int x, int y, int z)
{
    if (!ValidCoordinates(map, x, y, z))
        return 0;
    const auto tile = map.GetMapItem(x, y, z);
    if (!tile || !tile->IsEntrance())
        return 0;
    for (const auto &draw : tile->objectDrawing)
    {
        int ox, oy, oz;
        const H3ObjectAttributes *attributes = nullptr;
        if (EntranceForDraw(map, draw, ox, oy, oz, attributes) && ox == x && oy == y && oz == z)
            return draw.sprite + 1;
    }
    return 0;
}

void __cdecl DeleteOwnedObject(HiHook *hook, int x, int y, int z)
{
    auto map = Map();
    const auto generation = mapGeneration;
    const int identity = map ? EntranceIdentity(*map, x, y, z) : 0;
    const int previousOwner = identity ? ReadOwner(Pack(x, y, z)) : NO_OWNER;
    CDECL_3(void, hook->GetDefaultFunc(), x, y, z);
    // UN:O and the replacement API use this same native deletion entry. Do not
    // erase an owner if deletion was rejected, or touch a map loaded by a hook.
    if (!identity || Map() != map || generation != mapGeneration || EntranceIdentity(*map, x, y, z) == identity)
        return;
    const auto coords = Pack(x, y, z);
    try
    {
        if (replay.IsReplaying())
        {
            // Replay is a temporary view and must never rewrite saved ownership.
            replay.ownerView[coords] = NO_OWNER;
            return;
        }
        if (replayReady && previousOwner != NO_OWNER)
        {
            ReplayEvent event;
            event.kind = ReplayEvent::OWNER;
            event.coords = coords;
            event.oldOwner = previousOwner;
            event.newOwner = NO_OWNER;
            replay.Add(event, NativeActionCount());
        }
    }
    catch (const std::bad_alloc &)
    {
        if (replay.IsReplaying())
            return;
    }
    char name[64];
    OwnerVarName(coords, name);
    Era::SetAssocVarIntValue(name, 0);
    owners.erase(coords);
}

void __stdcall OnReportVersion(Era::TEvent *)
{
    Era::ReportPluginVersion("{ERA Runtime Services} v1.0.0-rc1 (" __DATE__ ")");
}

void __stdcall OnAfterWog(Era::TEvent *)
{
    erm_functions::replacement::InstallHooks(_PI);
    erm_functions::glow::InstallHooks();
    if (!hooksReady && erm_functions::rendering::InstallHooks())
    {
        static const unsigned char deleteCall[] = {0xE8, 0x2F, 0x13, 0xFE, 0xFF, 0x83, 0xC4, 0x0C};
        if (std::memcmp(reinterpret_cast<void *>(0x731856), deleteCall, sizeof deleteCall) == 0)
        {
            Patch *hooks[] = {_PI->WriteHiHook(0x712B8A, CDECL_, DeleteOwnedObject)};
            hooksReady = CheckHooks(hooks, "Ownership lifecycle");
        }
        else
            Log("Ownership", "Unknown native deletion contract; ownership API disabled.");
    }
    if (!replayReady)
    {
        Patch *hooks[] = {_PI->WriteHiHook(0x49D410, THISCALL_, ReplayTurn),
                          _PI->WriteHiHook(0x49D2A0, THISCALL_, ClearNativeReplay),
                          _PI->WriteHiHook(0x49D330, THISCALL_, PruneNativeReplay),
                          _PI->WriteLoHook(0x49D4F8, UndoReplayBoundary),
                          _PI->WriteLoHook(0x49D50D, UndoFirstReplayBoundary),
                          _PI->WriteLoHook(0x49D5A5, RedoReplayBoundary),
                          _PI->WriteLoHook(0x49D5B7, RedoCompletedReplayBoundary),
                          _PI->WriteLoHook(0x49D608, RedoLastReplayBoundary)};
        replayReady = CheckHooks(hooks, "Replay");
    }
    erm_functions::rendering::EnsureHdMinimapHooks();
}
} // namespace

namespace erm_functions
{
namespace runtime
{
H3MainSetup *GetMap()
{
    return Map();
}
const bool *FindObjectType(int type, int subtype)
{
    return objectTypes.Find(type, subtype);
}
int ReadOwnerAtPosition(std::uint32_t position)
{
    const auto map = Map();
    const auto point = H3Position(position).Unpack();
    const int z = (position >> 26) & 0xF;
    return map && ValidCoordinates(*map, point.x, point.y, z) ? ReadOwner(Pack(point.x, point.y, z)) : NO_OWNER;
}
bool ResolveDrawObject(H3MainSetup &map, const H3ObjectDraw &draw, int &x, int &y, int &z,
                       const H3ObjectAttributes *&attributes, bool registeredOnly)
{
    return EntranceForDraw(map, draw, x, y, z, attributes, registeredOnly);
}
} // namespace runtime
} // namespace erm_functions

extern "C" int __stdcall Erm_RegisterObjectType(int type, int subtype, int drawOnMinimap) noexcept
{
    try
    {
        erm_functions::rendering::EnsureHdMinimapHooks();
        return hooksReady && objectTypes.Register(type, subtype, drawOnMinimap);
    }
    catch (const std::bad_alloc &)
    {
        return 0;
    }
}

extern "C" int __stdcall Erm_GetObjectOwner(int x, int y, int z) noexcept
{
    auto map = Map();
    return hooksReady && map && RegisteredEntrance(*map, x, y, z) ? ReadOwner(Pack(x, y, z)) : INVALID_OBJECT;
}

extern "C" int __stdcall Erm_SetObjectOwner(int x, int y, int z, int owner) noexcept
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
            auto entry = owners.emplace(coords, previousOwner).first;
            ReplayEvent event;
            event.kind = ReplayEvent::OWNER;
            event.coords = coords;
            event.oldOwner = previousOwner;
            event.newOwner = owner;
            if (replayReady && !replay.Add(event, NativeActionCount()))
                return 0;
            char name[64];
            OwnerVarName(coords, name);
            Era::SetAssocVarIntValue(name, owner + 1);
            entry->second = owner;
        }
        RedrawOwnership();
        return 1;
    }
    catch (const std::bad_alloc &) { return 0; }
}

extern "C" int __stdcall Erm_SetObjectOwnerAndSync(int x, int y, int z, int owner) noexcept
{
    const bool send = H3Network::Multiplayer() && !replay.IsReplaying();
    const auto remote =
        send ? reinterpret_cast<FireRemoteEvent>(GetProcAddress(GetModuleHandleA("era.dll"), "FireRemoteEvent"))
             : nullptr;
    if (send && !remote)
        return 0;
    if (!Erm_SetObjectOwner(x, y, z, owner))
        return 0;
    if (!send)
        return 1;
    const OwnerMessage message{1, x, y, z, owner};
    // The receiver calls the local setter, never this export: no echo packets.
    return remote(-1, OWNER_EVENT, &message, sizeof(message), nullptr, nullptr) != 0;
}

extern "C" int __stdcall Erm_RegisterReplayEventArgs(int replayFunction, int undoFunction, const int *args,
                                                     int argCount) noexcept
{
    if (!replayReady || !Map() || replayFunction <= 0 || undoFunction < 0 || argCount < 0 || argCount > 16 ||
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
    catch (const std::bad_alloc &)
    {
        return 0;
    }
}

extern "C" int __stdcall Erm_RegisterReplayEvent(int replayFunction, int undoFunction, int x1, int x2, int x3,
                                                 int x4) noexcept
{
    const int args[] = {x1, x2, x3, x4};
    return Erm_RegisterReplayEventArgs(replayFunction, undoFunction, args, 4);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    static bool initialized = false;
    if (reason == DLL_PROCESS_ATTACH && !initialized)
    {
        initialized = true;
        globalPatcher = GetPatcher();
        if (!globalPatcher)
            return FALSE;
        if (globalPatcher->GetInstance(INSTANCE_NAME))
            return FALSE;
        _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
        if (!_PI)
            return FALSE;
        Era::ConnectEra(module, INSTANCE_NAME);
        config_services::Initialize();
        text_services::Initialize();
        graphics_resources::RegisterEvents();
        Era::RegisterHandler(OnReportVersion, "OnReportVersion");
        Era::RegisterHandler(ReceiveObjectOwner, OWNER_EVENT);
        Era::RegisterHandler(OnAfterWog, "OnAfterWoG");
        Era::RegisterHandler(ResetMap, "OnGameLeave");
        Era::RegisterHandler(ResetMap, "OnGameLeft");
        Era::RegisterHandler(ResetMap, "OnAbnormalGameLeave");
        Era::RegisterHandler(ResetMap, "OnBeforeLoadGame");
        Era::RegisterHandler(MapInitialized, "OnAfterErmInstructions");
        Era::RegisterHandler(MapInitialized, "OnGameEnter");
        Era::RegisterHandler(MapLoaded, "OnAfterLoadGame");
        Era::RegisterHandler(SaveOwnerReplay, "OnSavegameWrite");
        Era::RegisterHandler(LoadOwnerReplay, "OnSavegameRead");
    }
    return TRUE;
}
