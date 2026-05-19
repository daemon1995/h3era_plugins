#pragma comment(linker, "/EXPORT:DisplayHeroTeleporter=_DisplayHeroTeleporter@20")

#include "HeroTeleport.h"
DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int objectType, const int objectSubtype,
                                              const int *objectIndexes, const int arraySize);

int HeroTeleport::objectType = eObject::NO_OBJ;
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
   // teleportDlg->RedrawDestinationPanels(itemID, true);
}

void TeleportDlg::CreateItems()
{
    constexpr int scrollbarHeight = MAX_DESTINATIONS * DESTINATION_PANEL_HEIGHT;
    const BOOL ticksCount = destinations.size() > MAX_DESTINATIONS;
    scrollBar = CreateScrollbar(widthDlg - 50, 60, 16, scrollbarHeight, 12, ticksCount, ScrollBarHandler);
    const size_t length = std::min(destinations.size(), MAX_DESTINATIONS);
    destinationPanels.resize(length);

    for (size_t i = 0; i < length; i++)
    {
        auto &panel = destinationPanels[i];
        auto &destination = destinations[i];

        const int x = 20;
        const int y = 60 + i * DESTINATION_PANEL_HEIGHT;
        panel.icon = CreateDef(x, y, 1 + i * 3, "smalres.def", 0);
        panel.text = CreateText(x + 20, y, widthDlg - 80, DESTINATION_PANEL_HEIGHT - 4, h3_NullString,
                                NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, 2 + i * 3);
        panel.SetTarget(&destination, false);
    }

    CreateOKButton();
    CreateCancelButton();
}

DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int objectType, const int objectSubtype,
                                              const int *objectIndexes, const int arraySize)
{
    auto hero = P_Game->GetHero(heroId);
    if (!hero || !objectIndexes || arraySize < 1)
        return -1;
    std::vector<HeroTeleport> destinations;
    destinations.reserve(arraySize);

    HeroTeleport::objectType = objectType;
    HeroTeleport::objectSubtype = objectSubtype;

    for (size_t i = 0; i < arraySize; i++)
    {
        const int index = objectIndexes[i];
        if (index >= 0)
        {
            HeroTeleport teleport(index);
            destinations.emplace_back(teleport);
        }
    }
    if (destinations.size())
    {

        TeleportDlg dlg(hero, destinations);
        dlg.Start();

        return dlg.selectedIndex;
    }
    return -1;
}
