# Interface_MainMenuAPI 2.4.2

One plugin owns main-menu background rendering, logos, video positioning and
the button panel. It replaces Interface_MainMenuLogo,
Interface_MainMenuGraphics and Graphics_MainMenuVideoPositioning. Keep only
`Interface_MainMenuAPI.era` installed; the replaced `.era` files must be removed
or disabled to avoid installing the same hooks twice.

It also owns the JSON notification panel formerly in Assembly_MainPlugin.
Use Assembly_MainPlugin 1.12.0 or newer with this version to avoid duplicate
notification panels. Assembly continues to handle assembly versions and updates.

Notifications retain `era.<lowercase mod folder>.notification.{name,text,url}`
and numbered entries `notification.0` through `notification.4`. The single-message
form takes precedence; numbered entries stop at the first missing text key.
Panel captions retain `era.notifications.*`, and hidden-message history stays in
`runtime/era/main_menu_notifications.ini`. Existing `era_notifications.json`
translations and `ntf_*.def` resources remain in the mod's Lang/Data files.
The panel works without Assembly and when all assembly version labels are disabled.
Heroes Launcher detection moved with the panel; its call is commented out.

The notification panel anchors to the fitted picture's lower left corner,
with a 20-pixel left margin and a 32-pixel bottom band for version labels.
Position is resolved at the first draw, using the picture bounds rather than
those of the original 800x600 menu or its button column. Very small pictures
fall back to the native dialog area to keep the panel usable.
Its backing is a downsampled, blurred and darkened crop of the clean menu
picture, cached for the current menu. The body and native button backgrounds
use the same palette. The counter has its own backing, redrawn together with
its digits. Scrollable descriptions restore a clean crop of the themed body
through the native scroll canvas. Native button input,
pressed state and page-boundary disabling remain unchanged. Styling is built
after the clean menu background is available, including the initial auto-open.

Link clicks and the hand cursor compare the widget found by the dialog with
the saved notification-name widget pointer, requiring it to be visible and
active. Names do not need separate IDs or a manually maintained hit rectangle.
Name widget width is the font's maximum line width plus two pixels, clamped to
the available name area. The widget and underline stay centered and resize
together after a language change, including when a longer translation is loaded.
Previous/next buttons use `SendCommand(5 + isEnable, 4096)` according to the
available pages, updating after switching, hiding notifications or reopening
the panel. Both are disabled when there is no adjacent page.

The button panel captures its backing image on the menu's first full redraw,
after HD Mod has installed its background and before the panel items draw.
Restoring an outside panel therefore uses the menu texture rather than stale
adventure-map pixels left in the screen buffer during menu construction.

Original menu DEF buttons with PNG replacements listed in
`era.png_backs.data\\defs\\<def name>\\<group>_<frame>.png.file` use a runtime
backing image while the custom menu background renderer is enabled. After the
menu background is fitted and drawn, the plugin captures the clean screen.
Every PNG frame is alpha-composited onto that snapshot at its actual drawing
position, so hover animation cannot accumulate pixels from previous frames.
The old static background `file` selects these frames; its `x`/`y` are ignored
for this menu drawing path. Existing `LegendMenu.json` entries need no changes.

Source PNGs are read from the game's virtual `Data/Defs/` tree or Data ZIP
archives using ERA's existing libspng/kubazip libraries. RGB565 and BGRA32,
clipping and horizontal mirroring are supported. Frames outside these menus,
unconfigured frames and unreadable images retain ERA's drawing path. PNG and
ZIP lookup caches reset on language reload; screen snapshots are released
with their menu dialogs. New/load title PNGs at
`data\\pcx\\mainmenu\\znewgam.png` and `zloadgam.png` use the same runtime
composition when listed in `era.png_backs`. Both PCX8/PCX16 drawing paths and
the original `newgame.pcx`/`loadgame.pcx` aliases are supported. Other PCX
images retain ERA's rendering, including the main menu background itself.
Rendering entry hooks leave the original registers, flags and stack unchanged
when a menu PNG is not handled, preserving the ERA/HD chain for ordinary ERM
dialogs. Only a successfully drawn menu image bypasses that chain.
The four render hooks are created inactive, applied when a menu backing image
is captured and removed when the last such menu closes. Gameplay rendering
therefore does not pass through this plugin's PNG handlers. Nested menu pages
keep the hooks active until their parent and child snapshots are released.
The campaign-set selector uses the same fitted menu background, panel layout,
deferred backing capture and PNG rendering as the main and new/load menus.

MainMenuAPI 2.2.1 and Assembly 1.12.2 combine link-hover cursor requests for
each menu message and apply the cursor once after the hook chain. Remaining
over the same link does not reset the cursor between the plugins. Each
assembly-version badge is one PCX16 item with its own unused dialog ID.

The plugin remains in H3EraPlugins.sln and is also referenced by
Commissions/Commissions.sln. Its default output mod remains WoG.

## Configuration

All settings and translated captions use `era.main_menu`. A complete default
configuration is in `Data/main_menu.json`; Russian and Spanish captions are
in `Data/ru/main_menu.json` and `Data/es/main_menu.json`. Install these in
the mod's `Lang/`, `Lang/ru/` and `Lang/es/` directories respectively.
Boolean settings accept JSON booleans or numeric 0/1.

| Key below `era.main_menu` | Default | Meaning |
| --- | --- | --- |
| `background.enabled` | true | Draw the loaded Gamselbk.pcx using its actual size and the configured bounds. False retains the game/HD background renderer. |
| `background.width`, `background.height` | 0, 0 | Inner canvas dimensions; zero/negative means the corresponding PCX dimension. The canvas is limited to the current game buffer. |
| `background.frame.enabled` | false | Draw the four-pixel HD frame outside the canvas, reserving room for all four edges. |
| `widgets.frame.enabled` | true | Draw the same HD frame around the API button panel. |
| `video.position.enabled` | true | Enable the former VideoPositioning compatibility fix for menu video 33. |
| `video.position.x`, `video.position.y` | 0, 0 | Coordinates supplied to OpenVideo when that compatibility fix applies. |
| `logos.main.enabled` | false | Add a logo in main/new/load/campaign menus. |
| `logos.scenario.enabled` | false | Add a logo in scenario selection. |
| `logos.{main,scenario}.asset` | empty | PCX resource name; an empty name disables the logo. |
| `logos.{main,scenario}.x`, `.y` | 23, 23 | Logo position relative to the dialog. |
| `logos.{main,scenario}.item_id` | 666 | Dialog item ID, clamped to 1..32767. |
| `rendering.redraw_always` | false | Force main-menu logo and assembly-version label redraws during callbacks. |
| `widgets.visibility_toggle.enabled` | true | Create the hide-panel button. |
| `widgets.system_options.enabled` | false | Create the system-options button. |
| `widgets.wog_options.enabled` | true | Create the WoG-options button. |
| `widgets.{visibility_toggle,system_options,wog_options}.text` | English caption | Localized button caption. |

Settings are read during initialization/creation. Caption changes are reloaded
on `OnAfterReloadLanguageData`; restart the game after changing hook settings.
The menu lifecycle is initialized even if no external widgets are registered,
including when the DLL is loaded late, before menu construction.

## Migration

Old JSON keys are replaced by the following names; the new implementation
does not read the old aliases:

| Old | New |
| --- | --- |
| `nmmi.menu_info.main.*` | `era.main_menu.logos.main.*` |
| `nmmi.menu_info.select_scenario.*` | `era.main_menu.logos.scenario.*` |
| Logo `enable`, `pcx_name` | `enabled`, `asset` |
| `nmmi.menu_info.main.always_draw` | `era.main_menu.rendering.redraw_always` |
| `era.api.main_menu.widgets.hide.create` | `era.main_menu.widgets.visibility_toggle.enabled` |
| `era.api.main_menu.widgets.hide.text_hide` | `era.main_menu.widgets.visibility_toggle.text` |
| `era.api.main_menu.widgets.{system_options,wog_options}.create` | `era.main_menu.widgets.{system_options,wog_options}.enabled` |
| `era.api.main_menu.widgets.{system_options,wog_options}.text` | `era.main_menu.widgets.{system_options,wog_options}.text` |

The unused `hide.text_show` setting is removed. The former video plugin had
no active JSON reads: its commented `era.acredit_pos` examples do not become
aliases. Native/mod settings with that old name are outside this plugin.

The DLL filename, four exported MainMenu functions and widget registration
names are unchanged, preserving the existing C++ API and its callers.

## Background sizing example

```json
{
  "era": {
    "main_menu": {
      "background": {
        "enabled": true,
        "width": 1067,
        "height": 600,
        "frame": { "enabled": true }
      },
      "widgets": { "frame": { "enabled": true } }
    }
  }
}
```

With width/height zero, an 800x600 PCX stays 800x600; no 16:9 area is
forced. Larger PCXs are fitted into the available window. Explicit dimensions
set the canvas; the picture is centered and scaled proportionally to fit it,
with HD Mod's existing texture in any unused area. The plugin does not fill
the screen with black. The default native-size path does not resize.
For example, 800x600 in a 1067x600 canvas remains 800x600 over HD's background.
The former `background.widescreen.enabled` test switch is removed.

Both frames use `H3LoadedPcx16::SimpleFrameRegion`, which loads HD Mod's
`hd_fr_*.bmp` assets. Background/frame settings apply on full redraw;
restart the menu/game after changing panel frame settings.

## Rendering

Main-menu and new/load-menu Show/Redraw hooks render the background before
dialog items. A scoped Show context handles the first draw before AddDialog
links the menu into `WindowManager::lastDlg`. Partial item redraws do not clear
the background. The API panel uses the same configured/asset-derived bounds; background
snapshots are refreshed for each menu, including the campaign-set selector.

When the background must shrink to fit the window or its four-pixel frame,
the plugin first renders the asset through ERA into a temporary pixel buffer.
This supports PNG-backed PCX stubs, whose own pixel buffer does not contain
the PNG. Resizing runs once and is cached across menu-page redraws; the cache
is rebuilt for a different source, size or color mode, and cleared on language
reload or leaving the main menu. Native-size images keep ERA's direct drawing.
The bicubic fit precomputes axis coefficients and reuses four filtered rows,
avoiding repeated kernel evaluation for every pixel. Cache preparation logs
separate PNG/PCX draw and resize times in `Debug/Era/log.txt`; cached redraws
do not emit timings or run the resizer.
The first full redraw logs dimensions to `Debug/Era/log.txt`.

See `HD_RESEARCH.md` for addresses and the HD cache investigation. Rendering
without stretching was verified in-game by the user before consolidation;
the consolidated binary still needs an in-game check.
