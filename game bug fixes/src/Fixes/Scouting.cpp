#include "../../include/Plugin.h"
namespace gbf
{
namespace scouting
{
namespace
{
// H3API's H3Hero::GetScoutingRadius accounts for Scouting and artifacts.
void OpenArea(const h3::H3Hero *hero)
{
    if (!hero || hero->owner < 0 || hero->owner > 7 || hero->x < 0 || hero->y < 0 || hero->z < 0 || hero->x > 255 ||
        hero->y > 255 || hero->z > 8)
        return;
    // H3Main::OpenArea native signature; final argument is unused by this call.
    THISCALL_7(bool, 0x49CDD0, h3::H3Main::Get(), hero->x, hero->y, hero->z, hero->owner, hero->GetScoutingRadius(), 0);
}
_LHF_(AfterHeroAppears)
{
    OpenArea(reinterpret_cast<const h3::H3Hero *>(c->esi));
    return EXEC_DEFAULT;
}
void __stdcall OnLevelUp(HiHook *hook, h3::H3Hero *hero)
{
    const int oldLevel = hero ? hero->level : 0;
    THISCALL_1(void, hook->GetDefaultFunc(), hero);
    if (hero && hero->level > oldLevel)
        OpenArea(hero);
}
void __stdcall OnCombatWinner(HiHook *hook, h3::H3CombatManager *battle, int side)
{
    THISCALL_2(void, hook->GetDefaultFunc(), battle, side);
}
int __stdcall OnSecondarySkill(HiHook *hook, h3::H3Hero *hero, int skill, int value)
{
    const int result = THISCALL_3(int, hook->GetDefaultFunc(), hero, skill, value);
    if (result && skill == h3::NH3Skills::NSecondarySkills::SCOUTING)
        OpenArea(hero);
    return result;
}
bool __stdcall OnEquip(HiHook *hook, h3::H3Hero *hero, h3::H3Artifact *artifact, int slot)
{
    const bool result = THISCALL_3(bool, hook->GetDefaultFunc(), hero, artifact, slot);
    if (result && artifact &&
        (artifact->id == h3::NH3Artifacts::NArtifacts::SPECULUM ||
         artifact->id == h3::NH3Artifacts::NArtifacts::SPYGLASS))
        OpenArea(hero);
    return result;
}
_LHF_(AfterBattleStart)
{
    // In the original source, the second hero was accidentally ignored.
    const auto *hero1 = *reinterpret_cast<const h3::H3Hero *const *>(c->ebp - 0x24);
    const auto *hero2 = *reinterpret_cast<const h3::H3Hero *const *>(c->ebp - 0x28);
    OpenArea(hero1);
    if (hero2 && hero2 != hero1)
        OpenArea(hero2);
    return EXEC_DEFAULT;
}
} // namespace
} // namespace scouting
void InstallScouting(PatcherInstance *pi)
{
    // Исправление бага с неоткрывающейся картой при получении разведки, выкупе героя из таверны и пр.
    // from external tavern + from castle
    pi->WriteLoHook(0x4D7C4B, scouting::AfterHeroAppears);
    // from prison
    pi->WriteLoHook(0x4A3D9A, scouting::AfterHeroAppears);
    // Обертка над H3Hero::CheckLevelUps
    pi->WriteHiHook(0x4DA990, SPLICE_, EXTENDED_, THISCALL_, scouting::OnLevelUp);
    // The original winner hook only toggled a flag used by the old module.
    // It was removed because it has no observable effect in this port.
    // on hero get sec. skill ( check on scouting )
    pi->WriteHiHook(0x4E2540, SPLICE_, EXTENDED_, THISCALL_, scouting::OnSecondarySkill);
    // Обертка над H3Hero::EquipArtifact
    pi->WriteHiHook(0x4E2C70, SPLICE_, EXTENDED_, THISCALL_, scouting::OnEquip);
    // H3AdventureManager::StartBattle
    pi->WriteLoHook(0x4AD4CC, scouting::AfterBattleStart);
}
} // namespace gbf
