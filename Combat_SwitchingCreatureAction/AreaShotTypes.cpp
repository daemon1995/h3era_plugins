#include "ActionSwitcher.h"
#include <array>
#include <limits>

namespace switcher
{
namespace aoe
{
namespace
{
bool typesLoaded = false;

AreaShotType ValidType(INT value)
{
    return value >= 0 && value <= 3 ? static_cast<AreaShotType>(value) : AreaShotType::NONE;
}

AreaShotType ReadConfiguredType(INT creature)
{
    char key[80];
    sprintf(key, "jsMod.shotAtArea.creatures.%d.type", creature);
    bool found = false;
    const INT value = EraJS::readInt(key, found);
    return found ? ValidType(value) : AreaShotType::NONE;
}

void __stdcall LoadTypes(Era::TEvent *)
{
    ResetTargeting(P_CombatManager->Get());
    for (INT creature = 0; creature < static_cast<INT>(P_CreatureCount); ++creature)
    {
        const AreaShotType type = ReadConfiguredType(creature);
        char key[80];
        sprintf(key, "js_aimedShotType_%d", creature);
        Era::SetAssocVarIntValue(key, static_cast<INT>(type));
        // As in the supplied source, the JSON grants the area flag to shooters.
        if (type != AreaShotType::NONE && P_CreatureInformation[creature].shooter)
            P_CreatureInformation[creature].fireballAttack = true;
    }
    typesLoaded = true;
}

bool AtCenter(const H3CombatCreature *target, INT hex)
{
    return target->position == hex || (target->info.doubleWide && target->GetSecondSquare() == hex);
}

bool ValidTarget(H3CombatManager *manager, H3CombatCreature *shooter, INT hex, bool bow)
{
    return hex >= 0 && hex < 187 && hex % 17 != 0 && hex % 17 != 16 && !AtCenter(shooter, hex) &&
           (bow ||
            (!THISCALL_3(BOOL8, 0x4670A0, manager, shooter->position, hex) &&
             (!shooter->info.doubleWide || !THISCALL_3(BOOL8, 0x4670A0, manager, shooter->GetSecondSquare(), hex))));
}

// The native valuation already accounts for friendly fire and combat AI weights.
// Override its immunity branch for all three configured types, retaining the
// direct-hit exception for fire-immune/nonliving creatures at the center.
_LHF_(EvaluateAreaTarget)
{
    if (!ActionSwitcher::Get().IsEnabled())
        return EXEC_DEFAULT;
    const auto shooter = reinterpret_cast<H3CombatCreature *>(c->edi);
    const auto target = reinterpret_cast<H3CombatCreature *>(c->esi);
    const AreaShotType type = GetAreaShotType(shooter);
    if (type == AreaShotType::NONE || !target)
        return EXEC_DEFAULT;
    const INT hex = IntAt(c->ebp - 0x14);
    c->return_address = AtCenter(target, hex) || CanDealAreaDamage(target, type) ? 0x41EFC4 : 0x41EFEF;
    return NO_EXEC_DEFAULT;
}

INT __stdcall SelectAreaTarget(HiHook *hook, H3CombatManager *manager, H3CombatCreature *shooter, DWORD *data,
                               INT *weight)
{
    if (!manager || !data || !weight || !IsAvailable(shooter))
        return THISCALL_4(INT, hook->GetDefaultFunc(), manager, shooter, data, weight);

    const H3Hero *hero = shooter->GetOwner();
    const bool bow = hero && hero->WearsArtifact(eArtifact::BOW_OF_THE_SHARPSHOOTER);
    const INT ourGroup = IntAt(reinterpret_cast<INT>(data) + 0x20);
    std::array<BYTE, sizeof(manager->massSpellTarget)> savedTargets;
    memcpy(savedTargets.data(), manager->massSpellTarget, savedTargets.size());
    INT bestHex = -1, bestWeight = 0, bestDistance = 187, bestRank = -1;
    for (INT hex = 0; hex < 187; ++hex)
    {
        if (!ValidTarget(manager, shooter, hex, bow))
            continue;
        const INT value = FASTCALL_4(INT, 0x41EF20, shooter, hex, ourGroup, data);
        if (value <= 0)
            continue;
        INT distance = FASTCALL_2(INT, 0x469250, shooter->position, hex);
        if (shooter->info.doubleWide)
            distance = (std::min)(distance, FASTCALL_2(INT, 0x469250, shooter->GetSecondSquare(), hex));
        const H3CombatCreature *target = manager->squares[hex].GetCreature();
        const INT rank = target ? (target->position == hex ? 2 : 1) : 0;
        if (value > bestWeight ||
            (value == bestWeight && (distance < bestDistance || (distance == bestDistance && rank > bestRank))))
        {
            bestHex = hex;
            bestWeight = value;
            bestDistance = distance;
            bestRank = rank;
        }
    }
    memcpy(manager->massSpellTarget, savedTargets.data(), savedTargets.size());
    *weight = bestWeight;
    return bestHex;
}

_LHF_(DispatchAreaDamage)
{
    auto shooter = reinterpret_cast<H3CombatCreature *>(c->esi);
    const AreaShotType type = GetAreaShotType(shooter);
    if (!ActionSwitcher::Get().IsEnabled() || !shooter || !shooter->info.fireballAttack || type == AreaShotType::NONE)
        return EXEC_DEFAULT;
    // This point follows projectile drawing and the single native ammo decrement.
    c->eax = shooter->type;

    if (type == AreaShotType::FIRE)
        c->return_address = 0x43F735;
    else if (type == AreaShotType::DEATH)
        c->return_address = 0x43FB27;
    else
    {
        IntAt(c->ebp - 0x14) = -1;
        c->return_address = 0x43F870;
    }
    return NO_EXEC_DEFAULT;
}
} // namespace

AreaShotType GetAreaShotType(const H3CombatCreature *stack)
{
    if (!stack)
        return AreaShotType::NONE;
    if (!typesLoaded)
        return ReadConfiguredType(stack->type);
    char key[80];
    sprintf(key, "js_aimedShotType_%d", stack->type);
    return ValidType(Era::GetAssocVarIntValue(key));
}

bool CanDealAreaDamage(const H3CombatCreature *target, AreaShotType type)
{
    return target && target->numberAlive > 0 && type != AreaShotType::NONE &&
           (type != AreaShotType::FIRE || !target->info.fireImmunity) &&
           (type != AreaShotType::DEATH || target->info.alive);
}

void InstallTypes(PatcherInstance *patcher)
{
    Era::RegisterHandler(LoadTypes, "OnAfterErmInstructions");
    patcher->WriteLoHook(0x43F729, DispatchAreaDamage);
    patcher->WriteHiHook(0x41ED00, SPLICE_, EXTENDED_, THISCALL_, SelectAreaTarget);
    patcher->WriteLoHook(0x41EF93, EvaluateAreaTarget);
}
} // namespace aoe
} // namespace switcher
