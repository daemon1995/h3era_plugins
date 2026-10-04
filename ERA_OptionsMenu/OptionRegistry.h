#pragma once

#include "EOption.h"

#include <map>
#include <memory>
#include <set>
#include <unordered_map>

namespace era_options
{
enum class RegisterStatus
{
    Registered = 1, AlreadyRegistered = 2, Invalid = -1, Conflict = -2, Exhausted = -3
};

struct RegisterResult
{
    RegisterStatus status = RegisterStatus::Invalid;
    OptionId id = InvalidOptionId;
    std::string message;
    bool Succeeded() const noexcept;
};

class OptionRegistry final
{
    std::vector<std::unique_ptr<EOption>> options;
    std::unordered_map<std::string, EOption *> byKey;
    std::map<OptionId, EOption *> byId;
    std::int64_t nextDynamicId = FirstDynamicOptionId;
    RegisterResult Register(const OptionDefinition &definition, const std::set<OptionId> &reserved);

  public:
    RegisterResult Register(const OptionDefinition &definition);
    // Reserve explicit IDs before assigning any dynamic IDs in this load.
    std::vector<RegisterResult> RegisterBatch(const std::vector<OptionDefinition> &definitions);
    EOption *Find(OptionId id) const noexcept;
    EOption *Find(const std::string &key) const;
    OptionId GetId(const std::string &key) const;
    bool SetValue(OptionId id, int value);
    bool RestoreValue(OptionId id, int value);
    const std::vector<std::unique_ptr<EOption>> &Options() const noexcept;
};
} // namespace era_options
