# Assembly_MainPlugin 1.13.0

Assembly versions use one compact runtime PCX16 block: a `medfont.fnt` title
on the left and `smalfont.fnt` installed/latest rows on the right. The three
items have distinct free IDs and share their thin frame edges. The ERA engine
version remains a separate label. All labels retain the black background and
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
| `width` | 280 | Total block width in pixels. |
| `title_width` | 130 | Left-column width. |
| `row_height` | 16 | Right-row height; a single-row block reserves enough height for medfont. |
| `x`, `y` | -1, -1 | Position relative to the menu dialog; negative values anchor at the bottom right with 10 px right / 2 px bottom margins. |

Frames share one pixel: there is no gap between rows or columns. Widths and
positions are limited to the dialog. Use the same dimensions in all language
files, and localize only the three captions. Language reload updates the images
in place; restart the menu/game after changing geometry. Longer custom captions
must fit the configured cells; increase the common width if needed.

The old `current_version`/`online_version` `display_version`, `custom_text`,
`custom_version` and other version-source settings remain in their existing
locations. Their old `format`, `x` and `y` no longer define these two rows;
the shared block keys above replace them. ERA engine label `format`/`x`/`y`
remain separate. When one assembly row is disabled, the other fills a one-row
block. When both are disabled, the title is omitted.
