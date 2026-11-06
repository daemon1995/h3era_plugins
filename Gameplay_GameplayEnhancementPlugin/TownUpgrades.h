#pragma once

namespace townupgrades
{
void Init();
// Returns the number of upgraded stacks; -1 means invalid town/mask or unavailable feature.
int UpgradeTown(int townId, int armyMask, bool upgradeSkeletons);
// townId -1 = current town; slot -1 = army, 0..6 = one stack in mask 1 or 2.
int UpgradeTownSelection(int townId, int armyMask, int slot, bool confirm);
} // namespace townupgrades
