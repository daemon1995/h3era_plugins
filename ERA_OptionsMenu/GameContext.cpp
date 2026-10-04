#include "pch.h"
#include "GameContext.h"

namespace era_options
{
bool IsOnMap()
{
    Era::TGameState state = {};
    Era::GetGameState(&state);
    if (state.RootDlgId != AdventureMapDialog && state.RootDlgId != BattleDialog) return false;
    const auto *game = H3Main::Get();
    return game && HasActiveMap(state.RootDlgId, game->mainSetup.mapitems != nullptr, game->mainSetup.mapSize);
}
} // namespace era_options
