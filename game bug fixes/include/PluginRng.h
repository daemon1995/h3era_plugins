#pragma once
#include <cstdint>

// Match the game's 15-bit LCG for independent cosmetic/AI streams.
struct PluginRNG
{
  private:
    uint32_t seed = 123456;
  public:
    void srand(uint32_t value) { seed = value; }
    int next()
    {
        seed = seed * 214013u + 2531011u;
        return (seed >> 16) & 0x7FFF;
    }
    int range(int low, int high)
    {
        if (high <= low)
            return low;
        const uint64_t span = static_cast<uint64_t>(static_cast<int64_t>(high) - low) + 1;
        return static_cast<int>(static_cast<int64_t>(low) + static_cast<int64_t>(next() % span));
    }
};

// Shared by animations and the deterministic battle intro, never by music.
inline PluginRNG &CombatVisualRng()
{
    static thread_local PluginRNG rng;
    return rng;
}