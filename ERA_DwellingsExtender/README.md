# ERA_DwellingsExtender

Standalone ERA/Win32 plugin for creature dwellings **17** (one creature slot) and **20** (four slots).
It owns the creature/name tables, full-width subtype handling, recruitment valuation, save records and runtime
kingdom-overview portraits. It works without `RMG_CustomizeObjectProperties.era`.

The class does not inherit `ObjectExtender`, register object types/subtypes, or receive the RMG dispatcher's visit,
new-map, weekly or AI callbacks. Removing the plugin leaves the RMG dwelling information UI available.

## Installation

The Visual Studio solution includes `ERA_DwellingsExtender.vcxproj`. Build **Release / Win32**.
`Install-WoG.ps1 -Build` builds and installs this plugin, ERA_RuntimeServices, the updated RMG plugin and locale manager into
`Mods/WoG/EraPlugins`, retaining the previous files under this project's `build/installed-backups/`.
Without `-Build`, the installer uses the existing local `build/` outputs. Close the game before installation.

The old RMG build contained the same gameplay hooks. Update RMG together with the standalone plugin.
Existing JSON files and saves remain compatible. The save section intentionally retains its old name
`RMG_CustomizeObjectProperties.Dwellings` and version 1.

## JSON

Place JSON in a mod's `lang/` directory and restart ERA. The existing namespace is retained:
`RMG.objectGeneration.<17 or 20>.<subtype>`. See [examples/dwellings.json](examples/dwellings.json).

| Field | Meaning |
| --- | --- |
| `creatures.0` | Creature ID for type 17; first creature for type 20. |
| `creatures.1` … `creatures.3` | Remaining type 20 slots. `-1` disables a slot, ID `0` is valid. |
| `name` | Dwelling name; refreshed when language data reloads. |
| `frame` | Source frame within the type's original combined DEF block. New subtypes default to 0. |
| `image` | Optional native game PCX or separate DEF resource, overriding `frame`. |
| `imageFrame` | Frame in group 0 of a separate DEF; defaults to 0. |
| `properties` | Map graphics/prototypes, merged by RMG or the editor plugin. |
| `value`, `density`, `map`, `zone`, `description` | RMG's existing generation settings and description. |

Numeric strings and JSON numbers are accepted. Missing or invalid creature values preserve the previous slot.
Object subtypes use the shared signed-16-bit range 0–32767; higher IDs are rejected by the game/editor definition loaders.
New subtypes start with disabled slots. Each table contains `max(originalCount, maxLoadedSubtype + 1)` entries,
using the final loaded object definitions after RMG has merged its additions. There is no fixed minimum of 1024.
Creature, name and portrait overrides are read within that table's range; JSON settings alone do not extend it.
Without RMG, the plugin uses the game's loaded object definitions. Adding graphics/generators through `properties`
uses RMG. The editor has its own object-definition plugin.

## Kingdom overview

Portrait creation requires **Graphics API v1 in ERA_RuntimeServices.era**. Update the API and both extenders together.
The shared service owns frame caching, source resources, RLE and palette render hooks; the dwelling plugin owns
JSON and portrait layout. See [GRAPHICS.md](../ERA_RuntimeServices/Docs/GRAPHICS.md). Missing/old providers are logged;
native portraits remain available and extension waits for graphics readiness.

The installed constructor chooses `ZlagPort.def` in WoG or `FlagPort.def` in SoD. The plugin loads that game resource
and appends frames in memory on the first native load for the overview, after the video format is initialized.
Loading this resource during object initialization would create a black device palette. Every added `(type, subtype)` receives a distinct runtime ID, even when two entries
reuse the same image. Their counters and name hints remain separate. Original frame indices, including special
objects and unused frames, remain stable. Special WoG IDs are translated to the native SoD hint switch.

Subtype 256 must already be covered by the loaded object definitions. See [examples/dwellings.json](examples/dwellings.json)
for a complete definition with `properties` for RMG to load.

To add a portrait for type 17, subtype 256, place the separate DEF in the mod's `Data/` directory (or a game LOD),
and add a JSON file under the same mod's `lang/` directory:

```json
{
  "RMG": {
    "objectGeneration": {
      "17": {
        "256": { "image": "mydw.def", "imageFrame": 2 }
      }
    }
  }
}
```

Use the resource's name, without a filesystem path. Replace `mydw.def` with your actual resource.
`imageFrame` counts from **0**, within DEF group **0**: `2` selects the third frame.
Type 20 uses the same fields under `"20"`. Restart the game after changing resources or these settings.
The plugin creates the new runtime frame automatically; editing `zlagport.def` or assigning a global frame ID
is unnecessary. This example only sets the overview picture; creature and map-definition settings stay separate.
A PCX uses `"image": "mydw.pcx"` with no `imageFrame`.

To reuse an existing combined-resource picture, omit `image` and set `"frame": 25`, for example.
This index is relative to the type's source block: type 17 starts at its first picture, type 20 at its own first picture.
It does not append a physical frame to the on-disk DEF.
Pictures keep transparent margins, are centered and scaled down when necessary. PCX8 preserves its original indices
and active 16/32-bit palette through the API's render hooks. PNG overrides and separate DEF images are converted
to the combined DEF's palette. For ERA PNG overrides, use the PCX name, with the PNG under Data/Pcx. Missing resources
or invalid frames receive a question mark. Native ERA resource substitutions work; external PNG/BMP/JPEG/TIFF
decoding is not used. Use `frame` to borrow a combined-resource picture instead of referencing that DEF as `image`.

The legacy `RMG.settings.dwellings.frames.17` and `.20` counts describe a pre-existing customized source DEF layout.
They are unnecessary for adding new dwellings. In a legacy layout, special pictures are aliased to the native
special-object IDs. Runtime IDs are never exported into JSON.

## RMG and other dwelling plugins

Initialization hooks the game's object-loading stage. RMG may also call the idempotent
`InitializeDwellings(limits, count)` export after merging its definitions; no registration in its object dispatcher
is involved. `GetDwellingExportInfo(type, subtype, record)` returns a size-bounded, read-only snapshot when this
plugin is active. Earlier versions of that record remain supported.

If another dwelling plugin already owns the kingdom hint hook, this plugin leaves its tables, gameplay hooks and
portraits untouched and returns no owned snapshot. RMG's independent `DwellingInfo.cpp` then reads the current
native table operands and their published bounds. Its creature/town/growth hints work with relocated tables and
reflect subsequent table changes. A JSON subtype or map prototype is never treated as a foreign table capacity.
Plugins that replace the native table representation entirely need an adapter for their own data format.

RMG gives an explicitly registered object extender first refusal for generation and dialog hints. Its built-in
dwelling information/factory is the fallback when there is no gameplay owner or no custom hint, respectively.
RMG also keeps the old `GetDwellingExportInfo` export as a read-only forwarding adapter. The locale manager prefers
the standalone provider and otherwise uses that forwarding adapter.

## Verification

From an x86 Developer Command Prompt:

```cmd
ERA_DwellingsExtender\Verification\run.cmd
RMG_CustomizeObjectProperties\Verification\run.cmd
```

These run 156 portrait checks, 50 slot/save/editor checks and 20 read-only view checks. The views cover creature ID 0,
IDs above 255, sparse four-slot dwellings, relocated/mutated tables and rejecting subtypes beyond published bounds.
RMG's existing `Verification/run-object-dispatch.cmd` also verifies exact-subtype and type-fallback dispatch.

After local builds, with the game closed:

```powershell
ERA_DwellingsExtender/Verification/VerifyDwellingPortraits.ps1 -Mode Combined
ERA_DwellingsExtender/Verification/VerifyDwellingPortraits.ps1 -Mode Standalone
ERA_DwellingsExtender/Verification/VerifyDwellingPortraits.ps1 -Mode Foreign
```

Each run temporarily installs test files, starts a hidden game process, checks native callbacks at the main-menu
entry, and restores installed plugins/removes fixtures in `finally`. `Standalone` removes RMG and checks the native
table bounds and rejection of JSON-only subtypes outside the loaded range;
`Foreign` occupies the dwelling hook and relocates creature tables to verify RMG independently of this plugin.
The probe checks the device palette and renders original, borrowed, DEF and PCX portraits using native
`H3DlgDef` dialog controls. It saves a contact sheet to `Debug/Era/dwelling-portrait-render.bmp`.
Use `-Executable "h3era HD.exe"` to check the HD renderer too.
Logs are `Debug/Era/dwelling-split-<mode>.log`. Actual map mouse hover, overview scrolling and recruitment still
require manual visual/gameplay verification.
