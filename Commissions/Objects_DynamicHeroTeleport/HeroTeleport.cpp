#pragma comment(linker, "/EXPORT:DisplayHeroTeleporter=_DisplayHeroTeleporter@12")

#include "HeroTeleport.h"
DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int someOtherParam, const int arraySize);

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

inline static void TeleportHero(const H3Hero *hero, const H3Position &position)
{
    THISCALL_7(void, 0x041DAB0, P_AdventureManager->Get(), hero, position, P_Spell[eSpell::DIMENSION_DOOR].soundName,
               FALSE, TRUE, FALSE);
}

DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int someOtherParam, const int arraySize)
{
    auto hero = P_Game->GetHero(heroId);
    if (!hero || !someOtherParam || arraySize < 1)
        return false;

    TeleportDlg dlg(hero);
    dlg.resultDestination = H3Position(0, 0, 0);
    dlg.Start();



    H3Position heroPos = H3Position(hero->x, hero->y, hero->z);
    if (heroPos != dlg.resultDestination)
    {
        TeleportHero(hero, dlg.resultDestination);
    }
    return true;
}
