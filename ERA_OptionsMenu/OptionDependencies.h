#pragma once
#include "OptionRegistry.h"

namespace era_options
{
using OptionValues = std::map<OptionId, int>;
struct DependencyState
{
    OptionValues values;
    std::map<OptionId, std::vector<OptionId>> blockers;
    std::set<OptionId> missingRequirements;
    std::string error;
    std::set<std::string> conflicts;
    void Conflict(const std::string &message) { error = message; conflicts.insert(message); }
    bool IsBlocked(OptionId id) const { return blockers.count(id) != 0; }
};

class OptionDependencies final
{
    struct Edge { OptionId source, target; int when, value; bool required; };
    std::vector<Edge> edges;
    std::vector<OptionId> order;
    std::set<OptionId> targets;
    std::set<OptionId> invalidSources;

  public:
    std::vector<std::string> Configure(const OptionRegistry &registry);
    DependencyState Evaluate(const OptionRegistry &registry, const OptionValues &requested,
                             const OptionValues &immutable = {}) const;
    const std::set<OptionId> &Targets() const { return targets; }
};
} // namespace era_options
