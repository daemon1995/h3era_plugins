#pragma once

namespace era_options
{
// Root dialog IDs from ERA's DLG_ADVMAP / DLG_BATTLE constants. A modal
// options window changes CurrentDlgId, so only the root identifies a game.
constexpr int AdventureMapDialog = 4205280;
constexpr int BattleDialog = 4662240;
inline bool HasActiveMap(int rootDialog, bool hasMapItems, int mapSize)
{
    return hasMapItems && mapSize > 0 &&
        (rootDialog == AdventureMapDialog || rootDialog == BattleDialog);
}
inline bool AllowDialogEditing(bool requested, bool onMap) { return requested && !onMap; }
inline bool AllowOptionEditing(bool requested, bool onMap, bool changeDuringGame)
{
    return requested && (!onMap || changeDuringGame);
}
bool IsOnMap();
} // namespace era_options
