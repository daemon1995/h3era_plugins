#pragma once
class PatcherInstance;

namespace erm_functions
{
namespace replacement
{
// Install geometry-selection hooks; an unsupported executable disables this API.
void InstallHooks(PatcherInstance *patcher);
// Invalidate map-bound shape caches and replacement work on map transitions.
void ResetMap();
}
}
