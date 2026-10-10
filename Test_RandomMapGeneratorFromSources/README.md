# Test_RandomMapGeneratorFromSources

The HiHook at `0x58704B` replaces the `TSingleSelectionWindow::generateRandomMap`
call in `onWidgetDeselect` and dispatches to the native request-level generator
entry, `TRandomMapRequest::generate` at `0x0054C090`. It does not call the
high-level window method or hook the `TRmgGenerator::generate` method directly;
the request entry constructs/configures `TRmgGenerator`, runs its source
generation routine and writes the compressed map.

The request layout and enums are copied from the decompilation's
`include/rmg_request.h` and its small type dependencies under `rmg_include/`.
The call-site adapter mirrors `TSingleSelectionWindow::generateRandomMap` using
the recovered window and player offsets from `include/singleselectionwindow.h`.
It preserves its random option resolution, map-version mapping, player-seat and
town assignment, and `random_maps\\` output path. The native request constructor
is called at `0x0054BF00`; option rolls call the game's statically linked CRT
`_rand` at `0x00617E5C`, so they share the game RNG state. Progress is passed as
null, which the RMG source explicitly supports.

The generator implementation and its wider engine dependencies remain inside
the running game image. `TRandomMapRequest::generate` in `src/rmg.cpp` calls the
same in-process `TRmgGenerator` implementation whose constructor, terrain and
support bodies are in `src/rmg.cpp`, `src/rmg_terrain.cpp` and
`src/rmg_support.cpp`. Copying those translation units into a plugin would also
require the full reconstructed game runtime and its global object tables; this
project calls the executable's source-matched entry point instead.

The adapter returns success to the original click handler so its existing
success notification runs. The wrapper's progress UI and its three detailed
failure dialogs are bypassed; failure returns false to the click handler. As
with any fixed-address hook, this targets the English Complete 4.0 executable
at base `0x00400000`.
