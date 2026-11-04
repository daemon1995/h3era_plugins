// Fail fast if an external property sheet or compiler flag changed the game ABI.
// H3API deliberately mixes #pragma pack(push, 1) and pack(push, 4).
// Do not disable H3API assertions with _H3API_NO_VALIDATION_.
#include "../include/Plugin.h"
#include "../include/WoGCrExpo.h"
#include "../WoG_NPC.h"
#include <cstddef>

// This declaration is intentionally OUTSIDE #pragma pack: validate the /Zp1
// default applied by the original Game Bug Fixes project and this project.
struct DefaultPackingProbe { char first; int second; };
static_assert(sizeof(DefaultPackingProbe) == 5 && offsetof(DefaultPackingProbe, second) == 1,
              "Project default packing must be /Zp1 (StructMemberAlignment=1Byte)");

static_assert(sizeof(void*) == 4, "Game Bug Fixes requires MSVC Win32 / x86");
static_assert(sizeof(h3::H3Hero) == 0x492, "H3API Hero size mismatch");
static_assert(alignof(h3::H3Hero) == 1, "H3API Hero must have 1-byte packing");
static_assert(sizeof(h3::H3CombatCreature) == 0x548, "H3API combat stack size mismatch");
static_assert(offsetof(h3::H3CombatCreature, info) == 0x74, "Combat stack info field offset mismatch");
static_assert(sizeof(h3::H3RGB888) == 3, "Packed 24-bit RGB image pixel size mismatch");
static_assert(alignof(h3::H3RGB888) == 1, "Packed RGB layout mismatch");
static_assert(sizeof(h3::H3Position) == 4, "Packed coordinate size mismatch");
static_assert(sizeof(WoG::CrExpModStr) == 17, "Creature experience bonus entries must be 17 bytes");
static_assert(sizeof(WoG::CrExpo) == 16, "Creature experience record size mismatch");
static_assert(sizeof(WoG::NPC) == 296, "Commander record size mismatch");
