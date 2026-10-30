#pragma once
#include "framework.h"
#include <string>

namespace wog { namespace policy {
// Raw assoc values, shared with ERM: 0 = script, 1 = plugin. ERA serializes them per map.
std::string Key(int type, int subtype);
bool Handles(int type, int subtype) noexcept;
bool Handles(const H3MapItem *item) noexcept;
void InitializeDefaults() noexcept;
void RegisterEvents() noexcept;
bool LoadingGame() noexcept;
bool GameActive() noexcept;
} }
