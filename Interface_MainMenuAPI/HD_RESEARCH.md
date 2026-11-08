## HD Mod investigation

Local `HD_WOG.dll`, file version `0.0.3.36`, SHA256:
`AB601899F1862C7283C57A28101428FDF3040FE69B628C3620D418AA8FDA84A1`.
Addresses below are RVAs in that DLL (preferred image base `0x10000000`).

- `0x818A2..0x818CB` reads patcher variables `HD.Center.X` and
  `HD.Center.Y` into DLL globals at RVAs `0xFA4DC` and `0xFA4E4`.
- `0x89028..0x8904D` installs a call hook at game address `0x4FB961`.
  Its handler at RVA `0x609C0` adds these offsets to the base-dialog
  constructor's x/y arguments, preserves width/height and supplies flags
  `0x24`. The original constructor at `0x4FB930` requests 800x600.
- `0x8904F..0x89075` hooks the partial redraw call at game address
  `0x4FC195`. Handler RVA `0x60AD0` also adds the center offsets to x/y.
- HD stores a separate full-screen background with the nominal 800x600
  frame at RVA `0xFA4E8`, created at `0x6A459` and copied by its dialog-show
  handler at `0x6B48D..0x6B4CB`. That layer explains why drawing side strips
  after a menu callback did not remove the whole frame.
- Game MainLoop loads `Mainmenu.pcx` into `0x699564` at `0x4EEED7` and
  `Gamselbk.pcx` into `0x699568` at `0x4EEEFD`. The common menu background
  is drawn from the latter at `0x4EEF3F`; `Mainmenu.pcx` is separately used
  at x=460 in credits. The shared helper reads the live `Gamselbk` pointer
  each time and does not resize, replace or retain the game's resources.

The implementation uses the game's resource pointer and menu vtables,
without patching HD DLL addresses, its cache or global centering offsets.

Verified by static disassembly and a Release|Win32 build with `v141_xp`;
in-game visual verification is still required. When installing, replace
the old `Interface_MainMenuLogo.era` rather than loading both plugins.

## Credits background

The installed `credits_unlink.dll` (SHA256
`FE64E7D28A33407CBB011B9B24B5446A83FAE573D86B163323C0CEC0FC568284`)
hooks `0x4EF7EF`: its handler at RVA `0x1030` draws Gamselbk from source
(460, 0), size 340x600, through the existing call chain.

At the following ShowCredits call (`0x4EF7F4`), MainMenuAPI prepares the
fitted canvas but copies and presents only the native 340x600 strip at the
actual menu origin plus (460, 0). Pixels outside this region are retained.
ShowCredits saves a separate 328x580 scrolling
backing at `0x4EE536`; that call copies from the prepared canvas at the
actual menu origin plus (460, 10). This avoids capturing the old screen or
applying HD's source translation to an already positioned canvas. The
native scrolling/restore calls remain intact.

## Video presentation

The native video update routine at `0x597CD0` submits the decoded dirty region
through two `thiscall` calls to `H3WindowManager::H3Redraw` (`0x603190`):
`0x597DC1` for SMK and `0x597E95` for BIK. ECX is the window manager; the
four stack arguments are x, y, width and height. BIK offsets are already added
at this point; SMK obtains its destination rectangles through SmackToBufferRect.

Both call instructions and target prologues were checked in the local files:

- `h3era.exe`, SHA256 `451E208CB2985BCA75330F0E3C52F89F9219C0C3701BC6E1F354217890C36181`.
- `h3era HD.exe`, SHA256 `CA47B6406FB064AECACFD4620C3A0DE190A4A37D97A313D66E1F9AAF95A9D594`.

The supplied `video.asm` matches `Graphics_BikVideo/video.asm` (SHA256
`3B593F87C0288C6BB38E3E6D22D3558DD4279268FC26B84A1866412EA79D27E9`).
Its `recovered.inc` replaces codec/opening and frame paths while retaining
these common presentation calls. MainMenuAPI uses CALL hooks here, restores
overlapping added menu items, then forwards to the existing presentation chain.
The scrollable-text Draw method is a no-op; its separately initialized text
lines and scrollbar must also be restored.

A native x86 harness using the production implementation and patcher_x86.dll
passed 2000 frame submissions through both hooks, including stack balance,
panel children, clipping, modal/nested menus, lifecycle cleanup and hooks
installed before and after MainMenuAPI. This is an isolated hook/geometry
verification, not an in-game BIK/SMK playback or visual check.
