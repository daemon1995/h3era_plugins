#pragma once

#include <cstdint>
#include <string>

namespace era_options
{
constexpr int WogOptionCount = 1000;
constexpr const char *WogProfileName = "WoGSetup_ERA_OptionsMenu.dat";

// File-only adapter, independently testable without loading the game.
// directory must end in a slash. Only our managed profile is overwritten.
bool SaveWogProfileFiles(const std::string &directory, const std::int32_t *values, std::string *error = nullptr);
} // namespace era_options
