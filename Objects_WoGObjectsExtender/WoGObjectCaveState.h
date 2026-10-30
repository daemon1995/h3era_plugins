#pragma once
#include <cstdint>

namespace wog
{
namespace cave
{
// Four seven-bit chambers, two bits for branching, one entrance flag and an initialization bit.
// Saved directly in mapItem->setup; the cave has no player visitor mask.
struct State
{
    union
    {
        uint32_t word;
        struct
        {
            unsigned room0 : 7;           // [00] Chamber kind, parameters and cleared flag.
            unsigned room1 : 7;           // [07]
            unsigned room2 : 7;           // [14]
            unsigned room3 : 7;           // [21]
            unsigned branches : 2;        // [28] Number of chambers minus one.
            unsigned entranceCleared : 1; // [30]
            unsigned initialized : 1;     // [31] Native setup marker.
        };
    };
    bool Initialized() const { return initialized; }
    int Count() const { return 1 + branches; }
    int Room(int index) const
    {
        switch (index)
        {
        case 0: return room0;
        case 1: return room1;
        case 2: return room2;
        case 3: return room3;
        default: return 0;
        }
    }
    void Room(int index, int room)
    {
        switch (index)
        {
        case 0: room0 = room; break;
        case 1: room1 = room; break;
        case 2: room2 = room; break;
        case 3: room3 = room; break;
        }
    }
    void Finish(int index) { Room(index, Room(index) | 64); }
    bool EntranceCleared() const { return entranceCleared; }
    void ClearEntrance() { entranceCleared = true; }
    static State Fresh(int branches, const int (&rooms)[4])
    {
        State state{0};
        state.branches = branches;
        state.initialized = true;
        for (int i = 0; i < 4; ++i)
            state.Room(i, rooms[i]);
        return state;
    }
};
static_assert(sizeof(State) == 4, "Cave state must fit in the map item control word");
} // namespace cave
} // namespace wog
