#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace erm_functions
{
inline bool ValidOwner(int owner) noexcept { return owner >= -1 && owner < 8; }

class ObjectTypes
{
    std::unordered_map<std::uint64_t, bool> types;
    static std::uint64_t Key(int type, int subtype) noexcept
    {
        return (std::uint64_t(std::uint32_t(type)) << 32) | std::uint32_t(subtype);
    }
public:
    bool Register(int type, int subtype, int minimap)
    {
        if (type < 0 || type >= 232 || subtype < -1 || subtype > 32767 || (minimap != 0 && minimap != 1))
            return false;
        types[Key(type, subtype)] = minimap != 0;
        return true;
    }
    const bool *Find(int type, int subtype) const noexcept
    {
        auto entry = types.find(Key(type, subtype));
        if (entry == types.end())
            entry = types.find(Key(type, -1));
        return entry == types.end() ? nullptr : &entry->second;
    }
    void Clear() noexcept { types.clear(); }
};

struct ReplayEvent
{
    enum Kind { OWNER, ERM } kind = ERM;
    int boundary = 0; // Number of native actions that precede this event.
    std::uint32_t coords = 0;
    int oldOwner = -1;
    int newOwner = -1;
    int redoFunction = 0;
    int undoFunction = 0;
    std::array<int, 16> args{};
};

// Sidecar to the native action list: no game-owned pointers, vtables or savegame entries.
class ReplayTimeline
{
    std::vector<ReplayEvent> events;
    std::uint64_t generation = 0;
    int lastNativeCount = 0;
    bool replaying = false;
public:
    std::unordered_map<std::uint32_t, int> ownerView;

    void Clear() noexcept
    {
        ++generation;
        replaying = false;
        lastNativeCount = 0;
        events.clear();
        ownerView.clear();
    }
    bool Add(ReplayEvent event, int nativeCount)
    {
        if (replaying || nativeCount < 0)
            return false;
        if (nativeCount < lastNativeCount)
            Clear();
        event.boundary = nativeCount;
        events.push_back(event);
        lastNativeCount = nativeCount;
        return true;
    }
    bool IsReplaying() const noexcept { return replaying; }
    std::uint64_t Generation() const noexcept { return generation; }
    bool Begin() noexcept
    {
        if (replaying)
            return false;
        ownerView.clear();
        replaying = true;
        return true;
    }
    void End(std::uint64_t expectedGeneration) noexcept
    {
        if (generation == expectedGeneration)
        {
            replaying = false;
            ownerView.clear();
        }
    }
    template<class Callback> void AtBoundary(int boundary, bool undo, Callback callback)
    {
        if (!replaying)
            return;
        const auto expectedGeneration = generation;
        const auto first = std::lower_bound(events.begin(), events.end(), boundary,
            [](const ReplayEvent &event, int value) { return event.boundary < value; });
        const auto last = std::upper_bound(first, events.end(), boundary,
            [](int value, const ReplayEvent &event) { return value < event.boundary; });
        const auto start = std::size_t(first - events.begin());
        const auto count = std::size_t(last - first);
        for (std::size_t step = 0; step < count; ++step)
        {
            const auto index = start + (undo ? count - step - 1 : step);
            // Copy before invoking ERM: loading/quitting from a callback can clear this vector.
            const ReplayEvent event = events[index];
            callback(event, undo);
            if (generation != expectedGeneration || !replaying)
                return;
        }
    }
};
}
