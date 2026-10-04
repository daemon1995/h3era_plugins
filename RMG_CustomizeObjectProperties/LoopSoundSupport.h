#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sound
{
constexpr std::uint16_t ALL_OBJECT_SUBTYPES = 0xFFFF;

constexpr std::uint32_t MakeObjectSoundKey(std::uint16_t type, std::uint16_t subtype) noexcept
{
    return (std::uint32_t(type) << 16) | std::uint32_t(subtype);
}

inline int FindLoopSoundOverride(const std::unordered_map<std::uint32_t, int> &indexes,
                                 std::uint16_t type, std::uint16_t subtype, int undefined) noexcept
{
    const auto exact = indexes.find(MakeObjectSoundKey(type, subtype));
    if (exact != indexes.end())
        return exact->second; // An explicit silent entry (-1) also overrides the type default.

    const auto typeDefault = indexes.find(MakeObjectSoundKey(type, ALL_OBJECT_SUBTYPES));
    return typeDefault != indexes.end() ? typeDefault->second : undefined;
}

// Rebuild the flags for each trim pass; flags from a previous position must not
// keep obsolete WAVs in the cache. CurrentSounds has the native loopSound field.
template <class CurrentSounds>
int PrepareLoopSoundStates(std::vector<int> &states, const CurrentSounds &currentSounds) noexcept
{
    std::fill(states.begin(), states.end(), 0);
    int usedSounds = 0;
    for (const auto &currentSound : currentSounds)
    {
        const int soundId = currentSound.loopSound;
        if (soundId >= 0 && std::size_t(soundId) < states.size())
        {
            states[soundId] = 1;
            ++usedSounds;
        }
    }
    return usedSounds;
}
} // namespace sound
