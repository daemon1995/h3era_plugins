#pragma once

#include "framework.h"

namespace SpellDescriptions
{
// Publish the compatibility API at DLL initialization, before other plugins use it.
void PublishApi();
// Called once after ERA and the game resources have been initialized.
void Install(PatcherInstance *pi);
}
