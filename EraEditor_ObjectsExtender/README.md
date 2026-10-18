# EraEditor_ObjectsExtender

Native ERA map-editor plugin for JSON object prototypes, names and descriptions. Texts and prototype loading
work for every supported object type (0–231), including creature banks, universities and objects from the
HotA/WoG extenders. Creature dwellings additionally have an adapter for expandable native name tables:
type **17** (one creature) and type **20** (up to four creatures).

Keep this DLL alongside `EraEditor_JsonOverrides`. That plugin adapts artifact and existing TXT tables;
this one handles map-object prototypes and per-type/subtype hints. Both use `headers/Era/ObjectText.hpp`
for object text keys, also shared with `RMG_CustomizeObjectProperties`. No third plugin or duplicated
text hook in each object extender is needed.

Build **Win32**, Debug or Release. The default output is:

```text
$(LocalGamePath)\Mods\$(PluginModName)\EraEditor\EraEditor_ObjectsExtender.dll
```

Explicit `PluginOutDir` overrides still work. Release builds also run the repository's MapToDbgmap converter.
The DLL checks the supported ERA editor's executable signature before importing `eramap.dll`; it stays inert
when ERA discovers it from the game process.

## Configuration

Object texts use the same keys as the game plugin:

```json
{
  "RMG": {
    "objectGeneration": {
      "16": {
        "256": {
          "name": "New creature bank",
          "description": "A guarded bank with custom rewards."
        }
      }
    }
  }
}
```

Put JSON in the mod's `Lang` directory. [The text example](examples/object_text.json) covers a creature bank,
university, gazebo and warehouse. It supplies texts for objects defined by their respective extenders;
the example alone does not add their graphics, rewards or game behavior.

The editor uses `name` followed by `description` on a new line in palette and placed-object hints. Missing
texts preserve the native hint. With only `description`, the heading is the native object **type** name;
provide `name` too when a particular subtype needs its own heading. An explicitly empty `name` hides the
heading. Hints are limited to **199 bytes** because the native editor uses 200-byte buffers.
Texts are read live, so a language-data reload updates them. Gameplay fields such as `text.visit` and
`text.hint` keep their separate meanings; they are not aliases for the editor description.

In the game, the shared `description` is also shown in the object's RMG settings right-click information.
It does not replace visit messages or dynamic gameplay hints.

## Object prototypes and dwellings

Place [the shared six-object example](../RMG_CustomizeObjectProperties/examples/dwellings.json) in the mod's
`Lang` directory and restart the editor. It adds subtypes **256, 257, 258** of both types, including a four-slot
dwelling with an empty first slot. All six examples reuse an existing `AVGgrff0.def`; replace its DEF and masks
with your own graphics when authoring a mod.

`RMG.objectGeneration.<type>.<subtype>.properties` contains the usual nine-field object-definition lines. The
plugin merges these into the editor's active `ZEObjts.txt`, updates its count and replaces matching
DEF/type/subtype entries without duplicating them. Existing flat and nested `RMG.<mod>.properties` lists are supported
for all object types too. Per-subtype properties are discovered for IDs 0–1023; mod-level lists can add
higher IDs up to the map format's 65535 limit.

Names use this priority:

1. `RMG.objectGeneration.<17 or 20>.<subtype>.name`.
2. The editor's current `ZCrGn0`/`ZCrGn1` and `crgen4` names.

No additional dwelling TXT configuration is loaded. Creature composition is determined by the game plugin
when the map starts, using the shared JSON `creatures` settings. Restart after changing object definitions;
prototype discovery is cached for the editor session. Names also refresh on language-data reload.

## Editor integration

The adapter reads current name-table operands after native TXT loading, including relocations installed by
GrayFace/Unleashed. It publishes separate expandable eight-byte dwelling records for types 17 and 20 and
restores full subtype indexing for map hints, palette hints and dwelling name wrappers. Existing records are
also updated so GrayFace's own dwelling list sees configured names. No game-executable addresses or game
allocation routines are used in editor hooks.

The custom GrayFace settings window keeps its own configured list length; this plugin expands the native
palette and hint tables without changing `MapEdPatch/Options.ini` or that private list.

## Verification

The shared checks are in `RMG_CustomizeObjectProperties/Verification`:

```bat
run.cmd
python NativeLayout.py "D:\games\HoMM 3 ERA\h3era.exe" "D:\games\HoMM 3 ERA\h3wmaped.exe"
```

The first command requires an x86 Visual Studio Developer Command Prompt. It checks JSON numeric fields,
disabled/sparse slots, invalid IDs, save validation, generic prototype replacement/deduplication and shared
object text fallback/reload/length handling. The second checks native hook spans, editor string helpers,
table operands and the JSON examples.

`Verification/LoaderSmoke.cpp` is an additional x86 `LoadLibrary` check: pass the built DLL's full path to
the compiled executable. Both Release and Debug DLLs must load/unload without importing `eramap.dll` when
the host is not the supported editor.

Manual editor verification: load the example, locate all six entries, place each type/subtype, check palette
and map hints, save/reopen the map, then start it with the game extender and check the recruited creatures.
For other extenders, add their actual prototype/game configuration and the text example, then check both
palette and placed-object hints, including an object with only a description and one with no JSON texts.
