# Dwelling information in RMG

Creature dwelling gameplay is implemented by the standalone
[ERA_DwellingsExtender](../ERA_DwellingsExtender/README.md) plugin. Its creature/name tables, native hooks,
save support and runtime kingdom portraits are no longer compiled into RMG.

RMG retains `DwellingInfo.cpp`, a read-only adapter used directly by the settings dialog and name/default-value
resolution. It does not register `17/-1` or `20/-1` as ObjectExtender identities. Displaying dwelling information
therefore adds no visit, new-map, weekly or AI callbacks to the object dispatcher.

The adapter first queries the active standalone plugin, then falls back to the installed game table pointers
and their published capacities. A foreign plugin's relocated tables are read live; JSON/prototype subtype limits
never increase those read bounds. A plugin using an entirely different table representation needs its own adapter.
If an explicit object extender supplies a custom dialog hint, RMG uses it first; otherwise it appends its dwelling
creature/town/growth information. For generation, an explicit gameplay owner has priority, and the native dwelling
factory handles supported dwellings without an owner.

The compatibility `GetDwellingExportInfo` export forwards read-only data and preserves old record sizes.
`AppendDwellingRmgHint(type, subtype, H3String*)` and
`CreateDwellingRmgGenerator(type, subtype, value, density)` are explicit stdcall exports for UI/factory integration;
they do not register ownership. Strings and generator allocations use the game runtime.

Existing JSON names, descriptions, creature configuration and graphics examples remain under
`RMG.objectGeneration.<17 or 20>.<subtype>`. See [the example](examples/dwellings.json). RMG still merges `properties`
and supplies the added random-map generators; the standalone plugin provides their extended runtime gameplay.

Checks:

- `Verification/run.cmd`: slot/save/editor validation and bounded live data views.
- `Verification/run-object-dispatch.cmd`: exact/wildcard ownership and counter regression checks.
- `Verification/NativeLayout.py`: original game/editor patch spans and sample JSON.
- `../ERA_DwellingsExtender/Verification/VerifyDwellingPortraits.ps1`: Combined, Standalone and Foreign game runs.
