#include "../../include/Plugin.h"
namespace gbf
{
namespace
{
// Lighthouse bonus must belong to the same player as the visiting hero.
// by Ben80 ////// //////////////////////////////////////////////////////////////////////////////////
// The Castle's Lighthouse building bonus
_LHF_(CastleOwnerCheck)
{
    const auto *town = reinterpret_cast<const h3::H3Town *>(c->ecx);
    const auto *hero = *reinterpret_cast<const h3::H3Hero *const *>(c->ebp - 4);
    if (town && hero && town->owner != hero->owner)
    {
        c->return_address = 0x4E4D6C;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
// The native code addresses creature information at stack + 0x74;
// [EAX] points at the info flags (+0x84), so EAX-0x84 is H3CombatCreature.
_LHF_(EarthquakeBug)
{
    const auto *creature = reinterpret_cast<const h3::H3CombatCreature *>(c->eax - 0x84);
    return creature && creature->sideIndex == 0 ? NO_EXEC_DEFAULT : EXEC_DEFAULT;
}
} // namespace
void InstallBen(PatcherInstance *pi)
{
    // Tower damage, Fortress bonuses, map-generator Witch Huts.
    // Arrow towers damage bug fix
    // for cases:
    // Armorer sec.skill, Stoning of creature, Air shield spell
    pi->WriteByte(0x41E3A4, 0x4D);
    pi->WriteByte(0x41E4E0, 0x4D);
    pi->WriteByte(0x465944, 0x4D);
    // fix Fortress attack and defense bonuses for defended Hero
    pi->WriteByte(0x4639FE, 0x77);
    pi->WriteByte(0x463A31, 0x76);
    // The Castle's Lighthouse building bonus
    // is now only applied to the Castle's owner
    pi->WriteLoHook(0x4E4D40, CastleOwnerCheck);
    // Earthquake Bug will no longer kill creatures and end battle
    pi->WriteLoHook(0x465656, EarthquakeBug);
    // fix Witch Huts for random maps
    // (it gave only secondary skills with number 15 or lesser),
    pi->WriteDword(0x534624, 0x0FFFEFBF);
}
} // namespace gbf
