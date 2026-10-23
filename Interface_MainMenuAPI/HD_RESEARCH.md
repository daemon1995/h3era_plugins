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
