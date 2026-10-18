# JSON dwelling extender

The built-in `dwellings::DwellingsExtender` supports object types **17** (one creature slot) and **20** (four slots).
It registers with `ObjectExtenderManager` at startup and reads configuration in `AfterLoadingObjectsTxtProc`, after
the merged `objects.txt` prototypes have established the available subtypes. JSON overrides the current native
tables; no additional dwelling TXT configuration is loaded.

The extender owns dwelling tables, native patches, generator creation, RMG valuation, default creature AI values,
names and the creature/growth/town details in the RMG dialog. The host keeps generic object settings and calls
`InitDefaultRmgValues` after reading those settings; name lookup delegates to `GetObjectName`. Generator creation
and dialog hints use the existing extender callbacks, without dwelling-specific branches in the manager.

Put [the example JSON](examples/dwellings.json) in a mod's `lang` directory and restart ERA. It adds subtypes
256, 257 and 258 of each type using an existing dwelling DEF. The DEF and masks must match your graphics.
The example reuses `AVGgrff0.def`; substitute your dwelling's DEF and masks when adding different graphics.

Configure `RMG.objectGeneration.<17 or 20>.<subtype>`:

| Field | Meaning |
| --- | --- |
| `properties` | The usual `objects.txt` lines that add the subtype to the game's object definitions. JSON can discover subtypes 0–1023 for both types; higher subtypes can also come from the existing mod-level `RMG.<mod>.properties` JSON lists. |
| `creatures.0` | Creature ID for type 17; the first slot for type 20. |
| `creatures.1` … `creatures.3` | Remaining slots for type 20. Arrays or objects with numeric keys work through ERA's flattened JSON keys. |
| `name` | Name used by native hover, right-click, visit dialogs and the RMG settings dialog; refreshed on language reload. |
| `frame` | Zero-based icon within this dwelling type's block in `ZLAGPORT.DEF`. |
| `value`, `density`, `map`, `zone` | Existing RMG value, generation density and limits. Supply value/density to create an added generator. |

Numeric values are read with `EraJS::readInt`, including values stored as JSON strings. Creature ID **0** is valid;
**-1** disables a slot. Missing, malformed and out-of-range IDs preserve the previous slot. Added subtypes start
with all slots disabled; standard subtypes inherit the current native/WoG table before applying JSON overrides.
An added dwelling with no valid creatures does not receive an RMG generator. Normal growth, guards, ownership,
weekly updates and recruitment use the game's existing dwelling implementation.

For the same objects in the map editor, build
[EraEditor_ObjectsExtender](../EraEditor_ObjectsExtender/README.md). Its default output folder is `EraEditor`,
and it uses the same JSON example.

## Kingdom overview graphics

Default icon counts are read from the executable. Added subtypes without an available icon reuse frame 0;
`frame` can select another existing icon. The kingdom overview groups dwellings that share a frame, so those groups
also share the icon's hint. For separate icons, supply an extended `ZLAGPORT.DEF` and set string/integer counts at
`RMG.settings.dwellings.frames.17` and `RMG.settings.dwellings.frames.20`. Counts must be positive and no larger than
the corresponding allocated subtype table (at least 1024). Each subtype's `frame` must be below its type's count.

The DEF layout follows the supplied ASM: type 17 icons, type 20 icons, lighthouse, garrison, anti-magic garrison,
abandoned mine, the unused/animation frame, then shipyard. The extender adjusts those special frames when the
dwelling blocks change. Enlarging the counts requires matching DEF graphics. Names and creature tables can grow
without changing the icon counts.

## Native hooks and saves

The table operand addresses and subtype truncation points are based on `Dwellings.asm` and checked against the
local ERA executable. The extender carries the full subtype through map loading, RMG creation and ERM object
placement; native name lookup resolves the subtype from the dwelling's map coordinates. Both added generators
use the game's dwelling vtable (`0x640BC8`). Existing RMG tables are relocated and WoG's native generator
count is preserved; added generators are created from JSON settings. Disabled slots are rejected by the
valuation hook.

Four 32-bit creature IDs per dwelling are written to `RMG_CustomizeObjectProperties.Dwellings` through ERA's
savegame section events and restored after native loading. The records are checked against the dwelling count,
coordinates and current creature capacity before applying the entire section. Missing sections leave older saves
on the native load path. Creature counts, guards and ownership continue to use the native save format.

If `Dwellings.dll` or another dwelling plugin has already replaced the kingdom overview comparison, this
extender quietly leaves that plugin in charge. It does not install overlapping hooks or show an error dialog.
Its own JSON creature/frame overrides are inactive in this case; remove the old DLL to use the built-in
implementation.

## Verification

`Verification/DwellingSetupTests.cpp` tests the actual JSON numeric reader and shared slot/save validation:
missing/invalid fields, ID 0, ID -1, sparse four-slot configurations, subtypes 256/1023 and creature IDs above 255.
Build the plugin as Release/Win32. In-game verification should cover all six example objects on a new map, their
hover/right-click names, guarded visits, recruitment, weekly growth, kingdom overview and save/load. Repeat with
a creature ID above 255 supplied by an installed creature expansion, and with a type 20 dwelling whose first slot
is -1 while another slot is populated.

Run `Verification\run.cmd` from an x86 Visual Studio Developer Command Prompt. It builds and runs the slot/save
checks, including JSON numeric fields, slot/save validation and editor object definitions.
`python Verification\NativeLayout.py "D:\games\HoMM 3 ERA\h3era.exe" "D:\games\HoMM 3 ERA\h3wmaped.exe"` checks
the 15 game instruction spans, six editor patch spans, editor table operands and all six example definitions.
