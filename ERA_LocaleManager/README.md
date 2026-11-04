# JSON export

Configuration export is a separate component hosted in the existing DLL. The language menu opens it in
**Translations** mode (Data is initially unchecked); the main-menu **Export configuration** command opens
the full mode. Both use the same dialog and the public `ERA.Services.ConfigExportUi.v1` API.

Data comes from registered providers through `ERA.Services.ConfigExport`, discovered in Patcher memory.
LocaleManager no longer reads gameplay tables, identifies gameplay plugins by filename, or implements bank/RMG
serialization. Providers supply typed fields; ERA_RuntimeServices owns JSON construction, snapshot lifetime,
merging, filtering, counting and serialization. LocaleManager receives metadata and ready UTF-8 output.
Neither it nor the five providers includes nlohmann::json. New providers/categories appear without changes to the dialog.
Large category lists use pages. See [the API contracts and responsibility map](../ERA_RuntimeServices/Docs/CONFIG_SERVICES.md).

The language menu opens a table whose size follows its translated labels, row count and font metrics.
Each row has four independent checkboxes: **Original**, **Mods**, **Text**, **Data**. Numbers to their left
count exportable records for the other selected columns. A subtype with several DEF variants counts once;
unused capacity and holes in subtype IDs are not exported. Right-click opens the translated help.

Two checkboxes below the table control output: **Merge text and data** (off by default) writes one file
per category with the selected text/data columns; **Ignore default RMG settings** (on by default) switches
between differing and full RMG parameters. Counters follow the selected mode without rereading game tables.

Original/Mods refer to ID ranges, including currently applied overrides. They do not restore pristine values.
Inclusive original ID limits live at `era.locale.dlg.maximumId.*`; both dwelling limits default to **100**.
Object subtype limits come from the native `objects.txt` before JSON additions and can be overridden with
`era.locale.dlg.maximumId.objectSubtypes.<type>` (`-1` makes all subtypes additional).

| Category | Text | Data |
| --- | --- | --- |
| Creatures | Singular/plural names, description | Unavailable: numeric/animation readers are inactive |
| Artifacts | Name, description, event | All fields in the shared artifact schema |
| Map objects | Object type names, subtype names/descriptions and visit/hint texts | Exact merged definitions, differing RMG settings/limits, loop sounds, configured university skills |
| Dwellings | Separate category: native names and subtype names/descriptions | Definitions, creature slots (including `0`, `-1`, IDs above 255), icon frames/counts, differing RMG settings |
| Creature banks | Names, descriptions, visit/victory/plundered texts | Definitions, differing RMG settings, four native reward/guard states, troop placement, configured extra rewards and random spell filters |
| Towns/buildings | Town type/random names, common/special buildings, blacksmith descriptions, dwelling texts, neutral entries | Unavailable: reader handles text only |
| Heroes | Name, biography, specialty texts | All supported startup fields, army types and portrait resources |
| Spells | Full/short names and four descriptions | Target/school/level/flags, effects, costs, probabilities, AI values and sound/animation resources |
| Secondary skills | Name and three descriptions | Unavailable: reader handles text only |

Data availability follows the fields actually contributed by registered providers. Read-only snapshots in
`headers/Era/JsonObjectExport.hpp` avoid guessing relocated allocation sizes. By default RMG exports only
explicit configuration that differs from its inherited settings. Native generator values omit redundant
value/density overrides; added generators keep the settings needed to create them. Global/type dependencies
are retained, including aggregate limits. Per-user RMG INI settings and placed-map state are outside this format.
Automatic dwelling prices are not frozen to their calculated creature AI values. An explicit `value: -1` is
preserved when it cancels a fixed inherited price; full export retains automatic prices as `value: -1` too.
The default filter affects only the five RMG settings, not other object properties. Dwelling names use one shared game/editor key rather than
duplicating the same text in both `RMG.objectGeneration` and `era.dwellings1/4`. Extra bank
reward configuration preserves random-generation rules instead of exporting a particular randomized reward.
Inactive wall, creature-animation and legacy-list handlers are intentionally excluded.

Files are UTF-8 JSON with CRLF and numeric values stored as numbers. Existing files are replaced only after
a complete temporary file has been written:

- `Runtime/Exports/<language>/*Text.json` — copy into a mod's `Lang/<language>`.
- `Runtime/Exports/Data/*Data.json` — copy into the mod's `Lang` root.
- `Runtime/Exports/Combined/<language>/*.json` — merged text and data, one file per category.

These use the same keys as the game/editor readers. Restart both after changing structural data. The editor
consumes object definitions/hints and supported artifact fields; game-only hero/spell fields remain game-only.

`lang/` contains all four dialog translations. `Install-JsonExport.ps1` installs those files and the four built
plugins, preserving the previous binaries, debug maps and translations under the mod's `Backup/JsonExport-*`.
Its default build folder is `Verification/build` (plugins in `plugins/`, maps in `DebugMaps/`).

Run `Verification/run.cmd` in an x86 Visual Studio Developer Command Prompt to check all 16 checkbox
combinations, empty selections, sparse subtypes, DEF variants, array extension and artifact/dwelling reader
round trips. Build affected projects as Release/Win32 before installation.
