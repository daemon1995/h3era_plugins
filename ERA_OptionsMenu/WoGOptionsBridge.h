#pragma once

#include "EOption.h"
#include <utility>

namespace era_options
{
bool RetireLegacyObjectBansForNewMap();
bool ReadWogRawValue(int index, int &value, bool currentMap = false);
bool ReadWogValue(const EOption &option, int &value, bool *locked = nullptr, bool currentMap = false);
bool ApplyWogValues(const std::vector<std::pair<const EOption *, int>> &values, std::string &error, bool currentMap = false);
bool SaveWogProfile(std::string *error = nullptr);
} // namespace era_options
