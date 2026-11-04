#include "../../include/Plugin.h"
#include <cstddef>

namespace gbf
{
namespace summon
{
namespace
{
static_assert(sizeof(h3::H3CombatCreature) == 0x548, "CombatCreature ABI");
static_assert(offsetof(h3::H3CombatCreature, info) == 0x74, "CombatCreature information ABI");
static_assert(offsetof(h3::H3CombatCreature, type) == 0x34, "CombatCreature type ABI");
static_assert(offsetof(h3::H3CombatCreature, position) == 0x38, "CombatCreature position ABI");
static_assert(offsetof(h3::H3CombatCreature, numberAlive) == 0x4C, "CombatCreature size ABI");
static_assert(offsetof(h3::H3CombatCreature, secondHexOrientation) == 0x44, "CombatCreature direction ABI");

constexpr int MAX_BATTLE_HEX = 186; // 17 x 11: valid indices are [0,186]
constexpr int DRAGON_HEART_SPELL = 67;

inline bool Valid(int hex)
{
    return hex >= 0 && hex <= MAX_BATTLE_HEX;
}
inline bool Wide(int type)
{
    return type >= 0 && h3::H3CreatureInformation::Get()[type].doubleWide;
}
inline bool HexFree(int hex)
{
    if (!Valid(hex))
        return false;
    auto *battle = h3::H3CombatManager::Get();
    if (!battle)
        return false;
    auto *stack = battle->squares[hex].GetCreature();
    if (stack && stack->numberAlive > 0)
        return false;
    return !THISCALL_2(char, 0x4695F0, battle, hex);
}
inline int Neighbour(int center, int direction)
{
    if (!Valid(center) || direction < 0 || direction >= 6)
        return -1;
    const h3::H3AdjacentSquares &adjacent = h3::H3CombatManager::Get()->adjacentSquares[center];
    const int result = reinterpret_cast<const short *>(&adjacent)[direction];
    return HexFree(result) ? result : -1;
}
inline void Summon(int type, int count, int hex, int side, int redraw, int flags)
{
    CDECL_7(h3::H3CombatCreature *, 0x714D22, type, count, hex, side, -1, redraw, flags);
}
_LHF_(CloneWideMonster)
{
    auto *stack = *reinterpret_cast<h3::H3CombatCreature **>(c->ebp + 8);
    if (!stack || !stack->info.doubleWide || !Valid(c->ecx) || c->edx >= 6 || c->edx < 0)
        return EXEC_DEFAULT;
    int hex = c->ecx;
    if (stack->secondHexOrientation)
    {
        if (c->edx == 4)
            --hex;
    }
    else if (c->edx == 1)
        ++hex;
    c->eax =
        Valid(hex) ? reinterpret_cast<const short *>(&h3::H3CombatManager::Get()->adjacentSquares[hex])[c->edx] : -1;
    c->return_address = 0x5A709A;
    return NO_EXEC_DEFAULT;
}
// The order of adjacent squares follows the WoG side/direction convention.
constexpr int FIRST[2][8] = {{1, 0, 2, 5, 3, 5, 3, 4}, {4, 5, 3, 0, 2, 0, 2, 1}};
constexpr int SECOND[2][8] = {{1, 1, 1, 1, 1, 4, 4, 4}, {4, 4, 4, 4, 4, 1, 1, 1}};
_LHF_(WogCloneSummon)
{
    auto *stack = reinterpret_cast<h3::H3CombatCreature *>(c->edx);
    if (!stack || stack->side < 0 || stack->side > 1)
        return EXEC_DEFAULT;
    const int type = h3::IntAt(c->ebp - 0x58);
    const int count = h3::IntAt(c->ebp - 0x34);
    const int side = stack->side;
    const int redraw = h3::IntAt(c->ebp - 0x68);
    const int flags = h3::IntAt(c->ebp - 0x4C);
    int primary = stack->position;
    int second = primary;
    if (stack->info.doubleWide)
        second += stack->secondHexOrientation ? 1 : -1;
    int pos1 = -1, pos2 = -1;
    if (Valid(primary) && type >= 0 && count > 0)
    {
        for (int i = 0; i != 8; ++i)
        {
            const int near1 = FIRST[side][i];
            int base = primary;
            if ((!side && near1 <= 2) || (side && near1 >= 3))
                base = second;
            pos1 = Neighbour(base, near1);
            pos2 = pos1;
            if (pos1 != -1 && Wide(type))
            {
                const int near2 = SECOND[side][i];
                pos2 = Neighbour(pos1, near2);
                if ((!side && near2 == 4) || (side && near2 == 1))
                {
                    const int old = pos1;
                    pos1 = pos2;
                    pos2 = old;
                }
            }
            if (pos1 >= 0 && pos2 >= 0)
                break;
        }
        if (pos1 >= 0 && pos2 >= 0)
            Summon(type, count, pos1, side, redraw, flags);
    }
    c->return_address = 0x71E3F5;
    return NO_EXEC_DEFAULT;
}
_LHF_(DragonHeartSummon)
{
    const int side = h3::IntAt(c->ebp - 0x18);
    if (side < 0 || side > 1)
        return EXEC_DEFAULT;

    auto* battle = h3::H3CombatManager::Get();
    const int type = h3::IntAt(c->ebp - 0x1C);
    const int count = h3::IntAt(c->ebp - 0xC);
    const auto previousMonster = battle->summonedMonster[side];
    const int previousSide = battle->currentActiveSide;
    int &spellEffect = h3::H3Spell::Get()[DRAGON_HEART_SPELL].baseValue[0];
    const int previousEffect = spellEffect;
    battle->currentActiveSide = side;
    spellEffect = 1;
    THISCALL_5(void, 0x5A7390, battle, DRAGON_HEART_SPELL, type, count, 0);
    spellEffect = previousEffect;
    battle->summonedMonster[side] = previousMonster;
    battle->currentActiveSide = previousSide;
    c->return_address = 0x767B77;
    return NO_EXEC_DEFAULT;
}
} // namespace
void Install(PatcherInstance *pi)
{
    // фикс заклинания клон при использовании на двухклеточных существах SOD
    pi->WriteLoHook(0x5A7095, CloneWideMonster);
    // фикс вызова клонов ВОГом от опыта монстров
    pi->WriteLoHook(0x71E031, WogCloneSummon);
    // @ Archer30
    // Here we just recreate the summoning behaviour with the Summon Elemental way to avoid the wrong position to summon
    // Discussion: http://wforum.heroes35.net/showthread.php?tid=4218&pid=138936#pid138936
    pi->WriteLoHook(0x7679F5, DragonHeartSummon);
}
} // namespace summon
void InstallSummon(PatcherInstance *pi)
{
    summon::Install(pi);
}
} // namespace gbf
