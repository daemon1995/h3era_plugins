#pragma once
#include <array>
#include <string>

namespace artifacts
{
constexpr int CAPACITY = 1024;
constexpr int NATIVE_MAP_ARTIFACTS = 144;
// Unchanged until the separate combination-mask migration.
constexpr int NATIVE_COMBINATION_BITS = 160;
constexpr int NATIVE_COMBINATIONS = 32;

void Log(const char *format, ...);

struct ArtifactData
{
    bool declared = false;
    std::string legacyKey;
    std::string image;
    std::string mapDef;
    int imageFrame = 0;
    std::array<bool, 4> primaryChanged{};
    int aiValue = 0;
    bool hasAiValue = false;

    static bool HasDefinition(const std::string &key);
    void Apply(H3ArtifactSetup &setup, signed char *primary, LPCSTR &event, int id);
    void ReloadText(H3ArtifactSetup &setup, LPCSTR &event, bool name, bool description, int id) const;
};
} // namespace artifacts
