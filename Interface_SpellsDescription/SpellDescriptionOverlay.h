#pragma once

#include "framework.h"
#include "SpellDescriptionOverlayState.h"

namespace SpellDescriptions
{
namespace Overlay
{
void Install(PatcherInstance *pi);
void Update(H3CombatManager *combat, int spellId, const Detail::ChainRoute &route);
void Clear(bool redraw = true);
} // namespace Overlay
} // namespace SpellDescriptions
