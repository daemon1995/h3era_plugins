#pragma once

#include <windows.h>

namespace assemblyModList
{
// Updates mods\list.txt once and returns TRUE if at least one mod was renamed.
BOOL Get();
} // namespace assemblyModList
