# Assembly_MainPlugin 1.14.1

Assembly versions form a single row: `Assembly | v1.23 | v1.25`.
The cells are title, installed version, latest version, all in `smalfont.fnt`.
Each is one runtime PCX16 widget with a distinct free ID, black background,
gold text and shared one-pixel frame edges. Only Latest blinks when a newer
version is available; its frame and click target remain visible. The installed
version text is red while it is older, and returns to gold otherwise.

The installed version is read from `[Assembly] Version` in `ERA_Project.ini`.
A nonempty `current_version.custom_version` explicitly overrides that value.
The registry is never read; custom display text does not prevent loading the
actual version used for comparison.

The row touches the bottom-right corner of the fitted menu picture. ERA stays
at its bottom-left corner. MainMenuAPI image bounds exclude the surrounding
canvas, frame and HD texture; native dialog bounds are the fallback when its
background rendering is disabled or MainMenuAPI is absent.

Geometry is fixed in code, independent of JSON and translated text: the title
cell is 80 pixels wide, each version cell 70, total 218 with shared borders.
Height uses the loaded smalfont height plus six pixels of internal padding.
There are no outer margins. Disabled version cells are omitted; if both are
disabled, the title is omitted too.

Only captions under `gem_plugin.main_menu.assembly_version` are localized:

| Key | Default | Meaning |
| --- | --- | --- |
| `title` | `Assembly` | First cell; Russian translation is `Сборка`. |
| `installed_format` | `v%s` | Installed version; keep `%s`. |
| `latest_format` | `v%s` | Latest version; keep `%s`. |

The `v%s` formatter removes an existing leading v/V before adding its own v,
so server tags such as `v2.300` are not displayed as `vv2.300`.
Install `Data/assembly_version.json` in `Lang/` and its Russian counterpart
in `Lang/ru/`, or merge these keys into `assembly_main.json`. Language reload
updates images without changing their geometry.

Existing `display_version`, `custom_text`, `custom_version` and version-source
settings remain supported. Old installed/latest `format`, all per-version
`x`/`y`, and block `width`, `title_width`, `row_height`, `x`, `y` are ignored.
The ERA engine label retains its separate format setting. Obsolete
`read_registry`, `length_in_px`, `font`, `x`, `y`, old installed/latest `format`,
online `check_online`/`remote_file` and block geometry keys are removed from
the installed JSON. Online request parameters come from the existing
`[GitHub]` section of `ERA_Project.ini`, not those old JSON fields.
