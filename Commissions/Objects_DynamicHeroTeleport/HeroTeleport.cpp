#pragma comment(linker, "/EXPORT:DisplayHeroTeleporter=_DisplayHeroTeleporter@8")

#include "HeroTeleport.h"
DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int someOtherParam);

eObject HeroTeleport::objectType = eObject::NO_OBJ;
int HeroTeleport::objectSubtype = 0;

void TeleportDlg::RedrawDestinationPanels(const int index, const BOOL redrawDlg)
{
    const int lastIndex = index + MAX_DESTINATIONS;
    const auto size = destinations.size();
    for (size_t i = 0; i < size && i < MAX_DESTINATIONS; i++)
    {
        auto &panel = destinationPanels[i];
        auto &destination = destinations[index + i];
        panel.SetTarget(&destination, redrawDlg);
    }
}

void __fastcall TeleportDlg::ScrollBarHandler(INT32 itemID, H3BaseDlg *dlg)
{
    TeleportDlg *teleportDlg = static_cast<TeleportDlg *>(dlg);
    auto scrollBar = teleportDlg->scrollBar;
    const int scrollPos = scrollBar->GetTick();
    scrollBar->Draw();
    scrollBar->Refresh();
    teleportDlg->RedrawDestinationPanels(itemID, true);
}

void TeleportDlg::CreateItems()
{

    constexpr int scrollbarHeight = MAX_DESTINATIONS * DESTINATION_PANEL_HEIGHT;
    const BOOL ticksCount = destinations.size() > MAX_DESTINATIONS;
    scrollBar = CreateScrollbar(widthDlg - 50, 60, 16, scrollbarHeight, 12, ticksCount, ScrollBarHandler);

    CreateOKButton();
    CreateCancelButton();
}

DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int someOtherParam)
{
    auto hero = P_Game->GetHero(heroId);
    if (!hero)
    {
        return false;
    }
    TeleportDlg dlg(hero);
    dlg.Start();
    return true;
}
