#pragma once

#include "OptionRegistry.h"

#include <functional>

namespace era_options
{
// ERA flattens Lang JSON to dotted scalar keys. False means missing;
// an empty string may still be a present translation.
using JsonReader = std::function<bool(const std::string &key, std::string &value)>;
struct JsonIssue { std::string path; std::string message; };
struct JsonLoadResult
{
    size_t registered = 0;
    size_t alreadyRegistered = 0;
    std::vector<JsonIssue> issues;
};

bool ReadJsonOption(const JsonReader &read, const std::string &path, const std::string &modFolder,
                    const std::string &modName, OptionDefinition &definition, std::string &error);
JsonLoadResult LoadJsonOptions(OptionRegistry &registry, const JsonReader &read,
                               const std::vector<std::string> &modFolders);
} // namespace era_options
