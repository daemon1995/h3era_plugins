#pragma once
#include "ObjectGeometry.h"
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
// Install native object/minimap hooks. Failure leaves the original renderer active.
bool InstallHooks();
// Discover and hook a compatible HD_WOG module; incompatible builds keep native HD markers.
void EnsureHdMinimapHooks();

// The native tile flags describe the whole tile. Check the mask of this DEF piece
// as well, since an overlapping sprite can have the same type as the actual object.
inline bool IsMinimapObjectCell(int actualType, int drawnType, unsigned char mirror, unsigned char access,
                               unsigned char tileId, std::uint64_t passability, std::uint64_t entrances)
{
    const int column = tileId & 15, row = tileId >> 4;
    if (actualType != drawnType || column >= geometry::MASK_WIDTH || row >= geometry::MASK_HEIGHT)
        return false;
    if (access & 0x10)
        return geometry::NativeMaskCell(entrances, column, row);
    return !(mirror & 0x40) && !geometry::NativeMaskCell(passability, column, row);
}
}

// Read-only bridge to map and ownership state owned by the API component in dllmain.cpp.
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
