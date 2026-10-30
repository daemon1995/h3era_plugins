#pragma once
class PatcherInstance;

namespace erm_functions
{
namespace replacement
{
void InstallHooks(PatcherInstance *patcher);
void ResetMap();
}
}
