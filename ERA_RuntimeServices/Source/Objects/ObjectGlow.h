#pragma once

namespace erm_functions
{
namespace glow
{
// Install the all-or-nothing fog/glow hook group; false keeps native rendering only.
bool InstallHooks();
// Drop map-specific settings and decoded-mask caches at map transitions.
void ResetMap();
}
}
