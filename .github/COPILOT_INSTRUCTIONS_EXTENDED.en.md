# Copilot Extended Instructions (EN) — h3era_plugins

This document configures GitHub Copilot (Ask / Edits / Chat / Agent) behavior for the Heroes of Might & Magic III ERA plugin monorepo. Goals:
- Provide accurate, contextual API suggestions.
- Leverage the structural map from `headers/H3API_for_copilot`.
- Prevent generation of code that directly includes files under `H3API_for_copilot`.
- Speed up navigation across internal types, managers, adventure map objects.

> WARNING: The directory `headers/H3API_for_copilot` is a REFERENCE INDEX ONLY. Plugin code must include only `framework.h` → `../headers/header.h`. Do NOT suggest adding direct includes to files inside `H3API_for_copilot`.

## 1. Core Rules for Copilot
1. Always propose minimal includes: just `#include "framework.h"` in plugin *.cpp* files.
2. Never duplicate `era.cpp` or internal API sources — rely on the shared solution integration.
3. When generating structure / method samples, clarify that field names may differ; advise verifying in real headers under `headers/`.
4. Offer logging / patching only through existing `Patcher` and `PatcherInstance` objects.
5. If user wants access to a world object, first identify its module (Adventure Map, Managers, RMG, Towns, Skills, Combat, etc.).
6. Do not suggest rolling custom containers instead of `H3Map<T>` or `H3FastMap<T>`.

## 2. New Plugin Skeleton (for auto‑generation)
```
MyNewPlugin/
  MyNewPlugin.vcxproj
  framework.h
  dllmain.cpp
  (optional) FeatureLogic.{h,cpp}
```
`framework.h`:
```cpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include "../headers/header.h"
```
`dllmain.cpp` (minimal):
```cpp
#include "framework.h"
Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;
namespace dllText {
    constexpr char InstanceName[]  = "EraPlugin.MyNewPlugin.Sample";
    constexpr char PluginAuthor[]  = "dev";
    constexpr char PluginVersion[] = "1.0.0";
    constexpr char PluginDate[]    = __DATE__;
}
void StartPlugin() {
    globalPatcher = GetPatcher();
    _PI = globalPatcher->CreateInstance(dllText::InstanceName);
    Era::ConnectEra(GetModuleHandleA(nullptr), dllText::InstanceName);
    // TODO: hooks
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        StartPlugin();
    return TRUE;
}
```

## 3. API Module Map & Purpose
Extended classification of files in `headers/H3API_for_copilot`. Copilot should anchor suggestions to these domains.

### 3.1 Adventure Map (object layer)
Each file models a concrete map object or auxiliary metadata. Usage: scanning, modifying, spawn logic, interaction triggers.
Files (alphabetical, duplicates arise from index passes):
`H3GlobalObjectSettings.hpp`, `H3MapArtifact.hpp`, `H3MapBlackMarket.hpp`, `H3MapBoat.hpp`, `H3MapCampfire.hpp`, `H3MapCorpse.hpp`, `H3MapCreatureBank.hpp`, `H3MapEvent.hpp`, `H3MapFlotsam.hpp`, `H3MapFountainFortune.hpp`, `H3MapGarrison.hpp`, `H3MapGenerator.hpp`, `H3MapHero.hpp`, `H3MapItems.hpp`, `H3MapLeanTo.hpp`, `H3MapLearningStone.hpp`, `H3MapLighthouse.hpp`, `H3MapMagicShrine.hpp`, `H3MapMagicSpring.hpp`, `H3MapMine.hpp`, `H3MapMonolith.hpp`, `H3MapMonster.hpp`, `H3MapMysticGarden.hpp`, `H3MapObelisk.hpp`, `H3MapOceanBottle.hpp`, `H3MapPandorasBox.hpp`, `H3MapPrison.hpp`, `H3MapPyramid.hpp`, `H3MapQuestGuard.hpp`, `H3MapRefugeeCamp.hpp`, `H3MapResource.hpp`, `H3MapScholar.hpp`, `H3MapScroll.hpp`, `H3MapSeaChest.hpp`, `H3MapSeerHut.hpp`, `H3MapShipwreckSurvivor.hpp`, `H3MapShipyard.hpp`, `H3MapSign.hpp`, `H3MapTown.hpp`, `H3MapTreasureChest.hpp`, `H3MapTreeOfKnowledge.hpp`, `H3MapUniversity.hpp`, `H3MapWagon.hpp`, `H3MapWarriorsTomb.hpp`, `H3MapWaterMill.hpp`, `H3MapWindmill.hpp`, `H3MapWitchHut.hpp`, `H3ObjectAttributes.hpp`, `H3ObjectDetails.hpp`, `H3TileVision.hpp`.
Focus groups:
- Resource / reward: Campfire, TreasureChest, Resource, SeaChest.
- Training / development: University, TreeOfKnowledge, LearningStone, WitchHut.
- Combat sites / guarded: CreatureBank, Monster, WarriorsTomb, Pyramid.
- Special services: BlackMarket, SeerHut, MagicShrine, Shipyard, Lighthouse.
- Movement / interaction: Boat, Monolith, Obelisk (quests / navigation), QuestGuard.

### 3.2 RMG (Random Map Generator)
Files: `H3RmgTemplate.hpp`, `H3RmgRandomMapGenerator.hpp`, `H3RmgParameters.hpp`, `H3RmgObjectTown.hpp`, `H3RmgObjectSeer.hpp`, `H3RmgObjectProperties.hpp`, `H3RmgZone.hpp`, `H3RmgObjectMonster.hpp`, `H3RmgObject.hpp`, `H3RmgMovementCost.hpp`, `H3RmgMapItem.hpp`, `H3RmgMapInfo.hpp`, `H3RmgMap.hpp`, `H3RmgLocalMap.hpp`, `H3RmgGroundTileData.hpp`, `H3RmgGroundTile.hpp`, `H3RmgZoneGenerator.hpp`, `H3RmgZoneData.hpp`, `H3RmgZoneConnection.hpp`.
Use cases:
- Zone connectivity tuning.
- Movement cost alterations (MovementCost / GroundTile).
- Template parameter overrides.
- Object generation customization (Object*, ObjectProperties).

### 3.3 Skills (Primary & Secondary)
Files: `H3PrimarySkills.hpp`, `H3SecondarySkill.hpp`, `H3Diplomacy.hpp`.
Use: stat calculation modifications, neutral creature join logic, dynamic secondary skill adjustments.

### 3.4 Towns (economy & events)
Files: `H3Town.hpp`, `H3SetupTown.hpp`, `H3TownCreatureTypes.hpp`, `H3TownDependencies.hpp`, `H3BuildingCosts.hpp`, `H3CastleEvent.hpp`, `H3OraclePuzzle.hpp`.
Use: economy tuning, build dependencies, creature lineups, town events, puzzle map logic, initialization.

### 3.5 Artifacts
Aggregate: `H3Artifacts.hpp` → `H3Artifact.hpp`, `H3ArtifactSetup.hpp`, `H3CombinationArtifacts.hpp`.
Use: artifact definition, combination bonus evaluation.

### 3.6 Game Data
Aggregate: `H3GameData.hpp` global scenario/session components.
Core types: `H3Main`, `H3MainSetup`, `H3MapInfo`, `H3ScenarioMapInformation`, `H3Resources`, `H3Spell`, `H3GlobalEvent`, `H3LossCondition`, `H3VictoryCondition`, `H3TurnTimer`, `H3GarrisonInterface`, `H3RandomDwelling`, `H3Network`.
Use: scenario rules, economy metrics, timing, network state.

### 3.7 Managers
`H3Managers.hpp`: `H3AdventureManager`, `H3CombatManager`, `H3ExecutiveMgr`, `H3HiScoreManager`, `H3InputManager`, `H3MouseManager`, `H3RecruitManager`, `H3SoundManager`, `H3SwapManager`, `H3TownManager`, `H3WindowManager`.
Use: lifecycle and UI hooks, combat, recruiting.

### 3.8 Base / Containers / Core
- Version macros (`H3Version.hpp`).
- Containers: `H3Map<T>`, `H3FastMap<T>` (in `H3Base/H3Core.hpp`).
- Allocators: `H3AllocatorReference` (`H3Containers/H3Vector.hpp`).
Use: efficient game-aligned storage.

### 3.9 Dialogs & UI
Representative: `H3DlgHintBar.hpp`, `H3DlgHighlightable.hpp` + aggregated `H3Dialogs.hpp`.
Use: interface extension, highlighting, tooltips.

### 3.10 Utilities
`H3Utilities.hpp`: helper utilities (string ops, logging, resource helpers). Prefer these over bespoke duplicates.

### 3.11 Version
`H3Version.hpp` macros:
```
_H3API_VERSION_MAJOR_
_H3API_VERSION_MINOR_
_H3API_VERSION_BUILD_
_H3API_VERSION_
_H3API_VERSION_TEXT_
```
Use: compatibility checks / warnings.

### 3.12 Combat & Spells
Combat layer + spell representation and AI simulation.
Files (Combat): `H3CombatCreature.hpp`, `H3CombatSquare.hpp`, `H3AdjacentSquares.hpp`, `H3Obstacle.hpp`, `H3ObstacleInfo.hpp`, `H3WallSection.hpp`, `H3ValidCatapultTargets.hpp`, `H3MagicAnimation.hpp`.
Files (AI / Spell Simulation): `H3SimulatedCombat.hpp`, `H3SimulatedCreature.hpp`, `H3SimulatedHero.hpp`, `H3SimulatedSpell.hpp`, `H3SpellCastInfo.hpp`, `H3SpellData.hpp`.
Use cases:
- Inspect / modify stack state pre/post attack.
- Influence AI target choice (via simulated structures).
- Customize siege walls / obstacles creation & durability.
- Track or suppress spell visual effects.
Guidelines:
1. Access creatures through `H3CombatManager` → creature collections.
2. Validate target cells using `H3CombatSquare` / `H3AdjacentSquares`.
3. For spells: fetch definition via `H3Spell` (Game Data) + context `H3SpellCastInfo`.
4. Never free obstacle memory manually; rely on engine transitions.
5. Prefer simulated AI structures instead of reimplementing prediction logic.
Anti‑patterns:
- Creating raw stack objects without engine routines.
- Deleting obstacles with `delete`.
- Silent mutation of spell data (log changes for debugging).

## 4. Autocomplete Pattern Table
| Intent | Recommended Symbols / Managers |
|--------|--------------------------------|
| Hero manipulation | `H3Hero`, `H3MapHero`, `H3PrimarySkills`, `H3SecondarySkill` |
| Combat context | `H3CombatManager`, battle stacks, `H3Spell` |
| Map objects | Any `H3Map*` + `H3TileVision` |
| Random map gen | `H3RmgRandomMapGenerator`, `H3RmgZoneGenerator`, `H3RmgTemplate` |
| Economy/resources | `H3Resources`, town modules |
| UI/dialogs | `H3WindowManager`, `H3Dlg*` |
| Artifacts | `H3Artifact`, `H3CombinationArtifacts` |

## 5. Anti‑Patterns (Disallow)
| Anti‑pattern | Reason | Suggest Instead |
|-------------|--------|-----------------|
| Direct include from `H3API_for_copilot` | Reference-only index | Use `framework.h` |
| Raw JMP patch without Patcher | Crash / incompat risk | Use `globalPatcher` / `_PI` |
| Excess global variables | Namespace pollution | Encapsulate in functions / namespaces |
| Memory patch w/o version guard | Breaks compatibility | Check `_H3API_VERSION_` |
| Homemade copies of internal structs | Drift on updates | Use provided API types |

## 6. Example Answers
### Example: Modify player gold
```cpp
H3Main* main = *P_Main;
H3Player* player = main->GetPlayer(P_CurrentPlayerID);
if (player)
    player->resources.gold += 1000; // add 1k gold
```
### Example: Get current combat attacker
```cpp
if (auto cm = H3CombatManager::Get()) {
    H3Hero* attacker = cm->hero[0];
    if (attacker) {
        // skill logic
    }
}
```
### Example: Nearby map objects scan
Mention: use `H3MapHero` position + iterate map tiles with `H3TileVision`.

## 7. Pre‑Answer Checklist
- Only `framework.h` includes? (Yes)
- No direct `H3API_for_copilot` paths? (Must be none)
- Correct module classification? (e.g., combat → Combat & Spells)
- Field name uncertainty noted where deep internals used?

## 8. Glossary
| Symbol | Meaning |
|--------|---------|
| `H3Main` | Global scenario / session state |
| `H3Hero` | Hero entity (skills, inventory) |
| `H3Town` | Town state (buildings, garrison) |
| `H3CombatManager` | Active battle context |
| `H3RmgRandomMapGenerator` | Random map generator core |
| `H3Artifact` | Artifact definition |
| `H3Resources` | Player resources container |
| `H3WindowManager` | Dialog/window management |
| `H3Dlg*` | UI controls |
| `H3Map*` | Adventure map objects |

## 9. Additional Copilot Guidance
- When unsure: prompt user to verify structure fields in real headers.
- Use patcher APIs; avoid raw assembly suggestions.
- Keep code minimal, annotate risky spots.

## 10. Reminder
`headers/H3API_for_copilot` must NOT be included in plugin sources. Any suggestion violating this should be corrected immediately.

---
Keep this document updated when adding new API domains.
