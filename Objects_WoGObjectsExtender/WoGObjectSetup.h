#pragma once
#include <cstdint>

namespace wog
{
namespace setup
{
// SetObjectAsVisited (0x4FC620) writes the team's eight player bits at [05, 13).
// The high bit marks a native word. Periods are shared by all eight player bits;
// advancing a week/month starts a fresh mask without a map scan or daily reset.
struct PeriodVisitors
{
    union
    {
        uint32_t word;
        struct
        {
            unsigned reserved : 5;    // [00]
            unsigned visited : 8;     // [05] Players who visited during this period.
            unsigned period : 18;     // [13] Week or month shared by the visitor mask.
            unsigned initialized : 1; // [31] Native setup marker.
        };
    };
    static constexpr uint32_t MAX_PERIOD = 0x3FFFF;
    bool IsPeriod(int currentPeriod) const noexcept
    {
        return currentPeriod > 0 && initialized && period == uint32_t(currentPeriod);
    }
    bool UpdatePeriod(int currentPeriod) noexcept
    {
        if (currentPeriod <= 0 || uint32_t(currentPeriod) > MAX_PERIOD)
            return false;
        if (!IsPeriod(currentPeriod))
            visited = 0;
        period = currentPeriod;
        initialized = true;
        return true;
    }
};
struct DayStamp
{
    union
    {
        uint32_t word;
        struct
        {
            unsigned day : 31;        // [00] Object-wide service day/week; no player visitors.
            unsigned initialized : 1; // [31] Native setup marker.
        };
    };
    int Day() const noexcept
    {
        return initialized ? int(day) : 0;
    }
    static DayStamp At(int day) noexcept
    {
        DayStamp state{0};
        state.day = day;
        state.initialized = true;
        return state;
    }
};
struct DeathChamber
{
    union
    {
        uint32_t word;
        struct
        {
            unsigned baseDifficulty : 2; // [00] Difficulty minus one, 0..3.
            unsigned clearedDay : 28;   // [02] Shared victory day; no player visitors.
            unsigned reserved : 1;       // [30] Zero in initialized state.
            unsigned initialized : 1;    // [31] Native setup marker.
        };
    };
    // Distinguish initialized state from the engine's default setup value -1.
    bool Initialized() const noexcept
    {
        return initialized && !reserved;
    }
    int BaseDifficulty() const noexcept
    {
        return 1 + baseDifficulty;
    }
    int ClearedDay() const noexcept
    {
        return Initialized() ? int(clearedDay) : 0;
    }
    static DeathChamber At(int difficulty, int clearedDay) noexcept
    {
        DeathChamber state{0};
        state.baseDifficulty = difficulty - 1;
        state.clearedDay = clearedDay;
        state.initialized = true;
        return state;
    }
};
static_assert(sizeof(PeriodVisitors) == 4 && sizeof(DayStamp) == 4 && sizeof(DeathChamber) == 4,
              "Native object state must fit H3MapItem::setup");
} // namespace setup
} // namespace wog
