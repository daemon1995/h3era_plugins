#pragma once

#include "MonolithGroupRegistry.h"

namespace monoliths
{
class ExtendedMonoliths final : public IGamePatch
{
    enum class LoadState
    {
        Idle,
        Rebuild,
        Restore
    };

    GroupRegistry<H3Vector<UINT32>> groups;
    H3Vector<UINT32> emptyGroup;
    std::vector<SavedGroup> loadedGroups;
    LoadState loadState = LoadState::Idle;

    ExtendedMonoliths();
    void CreatePatches() override;
    H3Vector<UINT32> *FindGroup(int subtype) noexcept;
    bool RestoreLoadedGroups();
    void RebuildFromMap();

    static _LHF_(Game_RegisterTwoWayMonolith);
    static _LHF_(Game_GetTwoWayDestinations);
    static _LHF_(Pathfinder_GetTwoWayDestinations);
    static void __stdcall Rmg_CreateZoneConnections(HiHook *hook, H3RmgRandomMapGenerator *rmg);
    static void __stdcall OnSavegameWrite(Era::TEvent *event);
    static void __stdcall OnSavegameRead(Era::TEvent *event);
    static void __stdcall OnAfterLoadGame(Era::TEvent *event);
    static void __stdcall OnGameLeave(Era::TEvent *event);

  public:
    static ExtendedMonoliths &Get();
    void ResetForNewMap();
};
} // namespace monoliths
