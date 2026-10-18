#pragma once
#include <windows.h>

namespace erm_functions
{
constexpr const char *PLUGIN_NAME = "API_ErmFunctions.era";
constexpr int NO_OWNER = -1;
constexpr int INVALID_OBJECT = -2;
constexpr int ANY_SUBTYPE = -1;

// All exports use __stdcall and undecorated names in both build configurations.
// Coordinates are the object's entrance (yellow tile), including when a hero stands there.
// Set/Register return 1 on success, 0 on invalid arguments or unavailable map/API.
using SetObjectOwner = int(__stdcall *)(int x, int y, int z, int owner);
using RegisterObjectType = int(__stdcall *)(int type, int subtype, int drawOnMinimap);
// Returns -1 for neutral, 0..7 for a player, -2 for an invalid/unregistered object.
using GetObjectOwner = int(__stdcall *)(int x, int y, int z);

// Geometry API v1 preserves blocked cells and every entrance after aligning the primary entrance.
using GetObjectReplacementApiVersion = int(__stdcall *)();
// Writes 15 integers; layout is documented in API_ErmFunctions/OBJECT_REPLACEMENT.md.
using GetObjectGeometry = int(__stdcall *)(int x, int y, int z, int *output, int capacity);
// Returns an opaque 1-based prototype token, or 0 if no variant fits. Tokens are map-local.
using FindObjectReplacement = int(__stdcall *)(int x, int y, int z, int type, int subtype,
                                               int terrain, int approachMask);
// Revalidates the token and geometry immediately before placement. Returns 1 on success.
using ReplaceObject = int(__stdcall *)(int x, int y, int z, int type, int subtype,
                                       int terrain, int approachMask, int prototype);

// Strict signed 32-bit JSON integers; missing/invalid values return the supplied default.
using ReadJsonInt = int(__stdcall *)(const char *key, int defaultValue);
// Accepts a scalar or contiguous flattened array. Null output/capacity 0 queries count.
// With a buffer, returns min(count, capacity). Invalid data returns 0 without writing.
// Caller owns output; these readers do not cache language data across reloads.
using ReadJsonIntArray = int(__stdcall *)(const char *key, int *output, int capacity);

// Record only; does not execute the callback now. undoFunction == 0 means no undo callback.
// Replayed callbacks receive the copied payload in x1..x16. They are not saved or networked.
using RegisterReplayEvent = int(__stdcall *)(int replayFunction, int undoFunction,
                                            int x1, int x2, int x3, int x4);
using RegisterReplayEventArgs = int(__stdcall *)(int replayFunction, int undoFunction,
                                                const int *args, int argCount);
}
