# Assembly_MainPlugin 1.13.1

Assembly versions use one compact runtime PCX16 block: a `medfont.fnt` title
on the left and `smalfont.fnt` installed/latest rows on the right. The three
items have distinct free IDs and share their thin frame edges. The ERA engine
version is anchored at the bottom left of the game window; the assembly block
is anchored at the bottom right. This uses the actual draw buffer, including
when HD Mod centers the menu dialog inside a larger window.
All labels retain the black background and
gold text. Only the latest version blinks when it is newer than the installed
version; its frame and click target remain visible.

The block has fixed dimensions across languages and server replies. Translated
text does not change the geometry. Install `Data/assembly_version.json` in the
mod's `Lang/` and `Data/ru/assembly_version.json` in `Lang/ru/`. The equivalent
settings can be merged into an existing `assembly_main.json`.

Keys below `gem_plugin.main_menu.assembly_version`:

| Key | Default | Meaning |
| --- | --- | --- |
| `title` | `Assembly version` | Left caption, using medfont. |
| `installed_format` | `Installed: %s` | Installed row, using smalfont; keep `%s`. |
| `latest_format` | `Latest: %s` | Latest row, using smalfont; keep `%s`. |

Frames share one pixel: there is no gap between rows or columns. Widths and
positions are fixed in code: the assembly block is 280 pixels wide with a
130-pixel title column and 16-pixel version rows; the engine label is 160x16.
Both corners use 10-pixel side and 2-pixel bottom margins. A single-row block
reserves enough height for medfont. Only captions are localized in JSON;
language reload updates the images in place. Captions must fit their cells.

The old `current_version`/`online_version` `display_version`, `custom_text`,
`custom_version` and other version-source settings remain in their existing
locations. Their old `format`, `x` and `y` no longer define these two rows;
only the three translated caption keys above are read for the assembly block.
The former `assembly_version.width`, `title_width`, `row_height`, `x`, `y`
and all old per-version `x`/`y` keys are ignored. The ERA label `format`
remains separate. When one assembly row is disabled, the other fills a one-row
block. When both are disabled, the title is omitted.
