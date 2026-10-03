#pragma once

#include <stdint.h>

// Binary-compatible with the API formerly published by Spells_Description.
namespace SpellDescriptionHint
{
static const char PATCHER_VARIABLE[] = "ERA.SpellsDescription.BattleHint.Api.1";
static const uint32_t MAGIC = 0x31484453u;
static const uint32_t ABI_VERSION = 0x00010000u;

enum HintKind
{
    HINT_SPELL = 1,
    HINT_RESURRECT_CREATURE = 2,
    HINT_SUMMON_DEMONS = 3
};
enum FormatResult
{
    FORMAT_INVALID = -1,
    FORMAT_DECLINED = 0,
    FORMAT_SUCCESS = 1
};

#pragma pack(push, 4)
struct RequestV1
{
    uint32_t structSize;
    uint32_t abiVersion;
    uint32_t hintKind;
    uint32_t flags;
    void *battleManager;
    void *casterStack;
    void *targetStack; // May be null for an area/global spell aimed at an empty hex.
    int32_t casterSide;
    int32_t targetHex;
    int32_t spellId;
    int32_t spellPower;
    int32_t spellMastery;
    char *output;
    uint32_t outputCapacity;
    uint32_t reserved[2];
};
// Call on the game's thread, for the current caster and its acting side.
// The input and output pointers must remain valid throughout the synchronous call.
typedef int32_t(__stdcall *FormatBattleHintProc)(const RequestV1 *request);
struct ApiV1
{
    uint32_t magic;
    uint32_t structSize;
    uint32_t abiVersion;
    FormatBattleHintProc formatBattleHint;
    uint32_t reserved[4];
};
#pragma pack(pop)

static_assert(sizeof(void *) == 4, "Spell Description hint API requires PE32");
static_assert(sizeof(RequestV1) == 64, "Unexpected RequestV1 layout");
static_assert(sizeof(ApiV1) == 32, "Unexpected ApiV1 layout");
}
