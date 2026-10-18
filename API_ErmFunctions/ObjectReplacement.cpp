#include "framework.h"
#include "ObjectRendering.h"
#include "ObjectReplacement.h"
#include "ObjectGeometry.h"
#include "ObjectShapeCache.h"
#include <cstring>

namespace
{
using namespace h3;
using namespace erm_functions;
using namespace geometry;
bool ready = false;
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
}

namespace erm_functions
{
namespace replacement
{
void ResetMap() { active = nullptr; ++mapGeneration; shapeCache.Clear(); }

void InstallHooks(PatcherInstance *patcher)
{
    if (ready || !patcher) return;
    // Stable call and argument layout, verified against local h3era.exe. Do not inspect a
    // shared HiHook's prologue, which other compatible plugins may already have patched.
    static const unsigned char selectCall[] = {0xE8,0x83,0xCF,0x03,0x00};
    static const unsigned char shiftCall[] = {0xE8,0x7D,0x02,0x00,0x00};
    static const unsigned char deleteCall[] = {0xE8,0x2F,0x13,0xFE,0xFF,0x83,0xC4,0x0C};
    if (std::memcmp(reinterpret_cast<void *>(0x4C95E8), selectCall, sizeof selectCall) ||
        std::memcmp(reinterpret_cast<void *>(0x712A69), shiftCall, sizeof shiftCall) ||
        std::memcmp(reinterpret_cast<void *>(0x731856), deleteCall, sizeof deleteCall)) return;
    auto select = patcher->WriteHiHook(0x506570, THISCALL_, SelectTemplate);
    auto shift = patcher->WriteHiHook(0x712CEB, CDECL_, FindEntranceShift);
    if (!select || !shift)
    {
        if (select) select->Destroy();
        if (shift) shift->Destroy();
        return;
    }
    ready = true;
}
}
}

extern "C" int __stdcall Erm_GetObjectReplacementApiVersion() { return ready ? 1 : 0; }

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

extern "C" int __stdcall Erm_ReplaceObject(int x, int y, int z, int type, int subtype,
                                           int terrain, int approachMask, int prototype)
{
    try
    {
        auto map = runtime::GetMap();
        Source source;
        const Shape *shape = nullptr;
        Placement placement;
        if (!ready || active || !map || H3Network::Multiplayer() ||
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
            if (loaded.width != attributes.width || loaded.height != attributes.height ||
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
        if (runtime::GetMap() != map || mapGeneration != generation) return 0;
        if (prepared.num >= map->objectAttributes.Count()) return 0;
        const auto loadedShape = ShapeOf(map->objectAttributes[prepared.num]);
        if (!loadedShape || !SameTopology(*shape, *loadedShape)) return 0;
        Selection selection{map, type, subtype, prepared.num, placement.originX, placement.originY, z,
                            shape->entranceColumn, shape->entranceRow};
        SelectionScope scope(selection);
        DeleteMapObject(x, y, z);
        if (runtime::GetMap() != map || mapGeneration != generation) return 0;
        // Call WoG's public placement entry so dwelling/bank initialization and ERA/IP hooks run.
        CDECL_8(int, 0x71299E, placement.originX, placement.originY, z, type, subtype, type, subtype, -1);
        if (runtime::GetMap() != map || mapGeneration != generation) return 0;
        Source result;
        if (!ResolveSource(*map, x, y, z, result)) return 0;
        const auto &placed = map->objectAttributes[result.attributesIndex];
        return placed.type == type && placed.subtype == subtype && SameTopology(*source.shape, *result.shape);
    }
    catch (const std::bad_alloc &) { return 0; }
}
