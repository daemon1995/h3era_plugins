#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace erm_functions
{
// Owners use -1 for neutral and 0..7 for players, matching the exported API.
inline bool ValidOwner(int owner) noexcept { return owner >= -1 && owner < 8; }

// Map-local type registrations. An exact subtype takes precedence over subtype -1.
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

// Sidecar event recorded at a native-action boundary; it owns no game memory.
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

// Save only ownership. Generic ERM callbacks may contain temporary pointers/IDs.
struct OwnerReplayRecord
{
    int boundary;
    std::uint32_t coords;
    int oldOwner, newOwner;
};
static_assert(sizeof(OwnerReplayRecord) == 16, "Owner replay save ABI");

// Undo/redo timeline layered over native actions. ownerView is temporary replay state;
// Clear starts a new map generation and invalidates callbacks already in progress.
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
    const std::vector<ReplayEvent> &Events() const noexcept { return events; }
    bool RestoreOwners(const std::vector<OwnerReplayRecord> &records, int nativeCount)
    {
        if (nativeCount < 0 || replaying)
            return false;
        std::vector<ReplayEvent> restored;
        restored.reserve(records.size());
        int previousBoundary = 0;
        for (const auto &record : records)
        {
            if (record.boundary < previousBoundary || record.boundary > nativeCount ||
                !ValidOwner(record.oldOwner) || !ValidOwner(record.newOwner))
                return false;
            ReplayEvent event;
            event.kind = ReplayEvent::OWNER;
            event.boundary = record.boundary;
            event.coords = record.coords;
            event.oldOwner = record.oldOwner;
            event.newOwner = record.newOwner;
            restored.push_back(event);
            previousBoundary = record.boundary;
        }
        Clear();
        events.swap(restored);
        lastNativeCount = nativeCount;
        return true;
    }
    void RemoveNativePrefix(int count) noexcept
    {
        if (count <= 0)
            return;
        // A callback can hand off the turn. Invalidate AtBoundary's indexes
        // before the native journal's new generation replaces the old one.
        if (replaying)
        {
            Clear();
            return;
        }
        // The game drops completed player turns from the front, without
        // clearing the remaining replay actions (0x49D330 at turn handoff).
        const auto firstKept = std::upper_bound(events.begin(), events.end(), count,
            [](int value, const ReplayEvent &event) { return value < event.boundary; });
        events.erase(events.begin(), firstKept);
        for (auto &event : events)
            event.boundary -= count;
        lastNativeCount = (std::max)(0, lastNativeCount - count);
    }
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
