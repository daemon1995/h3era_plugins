#include "../framework.h"
#include "ObjectRendering.h"
#include "ObjectReplacement.h"
#include "ObjectGeometry.h"
#include "ObjectShapeCache.h"
#include "SpecialTerrainGeometry.h"
#include "../Core/HookGroup.h"
#include <cstring>
#include <string>

namespace
{
using namespace h3;
using namespace erm_functions;
using namespace geometry;
bool ready = false;
bool terrainNetworkWarning = false;
ShapeCache shapeCache;
std::uint64_t mapGeneration = 0;
constexpr unsigned NATIVE_DELETE_OBJECT = 0x712B8A;

// The same cdecl WoG entry called by UN:O(.../0/0), including any native hooks.
using NativeDeleteFunction = void(__cdecl *)(int, int, int);
void DeleteMapObject(int x, int y, int z,
                     NativeDeleteFunction remove = reinterpret_cast<NativeDeleteFunction>(NATIVE_DELETE_OBJECT))
{
    remove(x, y, z);
}

struct Source
{
    int detailsIndex = -1, attributesIndex = -1;
    int ground = -1;
    const Shape *shape = nullptr;
};

struct Selection
{
    H3MainSetup *map = nullptr;
    int type = -1, subtype = -1, attribute = -1;
    int originX = -1, originY = -1, z = -1;
    int entranceColumn = -1, entranceRow = -1;
};
Selection *active = nullptr;

bool Coordinates(const H3MainSetup &map, int x, int y, int z)
{
    return x >= 0 && y >= 0 && x < map.mapSize && y < map.mapSize && z >= 0 &&
           z <= (map.hasUnderground ? 1 : 0);
}

const Shape *ShapeOf(const H3ObjectAttributes &attributes)
{
    return shapeCache.Get(attributes.width, attributes.height, attributes.passability.m_bitsPacked,
                          attributes.entrances.m_bitsPacked, attributes.colors.m_bitsPacked);
}

bool ResolveSource(H3MainSetup &map, int x, int y, int z, Source &source)
{
    if (!Coordinates(map, x, y, z)) return false;
    const auto tile = map.GetMapItem(x, y, z);
    if (!tile || !tile->IsEntrance() || tile->objectType == eObject::HERO) return false;
    for (const auto &draw : tile->objectDrawing)
    {
        if (draw.sprite >= map.objectDetails.Count()) continue;
        const auto &details = map.objectDetails[draw.sprite];
        if (details.num >= map.objectAttributes.Count() || details.z != z) continue;
        const auto &attributes = map.objectAttributes[details.num];
        if (attributes.type != tile->objectType || attributes.subtype != tile->objectSubtype) continue;
        const auto shape = ShapeOf(attributes);
        if (!shape || details.x - shape->entranceColumn != x ||
            details.y - shape->entranceRow != y) continue;
        source.detailsIndex = draw.sprite;
        source.attributesIndex = details.num;
        source.ground = tile->land;
        source.shape = shape;
        return true;
    }
    return false;
}

struct MapWorld
{
    H3MainSetup &map;
    const Source &source;
    int x, y, z;
    bool water;
    bool InBounds(int px, int py) const { return Coordinates(map, px, py, z); }
    bool ForeignEntrance(int px, int py) const
    {
        const auto tile = map.GetMapItem(px, py, z);
        if (!tile->IsEntrance()) return false;
        const Cell relative{px - x, py - y};
        if (std::find(source.shape->entrances.begin(), source.shape->entrances.end(), relative) ==
            source.shape->entrances.end()) return true;
        const auto &attributes = map.objectAttributes[source.attributesIndex];
        return tile->objectType != attributes.type || tile->objectSubtype != attributes.subtype;
    }
    bool CanApproach(int px, int py) const
    {
        if (!InBounds(px, py)) return false;
        const auto tile = map.GetMapItem(px, py, z);
        return tile && !tile->IsBlocked() && !tile->IsEntrance() && tile->land != 9 &&
               ((tile->land == 8) == water);
    }
};

bool ValidTarget(int type, int subtype, int terrain, unsigned approachMask)
{
    return type >= 0 && type <= 231 && subtype >= 0 && subtype <= 32767 &&
           terrain >= -1 && terrain <= 9 && approachMask && approachMask <= 255;
}

bool GroundAllowed(const Source &source, int terrain)
{
    return source.ground >= 0 && source.ground < 9 && (terrain < 0 || source.ground == terrain);
}

bool Candidate(H3MainSetup &map, const Source &source, int x, int y, int z,
               int type, int subtype, unsigned approachMask, int index,
               const Shape *&shape, Placement &placement)
{
    auto &list = map.objectLists[type];
    if (index < 0 || static_cast<unsigned>(index) >= list.Count()) return false;
    const auto &attributes = list[index];
    if (attributes.type != type || attributes.subtype != subtype ||
        !attributes.maskTerrain.bitfield.GetState(source.ground)) return false;
    shape = ShapeOf(attributes);
    if (!shape) return false;
    return CheckPlacement(*source.shape, *shape, x, y, approachMask,
                          MapWorld{map, source, x, y, z, source.ground == 8}, placement) &&
           placement.originX <= 255 && placement.originY <= 255;
}

// Prepare the exact prototype through the game's normal loader, before deleting any source.
// Only competing subtype values are hidden, and only during this synchronous selector call.
using PrototypeScope = ScopedPrototypeSelection<H3Vector<H3ObjectAttributes>>;

void __stdcall SelectTemplate(HiHook *hook, H3MainSetup *map, H3ObjectDetails *details,
                             int type, int subtype, int terrain)
{
    if (active && active->map == map && active->type == type && active->subtype == subtype &&
        details->x == active->originX && details->y == active->originY && details->z == active->z)
    {
        details->num = static_cast<UINT16>(active->attribute);
        return;
    }
    THISCALL_5(void, hook->GetDefaultFunc(), map, details, type, subtype, terrain);
}

// CDECL_ bridges require a __cdecl callback too; __stdcall corrupts the caller frame.
int __cdecl FindEntranceShift(HiHook *hook, int type, int subtype, int *dx, int *dy, int silence)
{
    if (active && active->type == type && active->subtype == subtype && dx && dy)
    {
        *dx = active->entranceColumn;
        *dy = active->entranceRow;
        return 1;
    }
    return CDECL_5(int, hook->GetDefaultFunc(), type, subtype, dx, dy, silence);
}

class SelectionScope
{
public:
    explicit SelectionScope(Selection &selection) { active = &selection; }
    ~SelectionScope() { active = nullptr; }
};

class ArgumentsScope
{
    int *arguments;
    std::array<int, 16> saved;
public:
    ArgumentsScope() : arguments(*Era::GetArgXVars())
    {
        std::copy(arguments, arguments + 16, saved.begin());
    }
    ~ArgumentsScope() { std::copy(saved.begin(), saved.end(), arguments); }
};

bool LandSpecialTerrain(int type)
{
    return type == eObject::CURSED_GROUND || type == eObject::MAGIC_PLAINS1 ||
           (type >= eObject::CLOVER_FIELD && type <= eObject::ROCKLANDS && type != eObject::FAVORABLE_WINDS);
}

bool UnderlayFits(H3MainSetup &map, const H3ObjectAttributes &a, int x, int y, int z)
{
    if (!LandSpecialTerrain(a.type) || a.subtype ||
        !PassableUnderlay(a.width, a.height, a.flat != 0, a.passability.m_bitsPacked, a.entrances.m_bitsPacked) ||
        !Coordinates(map, x, y, z) || x > 255 || y > 255 || x + 1 < a.width || y + 1 < a.height) return false;
    for (int dy = 0; dy < a.height; ++dy) for (int dx = 0; dx < a.width; ++dx)
    {
        const auto tile = map.GetMapItem(x - dx, y - dy, z);
        if (!tile || tile->land >= 8 || !a.maskTerrain.bitfield.GetState(tile->land)) return false;
        for (const auto &draw : tile->objectDrawing)
        {
            if (draw.sprite >= map.objectDetails.Count()) return false;
            const auto num = map.objectDetails[draw.sprite].num;
            if (num >= map.objectAttributes.Count()) return false;
            const int type = map.objectAttributes[num].type;
            if (LandSpecialTerrain(type) || type == eObject::FAVORABLE_WINDS) return false;
        }
    }
    return true;
}

// The centered API is replicated by its ERM caller with P/D. Suppress ERA's
// automatic placement event, which would otherwise create each soil twice and
// cannot carry the exact selected DEF. This is ERA's own remote-placement flag;
// no code bytes or general remote-event sender are modified.
bool LocalTerrainSendFlag(BYTE *&flag)
{
    flag = nullptr;
    if (!IntAt(0x69959C)) return true; // Single player and hotseat have no remote peers.
    const auto base = reinterpret_cast<BYTE *>(GetModuleHandleA("era.dll"));
    if (!base) return false;
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 64 || dos->e_lfanew > 4096) return false;
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage <= 0xB0008) return false;
    // Installed ERA 3.9.31: Tweaks.Hook_ZvsPlaceMapObject. Absolute operands are
    // relocated by the loader; verify the flag points inside this same module.
    static const BYTE head[] = {0x55,0x8B,0xEC,0x83,0xC4,0xA0,0x53,0x56,0x57,0x8B,0x7D,0x14,0x8B,0x75,0x10,0x8B,0x5D,0x0C};
    static const BYTE body[] = {0x6A,0x07,0x89,0x5D,0xA0,0xC6,0x45,0xA4,0x00};
    const auto test = base + 0x7161E;
    if (std::memcmp(base + 0x7160C, head, sizeof head) || std::memcmp(base + 0x71664, body, sizeof body) ||
        test[0] != 0x80 || test[1] != 0x3D || test[6] != 0 ||
        (test[7] != 0x74 && test[7] != 0xEB) || test[8] != 0x3D) return false;
    BYTE *candidate = nullptr;
    std::memcpy(&candidate, test + 2, sizeof candidate);
    if (candidate != base + 0xB0008 || *candidate > 1) return false;
    flag = candidate;
    return true;
}

class LocalTerrainSendScope
{
    BYTE *flag;
    BYTE previous;
public:
    explicit LocalTerrainSendScope(BYTE *value) : flag(value), previous(value ? *value : 0)
    { if (flag) *flag = 0; }
    ~LocalTerrainSendScope() { if (flag) *flag = previous; }
};

int CommitUnderlay(H3MainSetup &map, H3ObjectDetails prepared, std::uint64_t generation, BYTE *sendFlag = nullptr)
{
    if (prepared.num >= map.objectAttributes.Count() || map.objectDetails.Count() >= 65535) return 0;
    const auto &a = map.objectAttributes[prepared.num];
    const int x = prepared.x, y = prepared.y, z = prepared.z, type = a.type;
    const int width = a.width, height = a.height;
    if (!UnderlayFits(map, a, x, y, z)) return 0;
    const auto before = map.objectDetails.Count();
    Selection selection{&map, type, 0, prepared.num, x, y, z, 0, 0};
    {
        SelectionScope scope(selection);
        LocalTerrainSendScope local(sendFlag);
        CDECL_8(int, 0x71299E, x, y, z, type, 0, type, 0, -1);
    }
    if (runtime::GetMap() != &map || generation != mapGeneration) return -1;
    if (map.objectDetails.Count() == before) return 0;
    if (map.objectDetails.Count() != before + 1) return -1;
    const auto &added = map.objectDetails[before];
    if (added.num != prepared.num || added.x != x || added.y != y || added.z != z) return -1;
    for (int dy = 0; dy < height; ++dy) for (int dx = 0; dx < width; ++dx)
    {
        const auto &draws = map.GetMapItem(x - dx, y - dy, z)->objectDrawing;
        if (std::count_if(draws.begin(), draws.end(),
            [before](const H3ObjectDraw &draw) { return draw.sprite == before; }) != 1) return -1;
    }
    for (int dy = 0; dy < height; ++dy) for (int dx = 0; dx < width; ++dx)
    {
        auto &draws = map.GetMapItem(x - dx, y - dy, z)->objectDrawing;
        const auto it = std::find_if(draws.begin(), draws.end(),
            [before](const H3ObjectDraw &draw) { return draw.sprite == before; });
        std::rotate(draws.begin(), it, it + 1);
    }
    return 1;
}

// Temporary diagnostic for reproducing special-terrain generation failures.
struct SoilTrace
{
    int attempts, z, size = -1, underground = -1, result = -1, processed = 0, placed = 0;
    int terrain = 0, occupied = 0, noType = 0, def = 0, bounds = 0, mixed = 0, rock = 0, water = 0, mask = 0, overlap = 0, unreachable = 0, native = 0, anchors = 0;
    int x = -1, y = -1, type = -1;
    const char *reason = "exception";
    DWORD start = GetTickCount();
    SoilTrace(int a, int level) : attempts(a), z(level)
    {
        char text[128];
        sprintf_s(text, "begin z=%d attempts=%d", z, attempts);
        Log("Soil diagnostic", text);
    }
    int Finish(int value, const char *why) { result = value; reason = why; return value; }
    ~SoilTrace()
    {
        char text[512];
        sprintf_s(text, "end z=%d size=%d underground=%d attempts=%d processed=%d result=%d placed=%d reason=%s ms=%lu anchors=%d terrain=%d occupied=%d no_type=%d def=%d bounds=%d mixed=%d rock=%d water=%d mask=%d overlap=%d unreachable=%d native=%d last=%d,%d type=%d",
            z, size, underground, attempts, processed, result, placed, reason, GetTickCount() - start,
            anchors, terrain, occupied, noType, def, bounds, mixed, rock, water, mask, overlap, unreachable, native, x, y, type);
        Log("Soil diagnostic", text);
    }
};
}

namespace erm_functions
{
namespace replacement
{
void ResetMap() { active = nullptr; ++mapGeneration; shapeCache.Clear(); terrainNetworkWarning = false; }

void InstallHooks(PatcherInstance *patcher)
{
    if (ready) return;
    // Stable call and argument layout, verified against local h3era.exe. Do not inspect a
    // shared HiHook's prologue, which other compatible plugins may already have patched.
    static const unsigned char selectCall[] = {0xE8,0x83,0xCF,0x03,0x00};
    static const unsigned char shiftCall[] = {0xE8,0x7D,0x02,0x00,0x00};
    static const unsigned char deleteCall[] = {0xE8,0x2F,0x13,0xFE,0xFF,0x83,0xC4,0x0C};
    if (std::memcmp(reinterpret_cast<void *>(0x4C95E8), selectCall, sizeof selectCall) ||
        std::memcmp(reinterpret_cast<void *>(0x712A69), shiftCall, sizeof shiftCall) ||
        std::memcmp(reinterpret_cast<void *>(0x731856), deleteCall, sizeof deleteCall))
    {
        Log("Object replacement", "Unknown native call contract; replacement API disabled.");
        return;
    }
    auto select = patcher->WriteHiHook(0x506570, THISCALL_, SelectTemplate);
    auto shift = patcher->WriteHiHook(0x712CEB, CDECL_, FindEntranceShift);
    Patch *hooks[] = {select, shift};
    if (!CheckHooks(hooks, "Object replacement")) return;
    ready = true;
}
}
}

extern "C" int __stdcall Erm_GetObjectReplacementApiVersion() noexcept { return ready ? 1 : 0; }

// Fixed-location flat terrain for town/hero rules. x/y is the DEF bottom-right.
// Only completely passable, entrance-free underlays are accepted.
extern "C" int __stdcall Erm_PlaceSpecialTerrain(int x, int y, int z, int type)
{
    try
    {
        auto map = runtime::GetMap();
        if (!ready || active || !map || H3Network::Multiplayer() || !Coordinates(*map, x, y, z) ||
            x > 255 || y > 255 || (type != eObject::CURSED_GROUND && type != eObject::MAGIC_PLAINS1 &&
            (type < eObject::CLOVER_FIELD || type > eObject::ROCKLANDS)) || type == eObject::FAVORABLE_WINDS ||
            map->objectAttributes.Count() >= 32767 || map->objectDetails.Count() >= 65535) return 0;
        ArgumentsScope args;
        const auto generation = mapGeneration;
        H3ObjectDetails prepared{};
        prepared.x = static_cast<UINT8>(x);
        prepared.y = static_cast<UINT8>(y);
        prepared.z = static_cast<UINT8>(z);
        THISCALL_5(void, 0x506570, map, &prepared, type, 0, -1);
        if (runtime::GetMap() != map || generation != mapGeneration) return -1;
        if (prepared.num >= map->objectAttributes.Count()) return 0;
        const auto &a = map->objectAttributes[prepared.num];
        const int width = a.width, height = a.height;
        if (a.type != type || a.subtype || !a.flat || width < 1 || width > 8 || height < 1 || height > 6 ||
            x + 1 < width || y + 1 < height || (a.passability.m_bitsPacked & 0xFFFFFFFFFFFFull) != 0xFFFFFFFFFFFFull ||
            (a.entrances.m_bitsPacked & 0xFFFFFFFFFFFFull)) return 0;
        for (int dy = 0; dy < height; ++dy) for (int dx = 0; dx < width; ++dx)
        {
            const auto tile = map->GetMapItem(x - dx, y - dy, z);
            if (!tile || tile->land >= 8 || !a.maskTerrain.bitfield.GetState(tile->land)) return 0;
            for (const auto &draw : tile->objectDrawing)
            {
                if (draw.sprite >= map->objectDetails.Count()) return 0;
                const auto num = map->objectDetails[draw.sprite].num;
                if (num >= map->objectAttributes.Count()) return 0;
                const int t = map->objectAttributes[num].type;
                if (t == eObject::CURSED_GROUND || t == eObject::MAGIC_PLAINS1 ||
                    (t >= eObject::CLOVER_FIELD && t <= eObject::ROCKLANDS)) return 0;
            }
        }
        const auto before = map->objectDetails.Count();
        Selection selection{map, type, 0, prepared.num, x, y, z, 0, 0};
        SelectionScope scope(selection);
        CDECL_8(int, 0x71299E, x, y, z, type, 0, type, 0, -1);
        if (runtime::GetMap() != map || generation != mapGeneration) return -1;
        if (map->objectDetails.Count() == before) return 0;
        if (map->objectDetails.Count() != before + 1) return -1;
        const auto &added = map->objectDetails[before];
        if (added.num != prepared.num || added.x != x || added.y != y || added.z != z) return -1;
        for (int dy = 0; dy < height; ++dy) for (int dx = 0; dx < width; ++dx)
        {
            const auto &draws = map->GetMapItem(x - dx, y - dy, z)->objectDrawing;
            if (std::count_if(draws.begin(), draws.end(),
                [before](const H3ObjectDraw &draw) { return draw.sprite == before; }) != 1) return -1;
        }
        for (int dy = 0; dy < height; ++dy) for (int dx = 0; dx < width; ++dx)
        {
            auto &draws = map->GetMapItem(x - dx, y - dy, z)->objectDrawing;
            const auto it = std::find_if(draws.begin(), draws.end(),
                [before](const H3ObjectDraw &draw) { return draw.sprite == before; });
            std::rotate(draws.begin(), it, it + 1);
        }
        return 1;
    }
    catch (const std::bad_alloc &) { return -1; }
}

// Local operation, distributed by the ERM caller with P/D. Deterministic ordering
// selects the same minimum compatible DEF on every peer; native auto-send is muted.
extern "C" int __stdcall Erm_PlaceSmallestSpecialTerrain(int x, int y, int z, int type)
{
    try
    {
        auto map = runtime::GetMap();
        if (!ready || active || !map || !Coordinates(*map, x, y, z) || !LandSpecialTerrain(type) ||
            map->objectAttributes.Count() >= 32767 || map->objectDetails.Count() >= 65535) return 0;
        const auto center = map->GetMapItem(x, y, z);
        if (!center || center->land >= 8) return 0;
        const int ground = center->land;
        BYTE *sendFlag = nullptr;
        if (!LocalTerrainSendFlag(sendFlag))
        {
            if (!terrainNetworkWarning)
            {
                terrainNetworkWarning = true;
                Log("Centered terrain", "Unknown ERA network placement wrapper; centered terrain skipped before placement.");
            }
            return 0;
        }
        ArgumentsScope arguments;
        const auto generation = mapGeneration;
        int types[2] = {type, type};
        if (type == eObject::MAGIC_PLAINS1) types[1] = eObject::MAGIC_PLAINS2;
        else if (type == eObject::MAGIC_PLAINS2) types[1] = eObject::MAGIC_PLAINS1;
        else if (type == eObject::CURSED_GROUND) types[1] = eObject::CURSED_GROUND2;
        else if (type == eObject::CURSED_GROUND2) types[1] = eObject::CURSED_GROUND;
        struct Terrain { int type, attribute, width, height, catalogIndex; std::string name; };
        std::vector<Terrain> candidates;
        for (int group = 0; group < (types[0] == types[1] ? 1 : 2); ++group)
        {
            const int actualType = types[group];
            for (unsigned i = 0; i < map->objectLists[actualType].Count(); ++i)
            {
                auto &a = map->objectLists[actualType][i];
                if (a.type != actualType || a.subtype || !a.flat ||
                    !a.maskTerrain.bitfield.GetState(ground)) continue;
                const std::string name(a.defName.String());
                const int cached = static_cast<INT16>(a.defIndex);
                if (cached >= 0)
                {
                    bool valid = static_cast<unsigned>(cached) < map->objectAttributes.Count();
                    if (valid)
                    {
                        const auto &old = map->objectAttributes[cached];
                        valid = old.type == actualType && old.subtype == 0 && old.flat == a.flat &&
                            libc::strcmpi(old.defName.String(), name.c_str()) == 0 &&
                            old.passability.m_bitsPacked == a.passability.m_bitsPacked &&
                            old.entrances.m_bitsPacked == a.entrances.m_bitsPacked;
                    }
                    if (!valid) a.defIndex = static_cast<UINT16>(-1);
                }
                H3ObjectDetails prepared{};
                if (map->objectAttributes.Count() >= 32767) return 0;
                prepared.x = static_cast<UINT8>(std::min(x, 255));
                prepared.y = static_cast<UINT8>(std::min(y, 255));
                prepared.z = static_cast<UINT8>(z);
                {
                    PrototypeScope scope(map->objectLists[actualType], static_cast<int>(i));
                    THISCALL_5(void, 0x506570, map, &prepared, actualType, 0, -1);
                }
                if (runtime::GetMap() != map || generation != mapGeneration) return -1;
                if (prepared.num >= map->objectAttributes.Count()) continue;
                const auto &loaded = map->objectAttributes[prepared.num];
                if (loaded.type != actualType || loaded.subtype ||
                    libc::strcmpi(loaded.defName.String(), name.c_str()) ||
                    !PassableUnderlay(loaded.width, loaded.height, loaded.flat != 0,
                        loaded.passability.m_bitsPacked, loaded.entrances.m_bitsPacked)) continue;
                candidates.push_back({actualType, prepared.num, loaded.width, loaded.height, static_cast<int>(i), name});
            }
        }
        std::sort(candidates.begin(), candidates.end(), [](const Terrain &a, const Terrain &b)
        {
            if (a.width != b.width || a.height != b.height)
                return SmallerTerrain(a.width, a.height, b.width, b.height);
            if (a.type != b.type) return a.type < b.type;
            const int name = libc::strcmpi(a.name.c_str(), b.name.c_str());
            return name ? name < 0 : a.catalogIndex < b.catalogIndex;
        });
        for (const auto &candidate : candidates)
        {
            if (static_cast<unsigned>(candidate.attribute) >= map->objectAttributes.Count()) return -1;
            const auto &loaded = map->objectAttributes[candidate.attribute];
            if (loaded.type != candidate.type || loaded.subtype || loaded.width != candidate.width ||
                loaded.height != candidate.height || libc::strcmpi(loaded.defName.String(), candidate.name.c_str())) continue;
            std::array<Cell, 4> origins;
            const int count = CenteredTerrainOrigins(x, y, candidate.width, candidate.height, map->mapSize, origins);
            for (int i = 0; i < count; ++i)
            {
                H3ObjectDetails prepared{};
                prepared.x = static_cast<UINT8>(origins[i].x);
                prepared.y = static_cast<UINT8>(origins[i].y);
                prepared.z = static_cast<UINT8>(z);
                prepared.num = static_cast<UINT16>(candidate.attribute);
                if (!UnderlayFits(*map, map->objectAttributes[candidate.attribute], prepared.x, prepared.y, z)) continue;
                const int result = CommitUnderlay(*map, prepared, generation, sendFlag);
                if (result) return result;
            }
        }
        return 0;
    }
    catch (const std::bad_alloc &) { return -1; }
}

// Place one native underlay prototype per attempt. The selected DEF and its actual
// type are kept together; in particular 223 and 230 are not rewritten to 21/46.
extern "C" int __stdcall Erm_AddSpecialTerrainsEx(int attempts, int z)
{
    SoilTrace trace(attempts, z);
    try
    {
        auto map = runtime::GetMap();
        if (map) { trace.size = map->mapSize; trace.underground = map->hasUnderground; }
        if (!ready || active || !map || H3Network::Multiplayer() || map->mapSize < 8 || map->mapSize > 256 ||
            attempts < 0 || attempts > 100000 || z < 0 || z > (map->hasUnderground ? 1 : 0))
        {
            const char *why = !ready ? "hooks_not_ready" : active ? "busy" : !map ? "map_unavailable" :
                H3Network::Multiplayer() ? "multiplayer" : "invalid_arguments_or_map";
            return trace.Finish(-1, why);
        }
        const int types[] = {eObject::CURSED_GROUND, eObject::MAGIC_PLAINS1,
            eObject::CLOVER_FIELD, eObject::CURSED_GROUND2, eObject::EVIL_FOG,
            eObject::FIERY_FIELDS, eObject::HOLY_GROUNDS, eObject::LUCID_POOLS,
            eObject::MAGIC_CLOUDS, eObject::MAGIC_PLAINS2, eObject::ROCKLANDS};
        struct CandidateGroup { int type; std::vector<int> indices; };
        std::vector<CandidateGroup> groups;
        for (int type : types)
        {
            CandidateGroup group{type, {}};
            for (unsigned i = 0; i < map->objectLists[type].Count(); ++i)
            {
                const auto &a = map->objectLists[type][i];
                if (a.type == type && a.subtype == 0 && a.flat)
                    group.indices.push_back(static_cast<int>(i));
            }
            if (!group.indices.empty()) groups.push_back(std::move(group));
        }
        if (groups.empty() || attempts == 0) return trace.Finish(0, groups.empty() ? "no_templates" : "zero_attempts");
        ArgumentsScope arguments;
        int placed = 0;
        const auto generation = mapGeneration;
        // Underground is often mostly rock. Sample walkable land instead of spending
        // most attempts on cells where placement is impossible. The full footprint
        // and overlays are still rechecked immediately before placement.
        std::vector<Cell> undergroundAnchors;
        std::vector<int> impassablePrefix;
        std::array<std::vector<Cell>, 63> fittingAnchors;
        std::array<bool, 63> fittingBuilt{};
        const int side = map->mapSize + 1;
        if (z == 1)
        {
            impassablePrefix.resize(side * side);
            for (int y = 0; y < map->mapSize; ++y)
                for (int x = 0; x < map->mapSize; ++x)
                {
                    const auto cell = map->GetMapItem(x, y, z);
                    const int i = (y + 1) * side + x + 1;
                    impassablePrefix[i] = (!cell || cell->land >= 8 ? 1 : 0) +
                        impassablePrefix[i - 1] + impassablePrefix[i - side] - impassablePrefix[i - side - 1];
                    if (cell && cell->land < 8 && !cell->IsBlocked() && !cell->IsEntrance())
                        undergroundAnchors.push_back({x, y});
                }
            trace.anchors = static_cast<int>(undergroundAnchors.size());
            if (undergroundAnchors.empty()) return trace.Finish(0, "no_walkable_underground");
        }
        const auto randomIndex = [](int count) {
            if (count <= 32768) return H3Random::Rand(0, count - 1);
            const int range = 32768 * 32768;
            const int limit = range - range % count;
            int value;
            do { value = H3Random::Rand(0, 32767) * 32768 + H3Random::Rand(0, 32767); }
            while (value >= limit);
            return value % count;
        };
        for (int attempt = 0; attempt < attempts; ++attempt)
        {
            ++trace.processed;
            int x, y;
            if (z == 1)
            {
                const auto &anchor = undergroundAnchors[randomIndex(static_cast<int>(undergroundAnchors.size()))];
                x = anchor.x; y = anchor.y;
            }
            else
            {
                x = H3Random::Rand(0, map->mapSize - 1);
                y = H3Random::Rand(0, map->mapSize - 1);
            }
            trace.x = x; trace.y = y; trace.type = -1;
            const auto tile = map->GetMapItem(x, y, z);
            if (!tile || tile->land >= 8) { ++trace.terrain; continue; }
            bool occupied = false;
            for (const auto &draw : tile->objectDrawing)
            {
                if (draw.sprite >= map->objectDetails.Count()) continue;
                const auto &detail = map->objectDetails[draw.sprite];
                if (detail.num >= map->objectAttributes.Count()) continue;
                const int existingType = map->objectAttributes[detail.num].type;
                if (existingType == eObject::FAVORABLE_WINDS ||
                    std::find(std::begin(types), std::end(types), existingType) != std::end(types))
                { occupied = true; break; }
            }
            if (occupied) { ++trace.occupied; continue; }
            // Pick a type uniformly among those with at least one compatible DEF,
            // then pick a compatible variant. No allocations inside the attempt loop.
            int eligibleGroups[11], eligibleCount = 0;
            for (unsigned g = 0; g < groups.size(); ++g)
                for (int index : groups[g].indices)
                    if (map->objectLists[groups[g].type][index].maskTerrain.bitfield.GetState(tile->land))
                    { eligibleGroups[eligibleCount++] = static_cast<int>(g); break; }
            if (!eligibleCount) { ++trace.noType; continue; }
            const auto &group = groups[eligibleGroups[H3Random::Rand(0, eligibleCount - 1)]];
            int compatibleCount = 0;
            for (int index : group.indices)
                if (map->objectLists[group.type][index].maskTerrain.bitfield.GetState(tile->land)) ++compatibleCount;
            int choice = H3Random::Rand(0, compatibleCount - 1), candidateIndex = -1;
            for (int index : group.indices)
                if (map->objectLists[group.type][index].maskTerrain.bitfield.GetState(tile->land) && choice-- == 0)
                { candidateIndex = index; break; }
            if (candidateIndex < 0) continue;
            const int candidateType = group.type;
            trace.type = candidateType;
            auto &a = map->objectLists[candidateType][candidateIndex];
            const std::string defName(a.defName.String());
            // Catalog indices can outlive a map. Discard a stale DEF cache entry.
            const int cached = static_cast<INT16>(a.defIndex);
            if (cached >= 0)
            {
                if (static_cast<unsigned>(cached) >= map->objectAttributes.Count()) a.defIndex = static_cast<UINT16>(-1);
                else
                {
                    const auto &old = map->objectAttributes[cached];
                    if (old.type != candidateType || old.subtype != 0 || old.flat != a.flat ||
                        libc::strcmpi(old.defName.String(), a.defName.String()) != 0 ||
                        old.passability.m_bitsPacked != a.passability.m_bitsPacked ||
                        old.entrances.m_bitsPacked != a.entrances.m_bitsPacked)
                        a.defIndex = static_cast<UINT16>(-1);
                }
            }
            H3ObjectDetails prepared{};
            prepared.x = static_cast<UINT8>(x);
            prepared.y = static_cast<UINT8>(y);
            prepared.z = static_cast<UINT8>(z);
            {
                PrototypeScope scope(map->objectLists[candidateType], candidateIndex);
                THISCALL_5(void, 0x506570, map, &prepared, candidateType, 0, -1);
            }
            if (runtime::GetMap() != map || mapGeneration != generation) return trace.Finish(-1, "map_changed_after_prepare");
            if (prepared.num >= map->objectAttributes.Count()) { ++trace.def; continue; }
            const auto &loaded = map->objectAttributes[prepared.num];
            if (!loaded.flat || loaded.type != candidateType || loaded.subtype != 0 ||
                libc::strcmpi(loaded.defName.String(), defName.c_str()) != 0 ||
                !loaded.width || !loaded.height || loaded.width > 8 || loaded.height > 6) { ++trace.def; continue; }
            if (z == 1)
            {
                const int key = loaded.width * 7 + loaded.height;
                auto &fits = fittingAnchors[key];
                if (!fittingBuilt[key])
                {
                    for (const auto &anchor : undergroundAnchors)
                    {
                        const int left = anchor.x + 1 - loaded.width;
                        const int top = anchor.y + 1 - loaded.height;
                        if (left < 0 || top < 0) continue;
                        const int right = anchor.x + 1, bottom = anchor.y + 1;
                        const int bad = impassablePrefix[bottom * side + right] - impassablePrefix[top * side + right]
                            - impassablePrefix[bottom * side + left] + impassablePrefix[top * side + left];
                        if (!bad) fits.push_back(anchor);
                    }
                    fittingBuilt[key] = true;
                }
                if (fits.empty()) { ++trace.rock; continue; }
                const auto &anchor = fits[randomIndex(static_cast<int>(fits.size()))];
                x = anchor.x; y = anchor.y;
                trace.x = x; trace.y = y;
                prepared.x = static_cast<UINT8>(x); prepared.y = static_cast<UINT8>(y);
            }
            if (x + 1 < loaded.width || y + 1 < loaded.height) { ++trace.bounds; continue; }
            bool valid = true, reachable = false;
            for (int dx = 0; valid && dx < loaded.width; ++dx)
                for (int dy = 0; valid && dy < loaded.height; ++dy)
                {
                    auto cell = map->GetMapItem(x - dx, y - dy, z);
                    if (!cell || cell->land >= 8 || !loaded.maskTerrain.bitfield.GetState(cell->land))
                    {
                        if (cell && cell->land == 9) ++trace.rock;
                        else if (cell && cell->land == 8) ++trace.water;
                        else ++trace.mixed;
                        valid = false; break;
                    }
                    if (!NativeMaskCell(loaded.passability.m_bitsPacked, dx, dy) ||
                        NativeMaskCell(loaded.entrances.m_bitsPacked, dx, dy))
                    { ++trace.mask; valid = false; break; }
                    if (!cell->IsBlocked() && !cell->IsEntrance()) reachable = true;
                    for (const auto &draw : cell->objectDrawing)
                    {
                        if (draw.sprite >= map->objectDetails.Count()) continue;
                        const auto &details = map->objectDetails[draw.sprite];
                        if (details.num >= map->objectAttributes.Count()) continue;
                        const auto &existing = map->objectAttributes[details.num];
                        if (existing.type == eObject::FAVORABLE_WINDS) valid = false;
                        for (int t : types) if (existing.type == t) { valid = false; break; }
                        if (!valid) { ++trace.overlap; break; }
                    }
                }
            if (!valid) continue;
            if (!reachable) { ++trace.unreachable; continue; }
            const int width = loaded.width, height = loaded.height;
            Selection selection{map, candidateType, 0, prepared.num, x, y, z, 0, 0};
            SelectionScope scope(selection);
            const auto before = map->objectDetails.Count();
            if (before >= 65535) return trace.Finish(-1, "object_limit");
            CDECL_8(int, 0x71299E, x, y, z, candidateType, 0, candidateType, 0, -1);
            if (runtime::GetMap() != map || mapGeneration != generation) return trace.Finish(-1, "map_changed_after_place");
            if (map->objectDetails.Count() == before) { ++trace.native; continue; }
            if (map->objectDetails.Count() != before + 1) return trace.Finish(-1, "unexpected_object_count");
            const auto &added = map->objectDetails[before];
            if (added.x != x || added.y != y || added.z != z || added.num != prepared.num) return trace.Finish(-1, "unexpected_placed_object");
            // Validate every cell before changing any list. Native underlays all have
            // layer zero and are drawn in objectDrawing order, including other flats.
            for (int dx = 0; dx < width; ++dx)
                for (int dy = 0; dy < height; ++dy)
                {
                    const auto cell = map->GetMapItem(x - dx, y - dy, z);
                    if (!cell || std::count_if(cell->objectDrawing.begin(), cell->objectDrawing.end(),
                        [before](const H3ObjectDraw &draw) { return draw.sprite == before; }) != 1) return trace.Finish(-1, "missing_or_duplicate_draw");
                }
            for (int dx = 0; dx < width; ++dx)
                for (int dy = 0; dy < height; ++dy)
                {
                    auto &draws = map->GetMapItem(x - dx, y - dy, z)->objectDrawing;
                    const auto it = std::find_if(draws.begin(), draws.end(),
                        [before](const H3ObjectDraw &draw) { return draw.sprite == before; });
                    std::rotate(draws.begin(), it, it + 1);
                }
            ++placed;
            trace.placed = placed;
        }
        return trace.Finish(placed, "completed");
    }
    catch (const std::bad_alloc &) { return trace.Finish(-1, "allocation_failure"); }
}

extern "C" int __stdcall Erm_GetObjectGeometry(int x, int y, int z, int *output, int capacity)
{
    try
    {
        auto map = runtime::GetMap();
        Source source;
        if (!ready || !map || !output || capacity < 15 || !ResolveSource(*map, x, y, z, source)) return 0;
        const auto &attributes = map->objectAttributes[source.attributesIndex];
        const auto &details = map->objectDetails[source.detailsIndex];
        const std::uint64_t blocked = ~attributes.passability.m_bitsPacked & 0xFFFFFFFFFFFFull;
        const std::uint64_t entrance = attributes.entrances.m_bitsPacked & 0xFFFFFFFFFFFFull;
        const int data[] = {attributes.type, attributes.subtype, details.x, details.y, z,
                            source.shape->width, source.shape->height, source.shape->entranceColumn,
                            source.shape->entranceRow, static_cast<int>(blocked), static_cast<int>(blocked >> 32),
                            static_cast<int>(entrance), static_cast<int>(entrance >> 32),
                            source.attributesIndex, source.detailsIndex};
        std::copy(data, data + 15, output);
        return 1;
    }
    catch (const std::bad_alloc &) { return 0; }
}

extern "C" int __stdcall Erm_FindObjectReplacement(int x, int y, int z, int type, int subtype,
                                                   int terrain, int approachMask)
{
    try
    {
        auto map = runtime::GetMap();
        Source source;
        // ERA's standard placement packet does not carry the selected DEF variant. Until that
        // protocol is extended, never substitute a different variant on only one network peer.
        if (!ready || active || !map || H3Network::Multiplayer() ||
            !ValidTarget(type, subtype, terrain, approachMask) ||
            !ResolveSource(*map, x, y, z, source) || !GroundAllowed(source, terrain)) return 0;
        auto &list = map->objectLists[type];
        // Same preference as the native selector, but every variant is checked independently.
        for (int index = list.Count() - 1; index >= 0; --index)
        {
            const Shape *shape = nullptr;
            Placement placement;
            if (Candidate(*map, source, x, y, z, type, subtype, approachMask, index, shape, placement))
                return index + 1;
        }
        return 0;
    }
    catch (const std::bad_alloc &) { return 0; }
}

// Records have six integers: type, subtype, terrain, approach mask, weight, ERM offset handle.
// The last two fields are owned and interpreted by the caller.
extern "C" int __stdcall Erm_FindObjectReplacements(int x, int y, int z, const int *records,
                                                     int count, int *output)
{
    if (!records || !output || count < 0 || count > 100000) return 0;
    try
    {
        auto map = runtime::GetMap();
        Source source;
        if (!ready || active || !map || H3Network::Multiplayer() ||
            !ResolveSource(*map, x, y, z, source)) return 0;
        for (int i = 0; i < count; ++i)
        {
            const int *entry = records + i * 6;
            const int type = entry[0], subtype = entry[1], terrain = entry[2], approach = entry[3];
            output[i] = 0;
            if (!ValidTarget(type, subtype, terrain, approach) || !GroundAllowed(source, terrain)) continue;
            auto &list = map->objectLists[type];
            for (int index = static_cast<int>(list.Count()) - 1; index >= 0; --index)
            {
                const Shape *shape = nullptr;
                Placement placement;
                if (Candidate(*map, source, x, y, z, type, subtype, approach, index, shape, placement))
                {
                    output[i] = index + 1;
                    break;
                }
            }
        }
        return 1;
    }
    catch (const std::bad_alloc &) { return 0; }
}

extern "C" int __stdcall Erm_ReplaceObject(int x, int y, int z, int type, int subtype,
                                           int terrain, int approachMask, int prototype)
{
    bool deletionStarted = false;
    try
    {
        if (H3Network::Multiplayer()) return ReplacementNetworkUnsupported;
        auto map = runtime::GetMap();
        Source source;
        const Shape *shape = nullptr;
        Placement placement;
        if (!ready || active || !map ||
            prototype <= 0 || !ValidTarget(type, subtype, terrain, approachMask) || !ResolveSource(*map, x, y, z, source) ||
            !GroundAllowed(source, terrain) ||
            !Candidate(*map, source, x, y, z, type, subtype, approachMask,
                       prototype - 1, shape, placement) || map->objectAttributes.Count() >= 32767 ||
            map->objectDetails.Count() >= 65535) return 0;
        const auto generation = mapGeneration;
        ArgumentsScope arguments;
        H3ObjectDetails prepared{};
        prepared.x = static_cast<UINT8>(placement.originX);
        prepared.y = static_cast<UINT8>(placement.originY);
        prepared.z = static_cast<UINT8>(z);
        auto &attributes = map->objectLists[type][prototype - 1];
        const int cached = static_cast<INT16>(attributes.defIndex);
        if (cached >= 0)
        {
            if (static_cast<unsigned>(cached) >= map->objectAttributes.Count()) return 0;
            const auto &loaded = map->objectAttributes[cached];
            if (loaded.type != type || loaded.subtype != subtype ||
                loaded.width != attributes.width || loaded.height != attributes.height ||
                loaded.passability.m_bitsPacked != attributes.passability.m_bitsPacked ||
                loaded.entrances.m_bitsPacked != attributes.entrances.m_bitsPacked ||
                loaded.colors.m_bitsPacked != attributes.colors.m_bitsPacked ||
                libc::strcmpi(loaded.defName.String(), attributes.defName.String()) != 0)
                attributes.defIndex = static_cast<UINT16>(-1);
        }
        {
            PrototypeScope scope(map->objectLists[type], prototype - 1);
            THISCALL_5(void, 0x506570, map, &prepared, type, subtype, -1);
        }
        if (runtime::GetMap() != map || mapGeneration != generation) return ReplacementMapChanged;
        Source current;
        if (!ResolveSource(*map, x, y, z, current) || current.detailsIndex != source.detailsIndex ||
            current.attributesIndex != source.attributesIndex || !SameTopology(*current.shape, *source.shape))
        {
            Log("Object replacement", "Source changed during native preparation; stop the replacement pass.");
            return ReplacementPartialFailure;
        }
        if (prepared.num >= map->objectAttributes.Count()) return 0;
        const auto &preparedAttributes = map->objectAttributes[prepared.num];
        if (preparedAttributes.type != type || preparedAttributes.subtype != subtype) return ReplacementRejected;
        const auto loadedShape = ShapeOf(preparedAttributes);
        if (!loadedShape || !SameTopology(*shape, *loadedShape)) return 0;
        // Preparation invokes third-party hooks: recheck the source and placement
        // before committing the destructive part, not just the loaded DEF.
        const Shape *checkedShape = nullptr;
        Placement checkedPlacement;
        if (!Candidate(*map, current, x, y, z, type, subtype, approachMask,
                       prototype - 1, checkedShape, checkedPlacement) ||
            checkedPlacement.originX != placement.originX || checkedPlacement.originY != placement.originY)
            return ReplacementRejected;
        Selection selection{map, type, subtype, prepared.num, placement.originX, placement.originY, z,
                            shape->entranceColumn, shape->entranceRow};
        SelectionScope scope(selection);
        deletionStarted = true;
        DeleteMapObject(x, y, z);
        if (runtime::GetMap() != map || mapGeneration != generation) return ReplacementMapChanged;
        Source remaining;
        if (ResolveSource(*map, x, y, z, remaining) && remaining.detailsIndex == source.detailsIndex)
        {
            Log("Object replacement", "Native deletion left the source in place; placement cancelled.");
            return ReplacementPartialFailure;
        }
        // Call WoG's public placement entry so dwelling/bank initialization and ERA/IP hooks run.
        CDECL_8(int, 0x71299E, placement.originX, placement.originY, z, type, subtype, type, subtype, -1);
        if (runtime::GetMap() != map || mapGeneration != generation) return ReplacementMapChanged;
        Source result;
        if (ResolveSource(*map, x, y, z, result))
        {
            const auto &placed = map->objectAttributes[result.attributesIndex];
            if (placed.type == type && placed.subtype == subtype && SameTopology(*source.shape, *result.shape))
                return ReplacementSucceeded;
        }
        Log("Object replacement", "Placement verification failed AFTER deletion; stop the replacement pass.");
        return ReplacementPartialFailure;
    }
    catch (const std::bad_alloc &)
    {
        if (deletionStarted)
        {
            Log("Object replacement", "Allocation failed AFTER deletion; stop the replacement pass.");
            return ReplacementPartialFailure;
        }
        return ReplacementRejected;
    }
}
