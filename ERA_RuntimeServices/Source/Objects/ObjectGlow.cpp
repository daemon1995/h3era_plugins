#include "../framework.h"
#include "ObjectGlow.h"
#include "ObjectRendering.h"
#include "ObjectGeometry.h"
#include "ObjectGlowParticles.h"
#include "../Core/HookGroup.h"
#include <intrin.h>
#include <cstring>
#include <cstdlib>
#include <new>

namespace erm_functions
{
namespace glow
{
namespace
{
ObjectTypes overrides;
bool enabled = true;
bool hooksReady = false;
constexpr std::size_t MASK_CACHE_LIMIT = 16 * 1024 * 1024;
struct Profile
{
    bool enabled = false;
    std::uint64_t calls = 0, tiles = 0, hits = 0, misses = 0, flushes = 0;
    std::uint64_t glowTicks = 0, fogTicks = 0, decodeTicks = 0, peakBytes = 0, peakScratchBytes = 0;
};
Profile profile;
std::size_t maskBytes = 0;

std::uint64_t Clock() noexcept
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}
// ERA Scripts uses type 142 for warehouses; its setup differs from SoD's lean-to (39).
constexpr int WAREHOUSE = 142;

bool DefaultType(int type)
{
    switch (type)
    {
    case eObject::CREATURE_BANK:
    case eObject::CRYPT:
    case eObject::DERELICT_SHIP:
    case eObject::DRAGON_UTOPIA:
    case eObject::SHIPWRECK:
    case eObject::FOUNTAIN_OF_FORTUNE:
    case eObject::LEAN_TO:
    case eObject::KEYMASTER:
    case eObject::MYSTICAL_GARDEN:
    case eObject::OBELISK:
    case eObject::PYRAMID:
    case eObject::SHRINE_OF_MAGIC_INCANTATION:
    case eObject::SHRINE_OF_MAGIC_GESTURE:
    case eObject::SHRINE_OF_MAGIC_THOUGHT:
    case eObject::TREE_OF_KNOWLEDGE:
    case eObject::UNIVERSITY:
    case eObject::WAGON:
    case eObject::WARRIORS_TOMB:
    case eObject::WATER_WHEEL:
    case eObject::WINDMILL:
    case eObject::WITCH_HUT:
        return true;
    default:
        return false;
    }
}

// -1: no glow, 0..7: resource colour. Read a setup snapshot so a hero on the
// entrance is handled through H3MapItem::GetRealSetup, without changing game data.
bool TypeEnabled(int type, int subtype)
{
    const bool *setting = overrides.Find(type, subtype);
    // SoD type 142 is decorative. Recognize the ERA warehouse automatically only
    // when its script has registered that type for ownership, or explicitly for glow.
    const bool warehouse = type == WAREHOUSE && runtime::FindObjectType(type, subtype);
    return setting ? *setting : DefaultType(type) || warehouse;
}

int ResourceForObject(const H3MapItem &tile, int type, int subtype, int player, const H3Main *game)
{
    H3MapItem object{};
    object.setup = tile.GetRealSetup();
    object.objectType = static_cast<INT16>(type);
    switch (type)
    {
    case eObject::KEYMASTER:
        return subtype >= 0 && subtype < 8 && !game->keymasterVisited[subtype].Status(player)
                   ? eResource::GOLD : -1;
    case eObject::OBELISK:
        return object.obelisk.id < 48 && !(game->obeliskVisited[object.obelisk.id] & (1 << player))
                   ? eResource::GOLD : -1;
    case eObject::WINDMILL:
        return !(object.setup & (32u << player)) && object.windmill.GetAmount() > 0
                   ? object.windmill.GetType() : -1;
    case eObject::WATER_WHEEL:
        return !object.watermill.GetVisiters().Status(player) && object.watermill.GetGold() > 0
                   ? eResource::GOLD : -1;
    case eObject::MYSTICAL_GARDEN:
        return object.mysticGarden.hasRes ? object.mysticGarden.GetType() : -1;
    case eObject::LEAN_TO:
        return object.leanTo.GetAmount() > 0 ? object.leanTo.GetType() : -1;
    case WAREHOUSE:
        // Same reward layout as ES_879_AdjustStoredRes; bits 5..12 are visitors.
        return !(object.setup & (32u << player)) && ((object.setup >> 13) & 1023u)
                   ? int(object.setup & 31u) : -1;
    case eObject::CREATURE_BANK:
    case eObject::CRYPT:
    case eObject::DERELICT_SHIP:
    case eObject::DRAGON_UTOPIA:
    case eObject::SHIPWRECK:
        return !object.creatureBank.taken && !object.creatureBank.GetVisiters().Status(player)
                   ? eResource::GOLD : -1;
    case eObject::FOUNTAIN_OF_FORTUNE:
        return !object.fountainFortune.GetVisiters().Status(player) ? eResource::GOLD : -1;
    case eObject::PYRAMID:
        return object.pyramid.available && !object.pyramid.GetVisiters().Status(player) ? eResource::GOLD : -1;
    case eObject::TREE_OF_KNOWLEDGE:
        return !object.treeKnowledge.GetVisiters().Status(player) ? eResource::GOLD : -1;
    case eObject::UNIVERSITY:
        return !object.university.GetVisiters().Status(player) ? eResource::GOLD : -1;
    case eObject::WAGON:
        if (!object.wagon.hasBonus || object.wagon.GetVisiters().Status(player))
            return -1;
        return object.wagon.hasArtifact ? eResource::GOLD : object.wagon.GetResourceType();
    case eObject::WARRIORS_TOMB:
        return object.warriorsTomb.hasArt && !object.warriorsTomb.GetVisiters().Status(player)
                   ? eResource::GOLD : -1;
    case eObject::WITCH_HUT:
        return !object.witchHut.GetVisiters().Status(player) ? eResource::GOLD : -1;
    default:
        // Shrines and explicitly enabled custom types use the native player mask.
        return !(object.setup & (32u << player)) ? eResource::GOLD : -1;
    }
}

int ViewingPlayer(unsigned mask)
{
    if (!mask || mask > 128 || (mask & (mask - 1)))
        return -1;
    int player = 0;
    while ((mask >>= 1) != 0)
        ++player;
    return player;
}

void DrawPixel(H3LoadedPcx16 &buffer, const RECT &clip, int x, int y, unsigned colour, bool is32bit)
{
    if (x < clip.left || x >= clip.right || y < clip.top || y >= clip.bottom)
        return;
    const unsigned r = (colour >> 16) & 255, g = (colour >> 8) & 255, b = colour & 255;
    if (is32bit)
    {
        auto &pixel = *buffer.GetPixel888(x, y);
        pixel.r = static_cast<UINT8>(r);
        pixel.g = static_cast<UINT8>(g);
        pixel.b = static_cast<UINT8>(b);
    }
    else
    {
        auto &pixel = *buffer.GetPixel565(x, y);
        pixel = H3RGB888(static_cast<UINT8>(r), static_cast<UINT8>(g), static_cast<UINT8>(b));
    }
}

struct FrameMask
{
    const BYTE *source = nullptr;
    const H3Palette565 *palette = nullptr;
    int sourceSize = 0;
    int width = 0, height = 0, bitMode = 0, rowWords = 0;
    std::vector<std::uint32_t> opaque;
    std::array<unsigned, 4> colours{};
    int left = 0, bottom = 0, span = 0;
};
std::unordered_map<const H3DefFrame *, FrameMask> frameMasks;

// Scratch surfaces grow only when necessary; frames of different sizes reuse them.
// Process-owned wrapper avoids invoking engine destructors during DLL shutdown.
struct DecodeSurfaces
{
    H3Pcx16Loader black, white;
    int bitMode = 0;
};
DecodeSurfaces *decodeSurfaces = nullptr;
DecodeSurfaces &Surfaces()
{
    if (!decodeSurfaces) decodeSurfaces = new DecodeSurfaces;
    return *decodeSurfaces;
}

void BuildObjectColours(FrameMask &mask, unsigned red, unsigned green, unsigned blue)
{
    // Preserve the DEF's overall hue, with enough brightness for visible glints.
    const unsigned peak = (std::max)({red, green, blue});
    const unsigned r = peak ? unsigned(std::uint64_t(red) * 192 / peak) : 192;
    const unsigned g = peak ? unsigned(std::uint64_t(green) * 192 / peak) : 192;
    const unsigned b = peak ? unsigned(std::uint64_t(blue) * 192 / peak) : 192;
    const auto shade = [](unsigned colour, unsigned stage) {
        switch (stage)
        {
        case 0: return colour * 3 / 5;
        case 1: return colour;
        case 2: return (colour + 255) / 2;
        default: return (colour + 3 * 255) / 4;
        }
    };
    for (unsigned stage = 0; stage < 4; ++stage)
        mask.colours[stage] = (shade(r, stage) << 16) | (shade(g, stage) << 8) | shade(b, stage);
}

void BuildBounds(FrameMask &mask)
{
    int left = mask.width, right = -1, bottom = -1;
    for (int y = 0; y < mask.height; ++y)
        for (int x = 0; x < mask.width; ++x)
            if (mask.opaque[y * mask.rowWords + x / 32] & (1u << (x & 31)))
            {
                left = (std::min)(left, x);
                right = (std::max)(right, x);
                bottom = (std::max)(bottom, y);
            }
    mask.left = left;
    mask.bottom = bottom;
    mask.span = right < left ? 0 : (std::min)(255, right - left + 1);
}

const FrameMask *GetFrameMask(H3DefFrame &frame, H3Palette565 *palette)
{
    if (!palette || !frame.rawData || frame.width <= 0 || frame.height <= 0 ||
        frame.width > 512 || frame.height > 512)
        return nullptr;
    const int bitMode = H3BitMode::Get();
    const auto found = frameMasks.find(&frame);
    if (found != frameMasks.end() && found->second.source == frame.rawData && found->second.palette == palette &&
        found->second.sourceSize == frame.rawDataSize && found->second.width == frame.width &&
        found->second.height == frame.height && found->second.bitMode == bitMode)
    {
        if (profile.enabled) ++profile.hits;
        return &found->second;
    }
    const auto decodeStart = profile.enabled ? Clock() : 0;
    if (profile.enabled) ++profile.misses;
    auto &surfaces = Surfaces();
    if (!surfaces.black || !surfaces.white || surfaces.bitMode != bitMode ||
        surfaces.black->width < frame.width || surfaces.black->height < frame.height)
    {
        const bool sameMode = surfaces.black && surfaces.bitMode == bitMode;
        const int width = sameMode ? (std::max)(frame.width, surfaces.black->width) : frame.width;
        const int height = sameMode ? (std::max)(frame.height, surfaces.black->height) : frame.height;
        H3Pcx16Loader black(H3LoadedPcx16::Create(width, height)), white(H3LoadedPcx16::Create(width, height));
        if (!black || !white || !black->buffer || !white->buffer) return nullptr;
        H3Pcx16Loader oldBlack(surfaces.black.Release()), oldWhite(surfaces.white.Release());
        surfaces.black.Set(black.Release());
        surfaces.white.Set(white.Release());
        surfaces.bitMode = bitMode;
    }
    if (profile.enabled)
        profile.peakScratchBytes = (std::max)(profile.peakScratchBytes,
            std::uint64_t(surfaces.black->buffSize) + surfaces.white->buffSize);
    auto &black = surfaces.black;
    auto &white = surfaces.white;
    std::memset(black->buffer, 0, black->buffSize);
    std::memset(white->buffer, 255, white->buffSize);
    // Use the game's own DEF decoder for every compression type and HD colour
    // mode. Opaque pixels match on both backgrounds; transparency/shadows do not.
    frame.DrawToPcx16(0, 0, frame.width, frame.height, black.Get(), 0, 0, palette);
    frame.DrawToPcx16(0, 0, frame.width, frame.height, white.Get(), 0, 0, palette);
    FrameMask mask;
    mask.source = frame.rawData;
    mask.palette = palette;
    mask.sourceSize = frame.rawDataSize;
    mask.width = frame.width;
    mask.height = frame.height;
    mask.bitMode = bitMode;
    mask.rowWords = (mask.width + 31) / 32;
    mask.opaque.resize(std::size_t(mask.rowWords) * mask.height, 0);
    unsigned red = 0, green = 0, blue = 0;
    for (int y = 0; y < mask.height; ++y)
        for (int x = 0; x < mask.width; ++x)
        {
            const bool opaque = bitMode == 4
                ? (black->GetPixel888(x, y)->Value() & 0xFFFFFF) ==
                      (white->GetPixel888(x, y)->Value() & 0xFFFFFF)
                : black->GetPixel565(x, y)->GetBits() == white->GetPixel565(x, y)->GetBits();
            if (opaque)
            {
                mask.opaque[y * mask.rowWords + x / 32] |= 1u << (x & 31);
                const auto pixel = bitMode == 4 ? H3RGB888(*black->GetPixel888(x, y))
                                               : H3RGB888(*black->GetPixel565(x, y));
                red += pixel.r;
                green += pixel.g;
                blue += pixel.b;
            }
        }
    BuildObjectColours(mask, red, green, blue);
    BuildBounds(mask);
    const auto bytes = mask.opaque.size() * sizeof(std::uint32_t);
    if (maskBytes + bytes > MASK_CACHE_LIMIT || frameMasks.size() >= 4096)
    {
        frameMasks.clear();
        maskBytes = 0;
        if (profile.enabled) ++profile.flushes;
    }
    auto &cached = frameMasks[&frame];
    maskBytes -= cached.opaque.size() * sizeof(std::uint32_t);
    cached = std::move(mask);
    maskBytes += bytes;
    if (profile.enabled)
    {
        profile.peakBytes = (std::max)(profile.peakBytes, std::uint64_t(maskBytes));
        profile.decodeTicks += Clock() - decodeStart;
    }
    return &cached;
}

struct TileGlow
{
    std::array<std::uint32_t, 32> rows{};
    std::array<unsigned, 32 * 32> colour{};
};

void CompositePiece(const FrameMask &mask, int srcX, int srcY, int resource, unsigned phase, TileGlow &tile)
{
    // Map pieces start on a 32-pixel boundary: one word erases a whole row.
    if (srcX >= 0 && srcX < mask.width)
        for (int y = (std::max)(0, -srcY); y < 32 && srcY + y < mask.height; ++y)
            tile.rows[y] &= ~mask.opaque[(srcY + y) * mask.rowWords + srcX / 32];
    if (resource < 0 || !mask.span)
        return;
    // Two phase-shifted particle streams double density without speeding up motion.
    for (unsigned stream = 0; stream < 2; ++stream)
    {
        const unsigned frame = (phase + stream * 37) & 127;
        for (unsigned i = sparkleFrameOffsets[frame]; i < sparkleFrameOffsets[frame + 1]; ++i)
        {
            const auto &spark = sparklePixels[i];
            const unsigned position = (spark.position + stream * 127) & 255;
            const int x = mask.left + position * mask.span / 256 + spark.offset - srcX;
            const int y = mask.bottom - spark.rise - srcY;
            if (x < 0 || x >= 32 || y < 0 || y >= 32)
                continue;
            const int target = y * 32 + x;
            tile.rows[y] |= 1u << x;
            tile.colour[target] = mask.colours[spark.shade & 3];
        }
    }
}

const H3ObjectAttributes *PieceAttributes(H3MainSetup &map, const H3ObjectDraw &draw)
{
    if (draw.sprite >= map.objectDetails.Count())
        return nullptr;
    const auto index = map.objectDetails[draw.sprite].num;
    if (index >= map.objectAttributes.Count())
        return nullptr;
    auto &attributes = map.objectAttributes[index];
    const int column = draw.tileID & 15, row = draw.tileID >> 4;
    return !attributes.flat && column < attributes.width && row < attributes.height &&
                   geometry::NativeMaskCell(attributes.colors.m_bitsPacked, column, row)
               ? &attributes : nullptr;
}

int PieceResource(H3MainSetup &map, const H3ObjectDraw &draw, const H3ObjectAttributes &attributes, int player, unsigned *phase = nullptr)
{
    if (!TypeEnabled(attributes.type, attributes.subtype))
        return -1;
    int x = -1, y = -1, z = -1;
    const H3ObjectAttributes *resolved = nullptr;
    // Visibility belongs to the drawn tile, not to the entrance whose setup we read.
    if (!runtime::ResolveDrawObject(map, draw, x, y, z, resolved, false))
        return -1;
    const auto entrance = map.GetMapItem(x, y, z);
    if (!entrance || !entrance->IsEntrance() || entrance->GetRealType() != attributes.type)
        return -1;
    if (phase)
        *phase = unsigned(x * 7 + y * 13);
    return ResourceForObject(*entrance, attributes.type, attributes.subtype, player, H3Main::Get());
}

void DrawObjectGlow(H3AdventureManager &adv, int mapX, int mapY, int mapZ, int screenX, int screenY)
{
    if (!enabled) return;
    auto map = runtime::GetMap();
    if (!map)
        return;
    // Same watcher bit tested by native fog at 0x4122B5. The active player can
    // belong to the opponent while their turn is being shown.
    const int player = ViewingPlayer(ByteAt(0x69CD08));
    if (player < 0 || player >= 8 || IntAt(0x698A44) || mapX < 0 || mapY < 0 ||
        mapX >= map->mapSize || mapY >= map->mapSize || mapZ < 0 || mapZ > (map->hasUnderground ? 1 : 0) ||
        !H3TileVision::CanViewTile(mapX, mapY, mapZ, player))
        return;
    const auto tile = map->GetMapItem(mapX, mapY, mapZ);
    if (!tile)
        return;
    struct Piece
    {
        const H3ObjectDraw *draw;
        const H3ObjectAttributes *attributes;
        int resource;
        unsigned phase, order;
    };
    // Resolve visits/rewards once per piece, including the cheap no-glow path.
    // Typical tiles need no heap allocation. Preserve native order within layers.
    std::array<Piece, 32> local;
    std::vector<Piece> overflow;
    Piece *pieces = local.data();
    if (tile->objectDrawing.Count() > local.size())
    {
        overflow.resize(tile->objectDrawing.Count());
        pieces = overflow.data();
    }
    unsigned pieceCount = 0, order = 0;
    bool hasGlow = false;
    for (const auto &draw : tile->objectDrawing)
    {
        const auto attributes = PieceAttributes(*map, draw);
        if (!attributes || draw.layer > 6) continue;
        unsigned phase = 0;
        const int resource = PieceResource(*map, draw, *attributes, player, &phase);
        hasGlow = hasGlow || resource >= 0;
        pieces[pieceCount++] = {&draw, attributes, resource, phase, order++};
    }
    if (!hasGlow)
        return;
    if (profile.enabled) ++profile.tiles;
    std::sort(pieces, pieces + pieceCount, [](const Piece &left, const Piece &right) {
        return left.draw->layer != right.draw->layer ? left.draw->layer < right.draw->layer : left.order < right.order;
    });
    // The native fog renderer owns a live window and draw buffer here.
    auto buffer = H3WindowManager::Get()->GetDrawBuffer();
    const RECT clip{8, 8, (std::min)(buffer->width, H3GameWidth::Get() - 200),
                    (std::min)(buffer->height, H3GameHeight::Get() - 48)};
    const int left = screenX * 32 + adv.screenDrawOffset.x;
    const int top = screenY * 32 + adv.screenDrawOffset.y + 8;
    TileGlow glow;
    const unsigned tick = GetTickCount() / 128;
    // Match the native object layer/vector order. Solid pieces above the target
    // erase its glow where they occlude it, while transparent areas preserve it.
    bool started = false;
    for (unsigned i = 0; i < pieceCount; ++i)
    {
        const auto &piece = pieces[i];
        // Objects below the first effect cannot occlude it.
        if (piece.resource < 0 && !started) continue;
        started = true;
        const auto &draw = *piece.draw;
        const auto attributes = piece.attributes;
        const auto &details = map->objectDetails[draw.sprite];
        if (details.num >= map->defs.Count())
            continue;
        const auto def = map->defs[details.num];
        if (!def || !def->groups || def->groupsCount <= 0 || !def->groups[0] ||
            !def->groups[0]->frames || def->groups[0]->count <= 0)
            continue;
        const auto group = def->groups[0];
        const unsigned frameIndex = (unsigned(adv.refreshCounter) + details.frameRandomness) % group->count;
        const auto frame = group->frames[frameIndex];
        const auto mask = frame ? GetFrameMask(*frame, def->palette565) : nullptr;
        if (!mask)
            continue;
        const int srcX = (attributes->width - (draw.tileID & 15) - 1) * 32;
        const int srcY = (attributes->height - (draw.tileID >> 4) - 1) * 32;
        CompositePiece(*mask, srcX, srcY, piece.resource, tick + piece.phase, glow);
    }
    const bool is32bit = H3BitMode::Get() == 4;
    for (int y = 0; y < 32; ++y)
    {
        auto pixels = glow.rows[y];
        while (pixels)
        {
            unsigned long x;
            _BitScanForward(&x, pixels);
            pixels &= pixels - 1;
            DrawPixel(*buffer, clip, left + x, top + y, glow.colour[y * 32 + x], is32bit);
        }
    }
}

void __stdcall DrawFog(HiHook *hook, H3AdventureManager *adv, int x, int y, int z, int screenX, int screenY)
{
    const auto start = profile.enabled ? Clock() : 0;
    try { DrawObjectGlow(*adv, x, y, z, screenX, screenY); }
    catch (const std::bad_alloc &) { /* Keep the native fog pass on allocation failure. */ }
    const auto nativeStart = profile.enabled ? Clock() : 0;
    THISCALL_6(void, hook->GetDefaultFunc(), adv, x, y, z, screenX, screenY);
    if (profile.enabled)
    {
        ++profile.calls;
        profile.glowTicks += nativeStart - start;
        profile.fogTicks += Clock() - nativeStart;
    }
}
}

bool InstallHooks()
{
    if (hooksReady) return true;
    // Native thiscall, five stack arguments, both exits ret 14h. The function
    // splice also covers HD's map pass and chains with GEM's call-site hook.
    Patch *hooks[] = {_PI->WriteHiHook(0x412180, SPLICE_, EXTENDED_, THISCALL_, DrawFog)};
    hooksReady = CheckHooks(hooks, "Object glow");
    return hooksReady;
}
void ResetMap()
{
    overrides.Clear();
    frameMasks.clear();
    maskBytes = 0;
    if (decodeSurfaces)
    {
        H3Pcx16Loader black(decodeSurfaces->black.Release()), white(decodeSurfaces->white.Release());
    }
}
}
}

extern "C" int __stdcall Erm_SetObjectGlow(int type, int subtype, int enabled) noexcept
{
    try { return erm_functions::glow::overrides.Register(type, subtype, enabled); }
    catch (const std::bad_alloc &) { return 0; }
}
extern "C" int __stdcall Erm_SetObjectGlowEnabled(int enabled) noexcept
{
    if (enabled != 0 && enabled != 1)
        return 0;
    erm_functions::glow::enabled = enabled != 0;
    return 1;
}
extern "C" int __stdcall Erm_GetObjectGlowEnabled() noexcept
{
    return erm_functions::glow::enabled;
}

extern "C" int __stdcall Erm_SetObjectGlowProfiling(int enabled) noexcept
{
    using namespace erm_functions::glow;
    if (enabled != 0 && enabled != 1) return 0;
    if (enabled)
    {
        profile = {};
        profile.peakBytes = maskBytes;
        if (decodeSurfaces && decodeSurfaces->black && decodeSurfaces->white)
            profile.peakScratchBytes = std::uint64_t(decodeSurfaces->black->buffSize) + decodeSurfaces->white->buffSize;
    }
    profile.enabled = enabled != 0;
    return 1;
}

extern "C" int __stdcall Erm_GetObjectGlowProfile(erm_functions::ObjectGlowProfile *output) noexcept
{
    if (!output || output->structSize < sizeof(*output) || output->version != 1) return 0;
    const auto &sample = erm_functions::glow::profile;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    output->counterFrequency = frequency.QuadPart;
    output->calls = sample.calls; output->tiles = sample.tiles;
    output->hits = sample.hits; output->misses = sample.misses; output->flushes = sample.flushes;
    output->glowTicks = sample.glowTicks; output->fogTicks = sample.fogTicks; output->decodeTicks = sample.decodeTicks;
    output->peakMaskBytes = sample.peakBytes; output->peakScratchBytes = sample.peakScratchBytes;
    return 1;
}
