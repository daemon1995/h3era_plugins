#include "OptionRegistry.h"

#include <climits>
#include <algorithm>

namespace era_options
{
bool RegisterResult::Succeeded() const noexcept
{
    return status == RegisterStatus::Registered || status == RegisterStatus::AlreadyRegistered;
}

RegisterResult OptionRegistry::Register(const OptionDefinition &source, const std::set<OptionId> &reserved)
{
    OptionDefinition definition = source;
    // Windows INI keys are case-insensitive; use the same identity policy in
    // memory so differently cased keys cannot share a saved value by accident.
    definition.key = LowerAscii(definition.key);
    definition.modFolder = LowerAscii(definition.modFolder);
    std::string error;
    if (!ValidateDefinition(definition, error))
        return {RegisterStatus::Invalid, InvalidOptionId, error};
    if (EOption *existing = Find(definition.key))
    {
        const auto &previous = existing->definition;
        if (previous.modFolder != definition.modFolder || previous.type != definition.type ||
            previous.choices.size() != definition.choices.size() ||
            previous.wogOption != definition.wogOption || previous.wogValues != definition.wogValues ||
            (definition.fixedId != InvalidOptionId && definition.fixedId != existing->id))
            return {RegisterStatus::Conflict, InvalidOptionId, "key is already registered with another owner, type, or ID"};
        // Refresh presentation without changing the ID, object address or value.
        if (existing->id < FirstDynamicOptionId)
        {
            const std::string tag = std::to_string(existing->id);
            if (std::find(definition.tags.begin(), definition.tags.end(), tag) == definition.tags.end())
                definition.tags.push_back(tag);
        }
        existing->definition = definition;
        return {RegisterStatus::AlreadyRegistered, existing->id, {}};
    }
    if (definition.wogOption >= 0)
        for (const auto &option : options)
            if (option->Definition().wogOption == definition.wogOption)
                return {RegisterStatus::Conflict, InvalidOptionId, "WoG index is already bound to another key"};
    OptionId id = definition.fixedId;
    if (id == InvalidOptionId)
    {
        while (nextDynamicId <= INT_MAX &&
            (byId.count(static_cast<OptionId>(nextDynamicId)) || reserved.count(static_cast<OptionId>(nextDynamicId))))
            ++nextDynamicId;
        if (nextDynamicId > INT_MAX)
            return {RegisterStatus::Exhausted, InvalidOptionId, "32-bit option IDs are exhausted"};
        id = static_cast<OptionId>(nextDynamicId);
    }
    else if (Find(id))
        return {RegisterStatus::Conflict, InvalidOptionId, "static ID is already assigned to another key"};

    if (id < FirstDynamicOptionId)
    {
        const std::string tag = std::to_string(id);
        if (std::find(definition.tags.begin(), definition.tags.end(), tag) == definition.tags.end())
            definition.tags.push_back(tag);
    }
    auto option = std::make_unique<EOption>(id, definition);
    EOption *pointer = option.get();
    options.push_back(std::move(option));
    byKey.emplace(definition.key, pointer);
    byId.emplace(id, pointer);
    if (definition.fixedId == InvalidOptionId)
        ++nextDynamicId;
    return {RegisterStatus::Registered, id, {}};
}

RegisterResult OptionRegistry::Register(const OptionDefinition &definition) { return Register(definition, {}); }

std::vector<RegisterResult> OptionRegistry::RegisterBatch(const std::vector<OptionDefinition> &definitions)
{
    std::set<OptionId> reserved;
    for (const auto &definition : definitions)
    {
        std::string error;
        if (definition.fixedId != InvalidOptionId && ValidateDefinition(definition, error))
            reserved.insert(definition.fixedId);
    }
    std::vector<RegisterResult> results;
    results.reserve(definitions.size());
    for (const auto &definition : definitions)
        results.push_back(Register(definition, reserved));
    return results;
}

EOption *OptionRegistry::Find(OptionId id) const noexcept
{
    const auto found = byId.find(id);
    return found == byId.end() ? nullptr : found->second;
}
EOption *OptionRegistry::Find(const std::string &key) const
{
    const auto found = byKey.find(LowerAscii(key));
    return found == byKey.end() ? nullptr : found->second;
}
OptionId OptionRegistry::GetId(const std::string &key) const
{
    const EOption *option = Find(key);
    return option ? option->GetId() : InvalidOptionId;
}
bool OptionRegistry::SetValue(OptionId id, int value)
{
    EOption *option = Find(id);
    return option && option->definition.enabled && RestoreValue(id, value);
}
bool OptionRegistry::RestoreValue(OptionId id, int value)
{
    EOption *option = Find(id);
    if (!option || !option->AcceptsValue(value))
        return false;
    option->value = value;
    return true;
}
const std::vector<std::unique_ptr<EOption>> &OptionRegistry::Options() const noexcept { return options; }
} // namespace era_options
