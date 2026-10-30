#pragma once
#include <cstdint>

namespace h3
{
struct H3MainSetup;
struct H3MapItem;
struct H3ObjectDraw;
struct H3ObjectAttributes;
}

namespace erm_functions
{
namespace rendering
{
bool InstallHooks();
void EnsureHdMinimapHooks();

// Same occupied-cell test as the native mine minimap branch. A DEF can extend onto
// walkable cells or cells belonging to another object; those are not owner markers.
inline bool IsMinimapObjectCell(int actualType, int drawnType, unsigned char mirror, unsigned char access)
{
    return actualType == drawnType && (!(mirror & 0x40) || (access & 0x10));
}
}

// Read-only bridge to the map and ownership state owned by dllmain's API component.
namespace runtime
{
h3::H3MainSetup *GetMap();
const bool *FindObjectType(int type, int subtype);
int ReadOwnerAtPosition(std::uint32_t position);
// Ownership/minimap keep the fast registered-type filter; glow resolves other types too.
bool ResolveDrawObject(h3::H3MainSetup &map, const h3::H3ObjectDraw &draw, int &x, int &y, int &z,
                       const h3::H3ObjectAttributes *&attributes, bool registeredOnly = true);
}
}
