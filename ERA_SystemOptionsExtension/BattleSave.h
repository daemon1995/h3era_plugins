#pragma once

namespace battleSave
{
// Install once after game/plugin initialization. The setting gates new saves;
// load repair stays active for previously created BATTLE! saves.
void Initialize() noexcept;
bool IsAvailable() noexcept;
} // namespace battleSave
