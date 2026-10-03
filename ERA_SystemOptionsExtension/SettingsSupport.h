#pragma once
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace sysopts
{
constexpr const char *SETTINGS_FILE = "Runtime/era_system_options.ini";
constexpr const char *LEGACY_GAME_FILE = "heroes3.ini";
constexpr const char *LEGACY_HEALTH_FILE = "Runtime/game_enhancement_mod.ini";

class SaveState
{
    bool dirty = false;
    unsigned int revision = 0;

  public:
    void MarkDirty() noexcept
    {
        dirty = true;
        ++revision;
    }
    bool IsDirty() const noexcept
    {
        return dirty;
    }
    unsigned int Revision() const noexcept
    {
        return revision;
    }
    void Saved(const bool success) noexcept
    {
        dirty = !success;
    }
};

inline bool ParseInteger(const char *text, int &value) noexcept
{
    if (!text)
        return false;
    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || errno == ERANGE || parsed < (std::numeric_limits<int>::min)() ||
        parsed > (std::numeric_limits<int>::max)())
        return false;
    while (std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    if (*end)
        return false;
    value = static_cast<int>(parsed);
    return true;
}

inline bool ParseFloat(const char *text, float &value, const float minValue, const float maxValue) noexcept
{
    if (!text)
        return false;
    char *end = nullptr;
    errno = 0;
    const double parsed = std::strtod(text, &end);
    if (end == text || errno == ERANGE || !std::isfinite(parsed) || parsed < minValue || parsed > maxValue)
        return false;
    while (std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    if (*end)
        return false;
    value = static_cast<float>(parsed);
    return true;
}

// A present value in the new file always wins, including an invalid value.
// Legacy values are consulted only before the first complete save.
template <class Reader>
bool ReadMigrated(Reader read, const char *key, const char *section, const bool migrate, const char *legacyKey,
                  const char *legacySection, const char *legacyFile, char *buffer)
{
    if (read(key, section, SETTINGS_FILE, buffer))
        return true;
    return migrate && legacyFile && read(legacyKey, legacySection, legacyFile, buffer);
}

inline int NormalizedToTick(const float value, const int ticksCount) noexcept
{
    if (ticksCount <= 1 || !std::isfinite(value))
        return 0;
    const float normalized = (std::max)(0.0f, (std::min)(value, 1.0f));
    return static_cast<int>(std::lround(normalized * (ticksCount - 1)));
}

inline float TickToNormalized(const int tick, const int ticksCount) noexcept
{
    if (ticksCount <= 1)
        return 0.0f;
    return static_cast<float>((std::max)(0, (std::min)(tick, ticksCount - 1))) / (ticksCount - 1);
}

inline bool CanChangeCombatOption(const bool inCombat, const bool requiresReconstruction) noexcept
{
    return !inCombat || !requiresReconstruction;
}

inline bool CanChangeVolume(const bool hasManager, const bool hasDriver, const int oldVolume,
                            const int newVolume) noexcept
{
    return oldVolume == newVolume || (hasManager && (hasDriver || oldVolume != 0 || newVolume == 0));
}
} // namespace sysopts
