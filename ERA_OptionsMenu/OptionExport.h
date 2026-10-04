#pragma once

#include "OptionRegistry.h"
#include <functional>

namespace era_options
{
std::string TextToUtf8(const std::string &text, unsigned codePage);
// All options, including hidden/disabled ones. Current values become defaults.
std::string SerializeOptionsJson(const OptionRegistry &registry, unsigned codePage,
    const std::function<int(const EOption &)> &readValue = {});
bool WriteOptionsJson(const std::string &pathUtf8, const std::string &json, std::string &error);
} // namespace era_options
