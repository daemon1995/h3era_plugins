#pragma once

#include "SpellDescriptionLogic.h"
#include "SpellDescriptionDuration.h"
#include <algorithm>

namespace SpellDescriptions
{
namespace ErmBridge
{
constexpr char EVENT_NAME[] = "OnQuerySpellDescription";
constexpr int VERSION = 1;
enum Phase { Classify = 0, BookEffect = 1, Duration = 2 };
enum Context { Book = 0, Battle = 1 };

// Zero-based C++ indices correspond to ERM x1..x16.
enum Argument
{
    ApiVersion, QueryPhase, QueryContext, SpellId, SpellFlags, HeroId,
    CasterId, TargetId, Power, Mastery, SpellKind, Amount, Enabled,
    Side, Hex, Reserved, ArgumentCount
};
using Arguments = std::array<int, ArgumentCount>;

inline bool ValidResult(const Arguments &request, const Arguments &result)
{
    return result[ApiVersion] == VERSION && result[QueryPhase] == request[QueryPhase] &&
           result[SpellKind] >= static_cast<int>(Detail::Kind::None) &&
           result[SpellKind] <= static_cast<int>(Detail::Kind::Duration) &&
           (result[Enabled] == 0 || result[Enabled] == 1) &&
           (request[QueryPhase] != BookEffect || !result[Enabled] || result[Amount] >= 0) &&
           (request[QueryPhase] != Duration || !result[Enabled] ||
            Detail::ValidDuration(result[Reserved], result[Amount]));
}

// ArgXVars and RetXVars are shared ERA buffers, including in nested triggers.
// Copy the result before restoring both buffers; never use ArgXVars as output.
template <class Fire>
bool Dispatch(int (&args)[ArgumentCount], int (&returns)[ArgumentCount], bool &busy,
              const Arguments &request, Arguments &result, Fire fire)
{
    if (busy)
        return false;
    Detail::ScopedArray<int, ArgumentCount> savedArgs(args), savedReturns(returns);
    Detail::ScopedValues<bool, 1> savedBusy({&busy});
    busy = true;
    std::copy(request.begin(), request.end(), args);
    // Also seed the output, so an event without handlers preserves defaults.
    std::copy(request.begin(), request.end(), returns);
    fire();
    std::copy(returns, returns + ArgumentCount, result.begin());
    return ValidResult(request, result);
}
}
}
