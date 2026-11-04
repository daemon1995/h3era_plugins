#include "../../include/Plugin.h"
namespace gbf
{
namespace
{
_LHF_(RefreshRoute)
{
    P_AdventureManager->MakeHeroPath();
    // Old lastHoverX at offset 0xEC is H3API previousMousePosition.x.
    P_AdventureManager->previousMousePosition.x = -1;
    THISCALL_1(void, 0x4ECCD0, h3::H3InputManager::Get());
    return EXEC_DEFAULT;
}
_LHF_(SkipHorseButtonReset)
{
    if (*reinterpret_cast<const int *>(c->ebp + 0x10))
        return EXEC_DEFAULT;
    c->return_address += 14;
    return NO_EXEC_DEFAULT;
}
} // namespace
void InstallHeroMovement(PatcherInstance *pi)
{
    // Обновление стрелок маршрута героя.
    pi->WriteLoHook(0x4082CD, RefreshRoute);
    pi->WriteLoHook(0x409E2F, RefreshRoute);
    pi->WriteLoHook(0x4AA7E5, RefreshRoute);
    pi->WriteLoHook(0x41C5F2, RefreshRoute);
    // AdvMgr_RouteUpdate 00418D30
    // Не теребить кнопку лошади
    pi->WriteLoHook(0x418D8D, SkipHorseButtonReset);
    pi->WriteLoHook(0x418E1D, SkipHorseButtonReset);
    pi->WriteLoHook(0x419153, SkipHorseButtonReset);
}
} // namespace gbf
