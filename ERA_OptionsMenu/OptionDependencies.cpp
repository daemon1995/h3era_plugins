#include "OptionDependencies.h"
#include <algorithm>
#include <queue>

namespace era_options
{
std::vector<std::string> OptionDependencies::Configure(const OptionRegistry &registry)
{
    edges.clear(); order.clear(); targets.clear(); invalidSources.clear();
    std::vector<std::string> issues;
    std::map<OptionId, const EOption *> options;
    std::map<std::string, std::vector<OptionId>> tags;
    for (const auto &option : registry.Options())
    {
        options.emplace(option->GetId(), option.get());
        for (const auto &tag : option->Definition().tags) tags[LowerAscii(tag)].push_back(option->GetId());
    }
    std::vector<Edge> candidates;
    for (const auto &entry : options)
    {
        const auto *source = entry.second;
        const auto collect = [&](OptionId id, const std::string &tag, int when, int value, bool required) {
            const auto warn = [&](const char *reason) {
                issues.push_back(source->GetKey() + ": " + reason);
                if (required) invalidSources.insert(source->GetId());
            };
            std::vector<OptionId> found;
            if (id >= 0) { if (registry.Find(id)) found.push_back(id); }
            else if (tags.count(LowerAscii(tag))) found = tags[LowerAscii(tag)];
            if (found.empty()) { warn("dependency target was not found"); return; }
            for (const auto target : found)
            {
                if (target == source->GetId())
                {
                    if (id >= 0 || (required && when != value)) warn("an option cannot depend on itself");
                    continue; // A shared tag can contain the source itself.
                }
                if (!registry.Find(target)->AcceptsValue(value)) { warn("dependency target state is out of range"); continue; }
                candidates.push_back({source->GetId(), target, when, value, required});
            }
        };
        for (const auto &rule : source->Definition().requires) collect(rule.id, rule.tag, rule.when, rule.value, true);
        for (const auto &rule : source->Definition().disables) collect(rule.id, rule.tag, rule.when, rule.value, false);
    }
    // Trace every activation, including transitive requirements and exclusions.
    // Contradictory definitions block their source rather than applying half.
    std::set<std::pair<OptionId, int>> seeds;
    for (const auto &edge : candidates) seeds.emplace(edge.source, edge.when);
    for (const auto &seed : seeds)
    {
        OptionValues implied = {{seed.first, seed.second}};
        std::vector<OptionId> pending = {seed.first};
        bool contradiction = false;
        while (!pending.empty() && !contradiction)
        {
            const auto id = pending.back(); pending.pop_back();
            for (const auto &edge : candidates)
                if (edge.source == id && implied[id] == edge.when)
                {
                    const auto added = implied.emplace(edge.target, edge.value);
                    if (added.second) pending.push_back(edge.target);
                    else if (added.first->second != edge.value) { contradiction = true; break; }
                }
        }
        if (contradiction && invalidSources.insert(seed.first).second)
            issues.push_back(options[seed.first]->GetKey() + ": contradictory requirements/exclusions; option is blocked");
    }
    std::map<OptionId, std::set<OptionId>> links;
    std::map<OptionId, int> incoming;
    for (const auto &entry : options) incoming[entry.first] = 0;
    for (const auto &edge : candidates)
    {
        if (invalidSources.count(edge.source)) continue;
        // Requirements run after their prerequisites, exclusions before their targets.
        const auto before = edge.required ? edge.target : edge.source;
        const auto after = edge.required ? edge.source : edge.target;
        std::vector<OptionId> pending = {after};
        std::set<OptionId> visited;
        bool cycle = false;
        while (!pending.empty())
        {
            const auto node = pending.back(); pending.pop_back();
            if (node == before) { cycle = true; break; }
            if (!visited.insert(node).second) continue;
            for (const auto child : links[node]) pending.push_back(child);
        }
        if (cycle)
        {
            issues.push_back(options[edge.source]->GetKey() + ": cyclic dependency was ignored");
            if (edge.required) invalidSources.insert(edge.source);
            continue;
        }
        if (links[before].insert(after).second) ++incoming[after];
        edges.push_back(edge);
        targets.insert(edge.required ? edge.source : edge.target);
    }
    std::priority_queue<OptionId, std::vector<OptionId>, std::greater<OptionId>> ready;
    for (const auto &entry : incoming) if (!entry.second) ready.push(entry.first);
    while (!ready.empty())
    {
        const auto id = ready.top(); ready.pop(); order.push_back(id);
        for (const auto target : links[id]) if (--incoming[target] == 0) ready.push(target);
    }
    return issues;
}

DependencyState OptionDependencies::Evaluate(const OptionRegistry &registry, const OptionValues &requested,
                                             const OptionValues &immutable) const
{
    DependencyState state;
    for (const auto &option : registry.Options())
    {
        const auto found = requested.find(option->GetId());
        state.values[option->GetId()] = found != requested.end() && option->AcceptsValue(found->second) ?
            found->second : option->GetValue();
    }
    std::map<OptionId, std::pair<OptionId, int>> forced;
    for (const auto id : order)
    {
        const auto fixed = immutable.find(id);
        const auto constraint = forced.find(id);
        if (fixed != immutable.end())
        {
            state.values[id] = fixed->second;
            if (constraint != forced.end() && fixed->second != constraint->second.second)
                state.Conflict("dependency cannot change locked option: " + registry.Find(id)->GetKey());
        }
        else if (constraint != forced.end()) state.values[id] = constraint->second.second;
        if (invalidSources.count(id))
        {
            state.blockers[id].push_back(id);
            if (fixed == immutable.end()) state.values[id] = 0;
            else if (fixed->second != 0) state.Conflict("invalid dependencies on locked option: " + registry.Find(id)->GetKey());
            continue;
        }
        for (const auto &edge : edges)
            if (edge.required && edge.source == id && state.values[edge.target] != edge.value)
            {
                state.missingRequirements.insert(id);
                state.blockers[id].push_back(edge.target);
                if (state.values[id] == edge.when)
                {
                    if (fixed != immutable.end()) state.Conflict("locked option has an unmet requirement: " + registry.Find(id)->GetKey());
                    else state.values[id] = 0;
                }
            }
        for (const auto &edge : edges)
            if (!edge.required && edge.source == id && state.values[id] == edge.when)
            {
                auto &blockers = state.blockers[edge.target];
                if (std::find(blockers.begin(), blockers.end(), id) == blockers.end()) blockers.push_back(id);
                const auto previous = forced.find(edge.target);
                if (previous != forced.end() && previous->second.second != edge.value)
                    state.Conflict("conflicting dependency states for: " + registry.Find(edge.target)->GetKey());
                if (previous == forced.end() || id < previous->second.first) forced[edge.target] = {id, edge.value};
            }
    }
    return state;
}
} // namespace era_options
