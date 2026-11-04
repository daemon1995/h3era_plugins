#pragma once
// Core includes are the supplied modern H3API and ERA headers, not igrik's homm3.h.
#include "../../headers/header.h"
#include <cstdint>
#include <climits>
#include <algorithm>
#include <string>

namespace gbf {
// Unqualified legacy helpers resolve to modern h3:: declarations.
using namespace h3;
extern Patcher* patcher;
extern PatcherInstance* instance;
void InstallBen(PatcherInstance* pi);
void InstallScouting(PatcherInstance* pi);
void InstallHeroMovement(PatcherInstance* pi);
void InstallRuntimePatches(PatcherInstance* pi);
void InstallAI(PatcherInstance* pi);
void InstallSpells(PatcherInstance* pi);
void InstallSummon(PatcherInstance* pi);
void InstallMonsters(PatcherInstance* pi);
void InstallLuck(PatcherInstance* pi);
void InstallNpc(PatcherInstance* pi);
void InstallGraphics(PatcherInstance* pi);
void InstallAudio(PatcherInstance* pi);
void InstallCrExpo(PatcherInstance* pi);
void InstallPrimarySkills(PatcherInstance* pi);
void InstallGameLogicCore(PatcherInstance* pi);
namespace game_logic_remaining { void Install(PatcherInstance* pi); }

}
