# WoG objects -> native ERA object handlers

This project ports the supplied WoG object scripts to the existing `Objects_WoGObjectsExtender` plugin architecture.

## Object identity

New RMG objects use ERA object type **141** (`eObject::BLANK3`) and preserve the original WoG subtype. Legacy maps using **63/subtype** are still dispatched to the same native C++ handlers when the corresponding WoG option is enabled.

| subtype | object | period/state |
|---:|---|---|
| 3 | Mysterious Creature Dwelling | once per hero |
| 8 | Junk Merchant | per visit |
| 9 | Arcane Tower | weekly per player/object |
| 14 | Magic Air Mushroom | weekly per hero |
| 15 | Death Chamber | once per player/object |
| 18 | Magic Water Mushroom | weekly per hero |
| 20 | Altar of Transformation | weekly per player/object |
| 22 | Forgotten Shrine | one pending blessing per hero |
| 23 | Magic Earth Mushroom | weekly per hero |
| 24 | Alms House | once per hero |
| 29 | Magic Fire Mushroom | weekly per hero |
| 32 | Battle Academy | daily per hero |
| 33 | Palace of Dreams | per visit |
| 50 | Living Skull | daily searches + weekly free search |
| 65 | Emerald Tower | monthly per player/object |
| 66 | Mirror of the Home-Way | per visit, human only |

Existing native handlers remain unchanged for 141/51 Market of Time and 141/52 Artificer.

**Commander Sanctuary is intentionally not ported or modified.** Type 80 is not registered by the universal handler.

## Universal handler

`WoGObjectHandlers.cpp` is the common dispatcher for visit, right-click hint and AI weight. `WoGObjectIds.cpp` is the descriptor registry. State that cannot safely fit into `H3MapItem::setup` is kept in namespaced ERA associative variables and keyed by object coordinates / hero / player.

The `objects.txt` hook clones every installed legacy 63/subtype property row for a ported subtype to 141/subtype. This preserves actual DEF names, masks, entrances and terrain compatibility from the user's installed WoG/data pack rather than hardcoding one graphics set.

## AI changes

AI uses the same state and gameplay calculations as human visits. Deliberate improvements over old ERM behavior:

- Mysterious Dwelling: AI evaluates the same generated offer instead of receiving a separate random WoG creature.
- Battle Academy: AI evaluates primary, daily secondary and daily artifact choices instead of a random branch; source-free primary training remains free for AI.
- Death Chamber: AI visits only when its army value has a safety margin over estimated danger.
- Palace of Dreams: AI keeps a gold reserve and caps gambling exposure per visit.
- Emerald Tower: AI enhances its highest-total-AI-value creature and excludes the Undead/Living special conversions exactly as the ERM comments intended.
- Living Skull and Mirror: AI weight remains zero to avoid the loop/pathfinding issues documented by the original scripts.

## Source bug fixes / defined behavior

- Altar chance is based on **actual mana invested**, not the displayed percentage label. The 0-mana branch, which leaves `townType` undefined in ERM, is explicitly defined as fully random. Cancelling town selection no longer loses mana.
- WoG creature IDs use named constants: Hell Baron 153, Blood Dragon 154, Darkness Dragon 155, Ghost 159, Emissaries 160..163, Gorynych 168, Arctic/Lava Sharpshooter 170/171, Sorceress 193.
- Living Skull consumes its weekly free search only if an artifact was actually found.
- Living Skull's “bring” action removes the old map object through WoG's public `UN:O` receiver and then uses native `H3Main::PlaceObjectOnMap`, mirroring the safe object-replacement approach already present in H3EraPlugins.
- Emerald Tower snapshots original creature data and replays persisted enhancements on load. Repeated percentage upgrades compound from the current value exactly as the ERM formulas do, avoiding irreversible 100 -> 120 -> 96 restoration errors.
- Emerald spell-charge Mithril cost uses the source clamp ranges (2..5 / 5..10 / 10..15 / 15..20).
- Emerald Fly excludes teleporters (Devil, Arch Devil, Hell Baron); Attack All Adjacent excludes Vampire Lord, Ghost and Blood Dragon.
- Death Chamber Fire Shield / Counterstrike power is clamped from the actual difficulty (1..20), not `difficulty / 20`.
- Right-click hints for legacy 63/subtype objects resolve the active human hero, so per-hero/per-player availability is shown correctly.

## Dynamic game data

Creature, artifact, hero and spell validation uses ERA/H3API runtime counters (`H3CreatureCount`, `H3ArtifactCount`, `H3HeroCount`, `H3SpellCount`) rather than fixed SoD list sizes. Fixed IDs occur only where the original WoG rule targets a specific named entity.

## Validation

Run:

```text
python Verification/verify-native-wog-objects.py
```

The checker validates the descriptor list, type 141 JSON entries, project source registration, Sanctuary exclusion, named WoG IDs and localization JSON syntax.

A Windows/Visual Studio build is still required for binary validation because the plugin depends on Win32/MSVC/H3API headers and x86 hooks.
