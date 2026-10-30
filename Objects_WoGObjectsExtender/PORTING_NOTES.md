# Native WoG adventure objects

The plugin implements the 23 custom-pyramid identities found by auditing active object-visit triggers in WoG Scripts. Type **141** is used for RMG generation; authored legacy **63/subtype** objects use the same C++ handlers by default. The native Pyramid **63/0** is excluded.

## Coverage

| Subtype (63 and 141) | Object |
|---:|---|
| 2 | Фонтан Венеры |
| 3 | Загадочное жилище существ |
| 8 | Торговец хламом |
| 9 | Башня тайн |
| 14 | Волшебный гриб Воздуха |
| 15 | Камера Смерти |
| 18 | Волшебный гриб Воды |
| 20 | Алтарь трансформации |
| 21 | Камни призыва |
| 22 | Забытое святилище |
| 23 | Волшебный гриб Земли |
| 24 | Дом милостыни |
| 29 | Волшебный гриб Огня |
| 32 | Боевая академия |
| 33 | Дворец грёз |
| 48 | Фонтан берсерка |
| 50 | Живой череп |
| 51 | Рынок времени |
| 52 | Ремесленник |
| 57 | Пещера приключений |
| 62 | Школа волшебства |
| 65 | Изумрудная башня |
| 66 | Зеркало пути домой |

The audit resolves numeric constants and reads both pre-visit and post-visit triggers, including the auxiliary Death Chamber / Adventure Cave triggers in script 21. `Verification/audit-wog-scripts.py` reports the source files and line numbers. Source references for the added potion, summoning, wizardry and cave ports are stored under `erm/reference/`.

Sphinxes (63/10..13), Power Stones (101/7..10), new creature banks (16) and creature dwellings (17) use built-in WoG or existing specialized extenders; they do not have custom-pyramid visit implementations in the audited ERM scripts. Their implementations remain owned by those components.

## Registration and RMG

Every ported identity has a value, density and explicit native prototype in the base JSON; both locales provide names. Locale overrides inherit any omitted fields from the base data. Prototype DEFs, footprints, entrances and terrain masks come from WoG's `zobjcts.txt`. This removes the dependency on loading WoG's legacy object table before the extender manager. Runtime cloning still admits additional installed legacy graphic variants. The API registration declares the complete identity set and the WoG dialog page; generation returns generators only for type 141.

The additive `RegisterObjectVisitObserver` C export in `ObjectExtenderAPI.hpp` supplements visits without changing the extender vtable or replacing an object's primary handler. It serves ordinary Fountain of Fortune potion refills, Adventure Cave cargo sales, Obelisk quests and the wealth lesson's chest bonus. The ordinary fountain retains its standard visit and AI behavior.

## State and engine integration

Adventure Cave stores its four seven-bit chambers, branch count, entrance-cleared flag and initialization marker directly in map-item setup. Only Arcane Tower, Altar and Emerald Tower reserve bits [5, 13) for the player visitor mask. Native calls at 0x4FC620 and 0x529B70 write and read those period-limited visits. Visit handlers receive an independent object snapshot: modal redraws and Space revisits can change the canonical tile into a hero marker. Updates reach the tile or the hero's underlying-object setup without changing the hero ID. These C++ objects have not been released; there is no migration of prior C++ setup formats. The branching layout and coordinate-dependent signs are retained.

Hero and player state is stored exclusively in namespaced global ERA associative variables: potion doses, Cave services/lessons/cargo, buffs, visits and pending combat effects. IntValue is a write-through handle, so an existing handle sees newly loaded ERA storage rather than cached values. There are no reads, writes or migration of w variables. Old w progress may be reset, as explicitly requested. API visit/AI positions are scoped explicitly because the engine can pass a temporary map-item copy.

Custom dialog selections process LBUTTON_DOWN. OK/Cancel controls use CreateOKButton/CreateCancelButton
at the same coordinates, raised 18 pixels from their previous positions;
artifact descriptions use the RMC popup. Living Skull searches artifacts or scrolls, filters by name,
and excludes artifacts 144 and 145. English and Russian JSON belongs to Mods/WoG Scripts/lang
and lang/ru. Skull, Altar and Arcane texts share the script namespaces wog.12, wog.14 and wog.104;
new dialog labels live in each script's native.ui section. Skull opens the paid search after its free
reveal/transfer offer; scroll tabs have distinct control IDs and clear the artifact-name filter.
Junk Merchant previews rewards in a custom artifact catalogue and refreshes after each exchange;
multiple copies of an artifact or scroll can be exchanged during the same visit. Creature portraits
use frame creatureId + 2, including the large reward and dwelling portraits, as in ERA_HelpDialog.
Skull tabs route presses through text overlays because native DEF buttons report release clicks.
The catalogue stays open while one service subdialog advances through search, direction, reveal and
transport. Every reveal returns the camera to the hero on leaving that service, including cancellation.
Selections use the MsgBox 300 ms same-item double-press rule and its native palette color
(H3GamePalette entry 45, read by ShowParsedDlg8Items at 0x4F798B).
Arcane slider positions 0..N-1 represent 1..N exchanges; native page steps are explicitly nonzero.
Palace of Dreams displays all three reels and the result of every spin, including losses, and checks
the entry gold requirement before charging movement. Their texts use wog.108 and wog.11 respectively.
Its reels, rerolls, AI choice and prizes use SplitMix32 with a saved object/player/hero seed in assoc.
Global RNG activity cannot change a replay after loading; canceling game selection consumes no draws.
ERM initialization during a load does not replace the saved map seed. Skull's catalogue and service
subdialog both use width 504.

Human Summoning Stones show a scrollable list of town recruits and garrison transfers. Selecting a
recruit invokes the native town-dwelling H3RecruitMgr constructor at 0x551960, with its stock pointers
and the visiting hero's army. The native manager chooses quantity and charges resources. A successful
purchase marks the stone used that day; cancel leaves the list and daily service available. Full-stock
affordability is only an AI filter; humans can hire part of a town's available population.

Mysterious Dwelling checks gold before showing the offer, merges generated creatures by species, and
validates army capacity only after acceptance. Its SplitMix32 stream combines a saved per-map random
seed with hero ID, object coordinates and day; repeated previews do not advance the game's RNG.
New-map/restart initialization explicitly replaces the saved seed; loading a save retains it.
At hero levels 1-2 during the initial weeks the original rules allow only the base tier-1 creature,
so changing the seed cannot change that offer. The dwelling window fits the number of merged species.

Primary skills and spells use their native structures; secondary training uses the engine method, and native experience invokes normal level-up processing. Warfare removal updates all three linked skills and compacts visible positions without reading w48. Mithril resolves the active eight-integer engine array through the operand at 0x073764C + 3 (original array 0x27F9A00). Native ArtDisabled at 0x714EA7 preserves ERA_ArtifactsExtender hooks and map bans. Army transfers merge stack experience and stack artifacts using CrExpoSet::HComb; untrained reinforcements dilute existing stack experience. Altar species changes match HE:C0 mode 0: reset experience while retaining stack artifacts.

Numeric obstacle layouts, battlefield occupancy/action queries, spell casting, tactics flags, stack movement/counts, map visibility, mine ownership and object removal now use C++ structures or native engine methods. No ERM remains in Market of Time, Artificer, Basic, Trading, Learning or Skull handlers.

The small remaining ERM bridge is isolated in WoGObjectUtils.cpp. Its callers only use private WoG services: pre-battle hero/network context (BA:H/E), background and random-layout gates (BA:B/BF:C), targeted mine/quicksand placement (BM:Q). The supplied sources show these depend on WoG-private state or temporary spell patches. Their known receiver entry points preserve this behavior without guessing undocumented storage addresses. They do not hold this plugin's persistent state.

The five newly audited identities are Venus Fountain (2), Summoning Stones (21), Berserker Fountain (48), Adventure Cave (57), and School of Wizardry (62). They include potion consumption/restoration, full town stock or garrison transfers with stack experience, daily training/spells/scroll copying, and Cave chambers, Rogue services, Hermit lessons, battlefield layouts and continuing combat effects.

## Heroes-style dialogs

Artificer uses native DEF buttons and their pressed frames/hotkeys, the standard textured background, native borders, Hero fonts and a hint bar. The examples studied are RMG settings, ERA Help, System Options and Enhanced Creature Information. Bounded background tiling follows Help's HD-renderer workaround. The header renders the visited object's actual DEF, and artifact names/descriptions come from the runtime artifact API.

Single and bulk backpack sales use the existing revalidated transaction core. Bulk sale confirmation lists the chosen Gold/Mithril proceeds. Equipped items are excluded, and sales consume no movement or daily orders. `Verification/preview-artificer.py` produces an offline layout preview using installed DEFs; it is not an in-game screenshot.

Potion choices use one native checkbox dialog, including availability, individual effects and the permanent Spell Power risk when mixing drinks. Academy offers one menu with prices and dynamic names. Dwelling/Summoning purchase previews are combined with their confirmation; Market introduction and repeated School success boxes are omitted. Re-visiting a full potion fountain does not show another refill notification.

## AI

AI weights use current availability and applicable visit periods. Visits recheck affordability, army capacity and useful rewards. Dwelling offers are stable between evaluation and purchase. Altar conversion avoids downgrading AI stacks. Academy and Wizardry choose useful legal training; Junk Merchant targets only artifacts it can actually trade; gambling is limited to one visit per hero/day with a reserve. Emerald chooses a legal species and useful upgrade. Death Chamber and Cave combat require an army safety margin, and no rewards are awarded on retreat/defeat. Cleared Death Chambers honor the optional 28-day refill rule and weekly growth.

Living Skull and Mirror deliberately keep zero AI attraction, matching the source's loop-avoidance behavior. They still have complete human handlers. The original free AI primary training and Emerald upgrades are retained.

## Installation and validation

Build Release/Win32 for **both** Objects_WoGObjectsExtender and RMG_CustomizeObjectProperties. Run `Install-WoGObjects.ps1` with a Python 3.10+ executable and the desired GameRoot. `-CheckOnly` stages and validates without changing the game. `Install-Artificer.ps1` forwards to the full installer.

Installation merges all 23 object settings and owned translations, installs both DLLs and symbols, and adds per-map assoc guards to legacy ERM entry points and callbacks. It suppresses the ported objects' WoGify replacement entries while retaining Titan's Winter's 140/0 Market and unrelated neutral-creature logic. The installer backs up every existing target, checks copied hashes, and rolls back on failure. Close ERA before installation; reload embedded ERM scripts when continuing an older save.

Verified on Windows with MSVC v141_xp, Release/Win32:

- Full DLL builds and exported observer API.
- Artificer native adapter, transactions, bulk sale and 23-object registration/generator routing.
- Artificer dialog interaction checks, 259 potion dialog interactions and Market native/Warfare regressions.
- 186 actual assoc/setup checks: loaded-store replacement, defaults, hero/object isolation and nested API positions.
- Exhaustive Cave control-word/chamber tests and extracted battlefield/spell-price tables.
- Actual manager dispatch/counter bodies, including declared subtypes absent from graphics and values above 255.
- Full installation into a disposable fixture: JSON preservation, ERM guards, prototypes, backups, idempotence and rollback with a locked target.

Live gameplay and every combat/dialog combination still require testing in ERA; these automated checks do not execute the game engine.

## Map policy and object control words

`wog_objects.native.plugin_handles.<type>.<subtype>` is a raw ERA assoc integer shared by C++ and ERM:
1 selects C++; 0 delegates visits, hints and AI without changing their output arguments. Both 63 and 141
are initialized to 1 for all 23 supported identities. Defaults run after ERA ResetMemory and before map
instructions. Saved settings are preserved on load, including 0 entries omitted by ERA serialization;
the nonzero `.version` marker distinguishes a configured map from a save predating this policy.
No process-wide registry or static ERM variable supplies native ownership. Late edits are read live.
Built-in Fountain of Fortune's potion observer has its own 30/0 key; its engine handler is unaffected.

Legacy script guards use these raw assoc values without touching v1. Shared mushroom/potion callbacks
run if any associated legacy identity still uses ERM; each OB entry and WoGify replacement entry has its
own type/subtype condition. Titan's Winter 140/0 retains its separate Market helpers. The installer
removes only this plugin's previous registry guards and keeps unrelated script edits.

Arcane Tower, Altar and Emerald Tower pack a shared 18-bit period and eight-player mask into setup, with
an initialization bit. Summoning Stones store the last use day; Living Skull stores its free-search week.
Death Chamber packs its base difficulty and clear day. Artificer packs its service day and order counters;
Cave stores its packed chamber word directly in setup. These layouts have no player visitor mask.
Writes update temporary visit items, canonical map tiles and hero-under-object storage without replacing
the hero tile's setup. Legacy 63 setup is untouched in the engine's early initialization pass because map
instructions have not chosen ownership yet; native setup is cleared after instructions choose ownership. Per-hero progress and
larger object records remain in namespaced assoc. Coordinate-based assoc keys now include object type.

There are no C++ reads/writes of fixed v, w, z or f arrays. Private BA:H/E queries return through an assoc
scratch integer with nested-call preservation; the other remaining receivers are private battlefield gates.
Loading is tracked through ERA lifecycle events. Cave journal notes are per-hero assoc records, rendered
as one contextual message at WoG's documented MakeQuestLog call site (0x4DD82F); no QW global z slots or
legacy PO import remain. The original quest log continues after these notes. Test the hook in ERA.
