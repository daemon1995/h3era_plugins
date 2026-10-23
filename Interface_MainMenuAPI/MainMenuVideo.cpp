#include "framework.h"

namespace mainmenu
{
static int videoX = 0;
static int videoY = 0;

static _LHF_(MainMenu_OpenVideo)
{
    if (c->ecx == 33)
    {
        c->edx = videoX;
        IntAt(c->ebp + 0x8) = videoY;
    }
    return EXEC_DEFAULT;
}

void InstallVideoPositioning()
{
    if (!config::Enabled("era.main_menu.video.position.enabled", true))
        return;
    if (H3GameWidth::Get() < 808 && H3GameHeight::Get() < 608)
        return;
    // Preserve the compatibility check from Graphics_MainMenuVideoPositioning:
    // the adjustment only applies when the video-menu patch is present.
    if (WordAt(0x597876) == 0x507C)
        return;

    videoX = config::Int("era.main_menu.video.position.x", 0);
    videoY = config::Int("era.main_menu.video.position.y", 0);
    _PI->WriteLoHook(0x597876, MainMenu_OpenVideo);
}
} // namespace mainmenu
