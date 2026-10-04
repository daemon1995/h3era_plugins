# ERA Help Dialog

An in-game reference library for Heroes III ERA. F1 and the main-menu **Help and reference** button open it. F1 from the standard town information window selects that town. The library combines current native game tables with documentation from active mods. UI strings, the bundled guide and authoring examples are in English.

## Browsing and filtering

The normal library window is 800×600, clamped only when the game viewport is smaller. Resize toggles between this size and the full viewport. All list and text scrollbars use the native brown theme.

Eight tabs cover Mods, Hotkeys, Creatures, Artifacts, Heroes, Secondary skills, Spells and Towns. Left-click a portrait or name to open a scrollable information card. Hold the right mouse button for a native `RMB_Show` popup; releasing it dismisses the preview. Pictures are centered above the title in both versions. Spell and secondary-skill cards on both left and right click use compact mastery tables, one row per level. Spell columns are Mastery, Mana, Strength and Effect; skill columns are Mastery and Effect. Row height follows native font wrapping and the popup follows content height. Exceptionally long mod descriptions can scroll within their cell; additional JSON notes remain below the table. Creature, hero and artifact cards use compact structured fields and a horizontal row of core stats where applicable. Hero portraits and starting data remain native; the biography is available in the left-click card. Starting army rows use CPRSMALL.def icons (including war machines) with plural creature names and exact quantities or random ranges; skills use Secskill.def at the starting mastery, and the starting spell uses SpellInt.def with its name. Spell strength shows the native base value plus the native coefficient multiplied by hero Spell Power (SP); it is a formula rather than a prediction of combat damage after bonuses or resistances. All close buttons in this plugin are at the bottom right; the main hint bar sits to their left. Right-click cards retain RMB_Show behavior and have no close button. Secondary skills use a compact grid that fits all 28 entries at 800×600. Artifact, spell, hero and secondary-skill portraits show tpthchk.def frame 2 in their bottom-right corner when currently banned. Restrictions are read again while the list is open, when changing tabs, filtering or scrolling. Scenario ban arrays are used only with a loaded map and within their native bounds (144 artifacts, 70 hero spells, 156 heroes, 28 skills); creature abilities are not confused with banned hero spells. Artifact setup restrictions are also respected. Heroes offered in taverns or already owned by a player are not treated as banned merely because they cannot be randomly hired.

The Mods button opens a dropdown directly beneath it with the same width. Its height follows the number of active mods and built-in references, showing at most ten rows; more entries use a scrollbar. The current mod is highlighted and initially visible. Click a row or use Up/Down, Home/End and Enter to choose; Escape or an outside click cancels. If there is insufficient space below, the list opens above the button.

All six object catalogues share `CatalogueSection`. The left column contains only categories, with counts after applying the other filters. Additional filters, local text filtering, sorting and reset are above the object grid. Controls use 22-pixel heights with two-pixel gaps, fitting three or four facet filters per row when space allows. The result count shares the level row. Left-click an additional filter to advance its value; right-click to go back. Text filters match titles only, require every word, ignore ASCII case and allow words in any order. Reset clears every filter and restores ID order. ID is the default; sorting can cycle through name, ID and level/name. An existing saved sort preference migrates to ID once; subsequent explicit choices are preserved.

| Catalogue | Categories | Additional filters / information |
| --- | --- | --- |
| Creatures | Nine towns, neutral creatures, war machines, commanders | Multiple levels 1–7; movement, attack, life and upgrade status. Cards show stats, recruitment cost, growth, traits and upgrade. |
| Artifacts | Treasure, minor, major, relic, special | Equipment position, availability, combination components, spells. Cards include cost, description and combination components. |
| Heroes | Nine towns | Might/magic, regular/campaign, spellbook, gender. Cards include biography, specialty, class starting stats, skills, army ranges and starting spell. |
| Spells | Fire, air, water, earth, creature abilities; multi-school entries may appear in several groups | All 81 native entries, including creature abilities at IDs 70–80. Multiple levels 0–5, combat/adventure/creature use, damage and expert mass effect. Mastery descriptions, mana costs and base effects appear in cards. |
| Secondary skills | Adventure, combat, magic, economy/leadership | Name filtering. All three mastery descriptions. |
| Towns | Good, evil, neutral alignment | Text search. Native creature roster, hero classes and optional JSON notes. |

Spell portraits in the grid and object cards use a school underlay at frame 0, with the same position and item size as the spell DEF. Native school index order is Air, Fire, Water, Earth; multi-school spells use the first matching index. The resources are SpLevA.def, SpLevF.def, SpLevW.def and SpLevE.def. No-school abilities have no underlay. Missing or oversized resources are ignored.

The all-levels state is an empty selection. Selecting level 2 and level 4 shows both; clicking a selected level removes it. Spell level 0 includes native creature abilities. Creature levels outside 1–7 remain accessible with All levels.

## Search and state

Clicking another control releases text-field focus; clicking a text field preserves its focus. The **Search** text field is always visible below the tabs; **Ctrl+F** focuses it. Typing shows a result panel inside the main dialog covering all catalogues, documented active mods and individual hotkeys. Search matches the displayed result titles: object names, mod/category headings and hotkey binding/action labels. It does not index article content, descriptions, stat summaries or positioned JSON objects. Local catalogue filters likewise match object names; local hotkey filters match their displayed binding/action/source labels. Results are ordered by page ID and then object ID, or mod/category/binding IDs. Choose a scope, then left-click a result or use Up/Down and Enter to navigate. Native results clear conflicting filters, reveal the object and highlight its tile with a frame; click it to open its card. Mod results open their category. Hotkey results select their context and filter by binding/action text plus the source folder. Hold the right mouse button on any result for a native popup preview without navigating or changing the search position/query. Mod category previews show their text and first image; hotkey previews show the action description. Previewing never executes positioned actions. Tabs remain available. Clear, Back or Escape returns to browsing.

Filters, queries, sort order, selected object, scroll row, hotkey context and the selected mod/category survive resizing. They are stored in `ERA_HelpDialog.ini`, section `Help`. Mod keys use the folder name rather than a transient list index. The ordinary opener remembers its last tab; targeted API calls do not replace that preference. Within a dialog, each visited mod category retains its own text scroll position; the current category position is saved on closing.

Hotkeys use one heading per action context in both the global catalogue and each mod. Within a context they follow mod ID and JSON binding ID, retaining sparse numeric IDs. The Hotkeys tab always combines all active sources, independently of the mod selected on the Mods tab; it has no source selector and ignores the old HotkeyMod preference. Lines show the binding and action, with the source folder in parentheses in the global list. Context and title filters still apply; identical bindings from different mods remain separate.

Individual hotkey search previews highlight the binding in a large gold font inside a framed box. The action title appears separately, followed by the source mod and action context. Description text uses a medium font below a divider; long descriptions scroll with the brown theme. These previews retain native RMB_Show behavior and size themselves to their content.

## JSON format

Copy `lang/era_help.json` to an active mod's `lang` directory. ERA merges language files; mods may supply their own documents separately. Use the **lower-case actual mod folder** as the key. The built-in `era help` document is always discoverable. Active physical mods appear when they have their own help content or actual plugin/patch files. A Plugins section is generated only when matching files exist. Empty mods and empty plugin sections are omitted. Heroes III is always included; HD mod is included when its module is loaded. GEM documentation lives separately in that mod's lang directory and is discovered only while the mod is active.

```json
{
  "help": {
    "my mod": {
      "name": "My Mod",
      "description": "An optional mod overview.",
      "plugins": [
        { "file": "EraPlugins/MyPlugin.era", "name": "My plugin", "description": "An optional file description." },
        { "file": "EraPlugins/AfterWog/Fix.bin", "description": "An optional patch description." }
      ],
      "hotkeys": [
        { "keys": "Ctrl+H", "name": "Open rules", "description": "Show this mod's rules.", "type": "ADV_MAP" }
      ],
      "categories": [
        {
          "name": "Rules",
          "content": "Read the rules before using the optional actions.",
          "objects": [
            { "kind": "text", "value": "Appended paragraph." },
            { "kind": "button", "value": "View creature", "action": "creatures:0", "x": 12, "y": 110, "width": 180, "height": 30 },
            { "kind": "image", "value": "artifact.def", "frame": 7, "x": 208, "y": 110, "width": 44, "height": 44 },
            { "kind": "link", "value": "Search", "action": "search", "x": 12, "y": 150, "width": 180, "height": 30 }
          ]
        }
      ]
    }
  }
}
```

Plugins enumerate .era, .dll and .bin files in exactly EraPlugins, EraPlugins/BeforeWog and EraPlugins/AfterWog below each active physical mod. Other extensions, backups, directories and deeper folders are ignored. Files appear as an alphabetical list of filenames under folder headings: EraPlugins, EraPlugins/AfterWog, EraPlugins/BeforeWog. Empty groups are omitted. Optional author names and descriptions appear beneath their filenames. Metadata paths are relative to the mod folder; matching ignores case and slash direction, and an exact relative path wins over a filename fallback. Metadata alone does not create a file entry. This lists supplied files, not whether a library was loaded. Plugin notes accept arrays or numeric-key objects with the same 4096-entry and 32-missing-entry bounds. Plugins and optional Overview sections are appended to preserve existing category indices.

Hotkeys and categories accept arrays or objects with numeric keys. Prefer consecutive indices. A short gap is allowed; reading stops after 32 consecutive missing entries or index 4095. Object lists stop at the first missing `kind` and are bounded at 4096 entries. Missing optional strings remain empty. Invalid native IDs and absent/out-of-range DEF frames are ignored.

The old root `help.mods.<folder>` remains a per-field fallback. Old hotkeys under `categories.hotkeys.content` also work, including `key` instead of `keys`. The preferred `hotkeys` form takes precedence when it contains usable records. Contexts accept numeric -1…6 and aliases: EVERYWHERE/GLOBAL/ALL/ANY (-1), NONE (0, grouped as Other), ADV_MAP/ADV_MAP_DLG/ADVENTURE/MAP (1), HERO/HERO_DLG (2), TOWN/TOWN_DLG/CITY (3), COMBAT/COMBAT_DLG/BATTLE (4), MAIN_MENU/MAIN_MENU_DLG/MENU (5), or OTHER (6). Matching ignores case.

Objects support `text`, `image`, `button`, `link`, `erm` and `ErmFunction`. `value` is their text or resource name. Text without `overlay: 1` is appended to the scrollable article. Overlay objects use coordinates relative to the content panel; `width`, `height`, `frame` and `zOrder` are numeric. Larger zOrder values are created later. Images use native PCX resources or group-zero DEF frames; resources are shown at their native dimensions when they fit the supplied rectangle. Coordinates and dimensions are clamped to the panel. Positioned objects remain fixed while article text scrolls. Controls are cached per category; the dialog has a 4000-control budget for positioned objects.

Actions are handled only when clicked:

| Action | Result |
| --- | --- |
| `creatures:0`, `artifacts:7`, `heroes:0`, `spells:15`, `skills:2`, `towns:0` | Open the native object card. |
| `mod:wog:0` | Open a mod category. Runtime indices include the generated Hotkeys category at index zero, when present. |
| `search`, `guide` | Show the search panel or open the guide. |
| `https://…`, `http://…` | Open the system browser. |
| `erm:HelpDialog_Example`, `erm:12345` | Call a named or numeric ERM function without arguments. ErmFunction objects also accept a bare function name. |

`examples/HelpDialogExample.erm` supplies the named demonstration function. Install it in an active mod's `Data/s` directory and restart before using that example. Parsing a document never executes its actions.

Additional native-object notes can be supplied as `help.creatures.entries.0.description`, with equivalent `artifacts`, `heroes`, `spells`, `skills` and `towns` roots. `help.ui`, `help.contexts`, `help.<catalogue>.categories.<index>.name` and `help.guide.content` customize labels and the guide. Native town names in creature/hero categories always come from the game town table. The supplied JSON contains over 30 guide/example categories, working navigation/image layouts, compatibility examples and notes for all nine towns.

## Integration API

Use `headers/EraPluginsAPI/HelpDialogAPI.hpp` from another plugin. It resolves the already loaded DLL and returns FALSE if the export is unavailable.

```cpp
era_help::ShowDialog(era_help::Page::Creatures, 0);
era_help::ShowObject(era_help::Page::Artifacts, 7);

// Direct card, without opening or navigating the library:
era_help::ShowObjectHint(era_help::Page::Creatures, 0, TRUE);  // RMB popup
era_help::ShowObjectHint(era_help::Page::Spells, 15, FALSE);  // OK dialog
```

The stable exports are `BOOL __stdcall ERAHelp_ShowDialog(int page, int subtype)` and `BOOL __stdcall ERAHelp_ShowObject(int page, int id)`, with decorated stdcall names also retained. Page IDs: Mods 100, Hotkeys 101, Creatures 102, Artifacts 103, Towns 104, Heroes 107, Secondary skills 108, Spells 109. Call after ERA initialization, on the game UI thread. ShowDialog refuses a second simultaneous main dialog; ShowObject can navigate an existing one. Invalid pages/objects return FALSE. An initial ShowObject call selects the object; calls from an open library also open its card.

`BOOL __stdcall ERAHelp_ShowObjectHint(int page, int id, BOOL popup)` opens the requested card directly. It supports Creatures, Artifacts, Heroes, Spells (including creature abilities), SecondarySkills and Towns. `popup = TRUE` uses native `RMB_Show`: invoke it from your right-click handler while the button is held. `popup = FALSE` opens the same card as a modal dialog with OK/Enter/Escape. The call returns after the card closes, restores the caller's window-manager result, and does not change library filters or selection. The card reads fresh native object data and author JSON notes for that single ID. Calls before native Help initialization, recursive hint calls, unsupported pages, invalid IDs or card creation failures return FALSE. The wrapper also returns FALSE when the plugin or new export is absent, without loading a DLL. No import library is required. See `examples/ExternalObjectHints.cpp` for handlers covering all six object types.

## Implementation and supported data

`MainDlg` owns page models and sections, routes navigation/actions, and loads/saves state. `CatalogueData` converts native tables into records. `HelpLogic` contains game-independent matching, bounds, integer parsing and hotkey parsing. `SearchPanel` owns global search controls in the main dialog; it never starts a modal window. `PluginFiles` enumerates physical plugin and patch files without loading them. `ModJson` centralizes root compatibility and missing-field handling. `ModInformation` owns parsed documents; `ModPage` renders them. `DlgPage` controls visibility of composite native text widgets and avoids drawing before the window manager saves the underlying screen.

Creature and artifact counts come from the game. The available fixed native data cover 163 heroes, 28 secondary skills, nine towns and 81 spells. `H3SpellCount` counts the 70 hero spells; the catalogue uses the full native table, including creature abilities. Cards describe table values and hero defaults. Script-computed effects and additional non-native towns/skills/spells need explicit mod metadata or another API; the library does not guess them. Search casing uses the game's byte strings and ASCII case folding; Unicode normalization is not implemented.

All library dialogs build their background with bounded texture tiles through `AddSafeBackground`. The bundled H3API background helper passes the remaining dialog width to the texture renderer instead of the tile width. The Halon crash report traced this read beyond the texture to the HD renderer while constructing the object card. The library avoids that helper without changing the shared H3API header.

## Build and verification

Build `ERA_HelpDialog.vcxproj` for Win32 with the repository toolset. Override `PluginOutDir` and `IntDir` for an isolated build; the repository defaults otherwise deploy directly to the configured game. Both Debug and Release are supported. The existing XP toolset reports its deprecation warning.

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' .\ERA_HelpDialog\ERA_HelpDialog.vcxproj /p:Configuration=Release /p:Platform=Win32
```

Before publishing, check in game at 800×600 and an HD resolution: open/close F1, visit every tab, combine creature towns/levels/facets, filter multi-school spells, scroll both panels, resize/reopen, search and follow all result types, change mods after scrolling a long category, and use image/navigation/ERM examples. Verify Cancel/Escape/Enter and return to the original game window without leftover controls.

## Shortcut sources and GEM package

The bundled reference adds 48 Heroes III bindings and 50 HD bindings, grouped by context. The base controls were checked against the publisher's [manual, keyboard pages 18-19](https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/297000/manuals/Heroes_of_Might_and_Magic_III_HDEdition_OnlineManual_EN.pdf?t=1699009789); the HD Edition F2 graphics command is excluded. HD controls were checked against the author's [functionality](https://sites.google.com/site/heroes3hd/eng/functionality), [tweaks](https://sites.google.com/site/heroes3hd/eng/tweaks) and [compatibility](https://sites.google.com/site/heroes3hd/eng/description) pages. HD features vary by build, settings and ERA compatibility. These are documentation entries, not bindings installed by Help Dialog.

Copy examples/gem_help.json to Mods/Game Enhancement Mod/lang/gem_help.json. It adds a mod overview, six articles, 17 verified bindings and 17 descriptions matching the files in this installation. Its root uses the actual lower-case folder name, game enhancement mod. Keep it out of the WoG shared language directory. Restart after changing active mods. GEM details were checked against its local author readme, language files, core scripts, army-action script and workspace source; entries without a detailed supplied specification say so.

Object-note examples are stored in `examples/object_notes.json`; they are intentionally absent from the runtime `lang/era_help.json`. Authors can copy relevant entry descriptions into their own mod language JSON.
