#include "pch.h"
#include "ExtendedMonoliths.h"

#include <cstring>

namespace monoliths
{
namespace
{
static_assert(sizeof(H3Vector<UINT32>) == 0x10, "Native monolith hooks require the 16-byte H3Vector layout.");
constexpr LPCSTR SAVE_SECTION = "RMG_CustomizeObjectProperties.TwoWayMonoliths";

bool IsMapPosition(UINT32 position)
{
    const H3Position coordinates(position);
    const auto *game = P_Game->Get();
    return coordinates.GetX() < UINT(game->mainSetup.mapSize) &&
           coordinates.GetY() < UINT(game->mainSetup.mapSize) &&
           coordinates.GetZ() < (game->mainSetup.hasUnderground ? 2 : 1);
}
} // namespace

ExtendedMonoliths::ExtendedMonoliths() : IGamePatch("EraPlugin.RMG.ExtendedMonoliths.daemon_n")
{
    CreatePatches();
}

ExtendedMonoliths &ExtendedMonoliths::Get()
{
    // The vectors use the game's allocator; keep their lifetime under game events,
    // rather than running static destructors after the game allocator shuts down.
    static auto *instance = new ExtendedMonoliths;
    return *instance;
}

void ExtendedMonoliths::CreatePatches()
{
    if (m_isInited)
        return;
    // These instructions address the native vector object, not its m_first field.
    // In the supplied ERA executable their bases are 4E67C / 4E6FC. The extra
    // groups do not depend on the offsets declared for these fields in H3API.
    const BYTE registration[] = {0x8B, 0x94, 0x18, 0x80, 0xE6, 0x04, 0x00,
                                 0x8D, 0x8C, 0x18, 0x7C, 0xE6, 0x04, 0x00};
    const BYTE destinations[] = {0x8D, 0x94, 0x08, 0x7C, 0xE6, 0x04, 0x00};
    if (std::memcmp(reinterpret_cast<const void *>(0x4C1413), registration, sizeof(registration)) ||
        std::memcmp(reinterpret_cast<const void *>(0x4CDAC3), destinations, sizeof(destinations)) ||
        std::memcmp(reinterpret_cast<const void *>(0x56B151), destinations, sizeof(destinations)))
    {
        return;
    }

    if (!_pi->WriteLoHook(0x4C1413, Game_RegisterTwoWayMonolith) ||
        !_pi->WriteLoHook(0x4CDAC3, Game_GetTwoWayDestinations) ||
        !_pi->WriteLoHook(0x56B151, Pathfinder_GetTwoWayDestinations) ||
        !_pi->WriteHiHook(0x543730, THISCALL_, Rmg_CreateZoneConnections))
    {
        _pi->UndoAll();
        return;
    }

    m_isInited = true;
    Era::RegisterHandler(OnSavegameWrite, "OnSavegameWrite");
    Era::RegisterHandler(OnSavegameRead, "OnSavegameRead");
    Era::RegisterHandler(OnAfterLoadGame, "OnAfterLoadGame");
    Era::RegisterHandler(OnGameLeave, "OnGameLeave");
}

void ExtendedMonoliths::ResetForNewMap()
{
    groups.Clear();
    emptyGroup.RemoveAll();
    loadedGroups.clear();
    loadState = LoadState::Idle;
}

H3Vector<UINT32> *ExtendedMonoliths::FindGroup(int subtype) noexcept
{
    auto *group = groups.Find(subtype);
    return group ? group : &emptyGroup;
}

void __stdcall ExtendedMonoliths::Rmg_CreateZoneConnections(HiHook *hook, H3RmgRandomMapGenerator *rmg)
{
    // 543730 cycles through the complete prototype list; 5431D0 places paired
    // type-45 objects with the selected subtype. Added objects.txt definitions
    // already enter this list through the host's shared 515038 loader hook.
    THISCALL_1(void, hook->GetDefaultFunc(), rmg);
}

_LHF_(ExtendedMonoliths::Game_RegisterTwoWayMonolith)
{
    const auto *mapItem = c->Esi<H3MapItem *>();
    const int subtype = mapItem->objectSubtype;
    if (subtype >= 0 && subtype < ORIGINAL_GROUP_COUNT)
        return EXEC_DEFAULT;

    if (!IsExtendedSubtype(subtype))
    {
        c->Esi<H3MapItem *>()->monolith.index = -1;
        c->return_address = 0x4C1476;
        return NO_EXEC_DEFAULT;
    }

    auto &group = Get().groups.GetOrCreate(subtype);
    c->ecx = reinterpret_cast<int>(&group);
    c->edx = reinterpret_cast<int>(group.begin());
    c->return_address = 0x4C1421; // Skip both native indexed accesses.
    // The remaining native branch sets monolith.index and appends the packed
    // coordinates using its own H3Vector insertion routine.
    return NO_EXEC_DEFAULT;
}

_LHF_(ExtendedMonoliths::Game_GetTwoWayDestinations)
{
    const int subtype = c->Arg<int>(1);
    if (subtype >= 0 && subtype < ORIGINAL_GROUP_COUNT)
        return EXEC_DEFAULT;
    c->edx = reinterpret_cast<int>(Get().FindGroup(subtype));
    c->return_address = 0x4CDACA;
    // Keep 4CD840's filtering, random selection, occupied exits and source index.
    return NO_EXEC_DEFAULT;
}

_LHF_(ExtendedMonoliths::Pathfinder_GetTwoWayDestinations)
{
    const int subtype = c->Edi<H3MapItem *>()->objectSubtype;
    if (subtype >= 0 && subtype < ORIGINAL_GROUP_COUNT)
        return EXEC_DEFAULT;
    c->edx = reinterpret_cast<int>(Get().FindGroup(subtype));
    c->return_address = 0x56B158;
    // Pass the extra vector to native 56A6D0, retaining its route/AI semantics.
    return NO_EXEC_DEFAULT;
}

void __stdcall ExtendedMonoliths::OnSavegameWrite(Era::TEvent *)
{
    const auto &groups = Get().groups.Entries();
    size_t positionCount = 0;
    for (const auto &entry : groups)
        positionCount += entry.second.Size();
    SaveDataBuilder builder(groups.size(), positionCount);
    for (const auto &entry : groups)
        builder.AppendGroup(uint32_t(entry.first), entry.second);
    auto words = builder.Take();
    Era::WriteSavegameSection(int(words.size() * sizeof(uint32_t)), words.data(), SAVE_SECTION);
}

void __stdcall ExtendedMonoliths::OnSavegameRead(Era::TEvent *)
{
    auto &instance = Get();
    instance.ResetForNewMap();
    instance.loadState = LoadState::Rebuild;
    uint32_t header[SAVE_HEADER_WORDS]{};
    const int read = Era::ReadSavegameSection(sizeof(header), header, SAVE_SECTION);
    if (!read) // A save made before this extension has no custom section.
        return;

    size_t wordCount = 0;
    if (read != sizeof(header) || !GetSaveWordCount(header, wordCount))
    {
        return;
    }

    std::vector<uint32_t> words(header, header + SAVE_HEADER_WORDS);
    words.resize(wordCount);
    const int remainingBytes = int((wordCount - SAVE_HEADER_WORDS) * sizeof(uint32_t));
    if ((remainingBytes && Era::ReadSavegameSection(remainingBytes, words.data() + SAVE_HEADER_WORDS,
                                                   SAVE_SECTION) != remainingBytes) ||
        !DecodeGroups(words, instance.loadedGroups))
    {
        return;
    }
    instance.loadState = LoadState::Restore;
}

bool ExtendedMonoliths::RestoreLoadedGroups()
{
    for (const auto &group : loadedGroups)
        for (const UINT32 position : group.positions)
            if (!IsMapPosition(position))
                return false;

    for (const auto &group : loadedGroups)
    {
        auto &positions = groups.GetOrCreate(int(group.subtype));
        positions.Reserve(UINT(group.positions.size()));
        for (const UINT32 position : group.positions)
            positions.Add(position);
    }
    return true;
}

void ExtendedMonoliths::RebuildFromMap()
{
    groups.Clear();
    auto map = P_Game->GetMap();
    for (auto it = map.begin(), end = map.end(); it != end; ++it)
    {
        H3MapItem *item = &it;
        // Match 4C0980: only yellow entrance tiles represent teleport endpoints.
        if (!(item->access & 0x10))
            continue;
        int subtype = item->objectSubtype;
        UINT32 *index = &item->setup;
        if (item->objectType == eObject::HERO)
        {
            auto *hero = P_Game->GetHero(item->hero.index);
            if (!hero || !hero->objectBelow || hero->objectTypeUnder != eObject::MONOLITH_TWO_WAY)
                continue;
            // Hero::Show (4D7840) changes the tile's type/setup, but leaves
            // objectSubtype on the tile. Only setup is saved in the hero.
            index = &hero->objectBelowSetup;
        }
        else if (item->objectType != eObject::MONOLITH_TWO_WAY)
            continue;

        if (!IsExtendedSubtype(subtype))
            continue;
        auto &group = groups.GetOrCreate(subtype);
        *index = group.Size();
        group.Add(H3Position::Pack(it.GetX(), it.GetY(), it.GetZ()));
    }
}

void __stdcall ExtendedMonoliths::OnAfterLoadGame(Era::TEvent *)
{
    auto &instance = Get();
    // Read and post-load events are separate phases. Consume each read once.
    if (instance.loadState == LoadState::Idle)
        return;
    if (instance.loadState != LoadState::Restore || !instance.RestoreLoadedGroups())
    {
        instance.RebuildFromMap();
    }
    instance.loadedGroups.clear();
    instance.loadState = LoadState::Idle;
}

void __stdcall ExtendedMonoliths::OnGameLeave(Era::TEvent *)
{
    Get().ResetForNewMap();
}
} // namespace monoliths
