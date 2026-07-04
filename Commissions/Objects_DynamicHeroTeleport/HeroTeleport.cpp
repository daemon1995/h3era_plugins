#pragma comment(linker, "/EXPORT:DisplayHeroTeleporter=_DisplayHeroTeleporter@20")

#include "HeroTeleport.h"
DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int objectType, const int objectSubtype,
                                              const int *objectIndexes, const int arraySize);

int HeroTeleport::objectType = eObject::NO_OBJ;
int HeroTeleport::objectSubtype = 0;
H3LoadedPcx16 *HeroTeleport::defaultPicture = nullptr;

void TeleportDlg::RedrawDestinationPanels(const int index, const BOOL redrawDlg)
{
    const int lastIndex = index + MAX_DESTINATIONS;
    const auto size = heroTeleports.size();
    selectionFrame->Hide();
    for (size_t i = 0; i < MAX_DESTINATIONS; i++)
    {
        auto &panel = destinationPanels[i];

        HeroTeleport *destination = nullptr;
        if (lastIndex < size)
        {
            destination = &heroTeleports[index + i];
            if (index + i == selectedIndex)
            {
                selectionFrame->SetX(panel.icon->GetX());
                selectionFrame->SetY(panel.icon->GetY());
            }
        }
        panel.SetTarget(destination, redrawDlg);
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

void TeleportDlg::CreateDestinationPanels()
{

    constexpr int scrollbarHeight = MAX_DESTINATIONS * DESTINATION_PANEL_HEIGHT;
    const INT ticksCount = heroTeleports.size() - MAX_DESTINATIONS;

    int y = 162;
    constexpr int x = 32;

    scrollBar = CreateScrollbar(widthDlg - 50, y, 16, scrollbarHeight, 3 * (MAX_DESTINATIONS + 1),
                                ticksCount > 0 ? ticksCount : 0, ScrollBarHandler);

    if (scrollBar->GetTicksCount() <= 0)
    {
        scrollBar->Disable();
    }

    H3RGB565 color = H3RGB565::Gold();
    selectionFrame = H3DlgFrame::Create(x, y, widthDlg - x * 2, DESTINATION_PANEL_HEIGHT, color);
    selectionFrame->HideDeactivate();

    const size_t length = heroTeleports.size();
    destinationPanels.resize(MAX_DESTINATIONS);

    for (size_t i = 0; i < MAX_DESTINATIONS; i++)
    {
        auto &panel = destinationPanels[i];

        panel.icon = CreatePcx16(x, y, 48, 32, 1 + i * 3, nullptr);
        panel.text = CreateText(x + 48, y, widthDlg - x * 2 - 48, DESTINATION_PANEL_HEIGHT - 4, h3_NullString,
                                NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, 2 + i * 3);
        y += DESTINATION_PANEL_HEIGHT;

        HeroTeleport *destination = nullptr;
        if (i < length)
        {
            destination = &heroTeleports[i];
        }
        panel.SetTarget(destination, false);
    }
}

void TeleportDlg::CreateMiniMap(const int index)
{




}

BOOL TeleportDlg::DialogProc(H3Msg &msg)
{

    return 0;
}

DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int objectType, const int objectSubtype,
                                              const int *objectIndexes, const int arraySize)
{
    auto hero = P_Game->GetHero(heroId);
    if (!hero || !objectIndexes || arraySize < 1)
        return -1;
    std::vector<HeroTeleport> heroTeleports;
    heroTeleports.reserve(arraySize);

    HeroTeleport::objectType = objectType;
    HeroTeleport::objectSubtype = objectSubtype;
    auto &pic = HeroTeleport::defaultPicture;
    pic = H3LoadedPcx16::Create(48, 32);
    libc::memset(pic->buffer, 0, pic->buffSize);

    auto pcx = H3LoadedPcx::Load("HPSXXX.PCX");
    if (pcx)
    {
        pcx->DrawToPcx16(pic, 0, 0, 1);
        pcx->Dereference();
    }

    for (size_t i = 0; i < arraySize; i++)
    {
        const int index = objectIndexes[i];
        if (index >= 0)
        {
            heroTeleports.emplace_back(HeroTeleport(index));
            auto &teleport = heroTeleports.back();
            teleport.CreatePcx16();
        }
    }

    int result = -1;

    if (heroTeleports.size())
    {

        TeleportDlg dlg(hero, heroTeleports);
        dlg.Start();

        result = dlg.selectedIndex;
    }

    for (auto &heroTeleport : heroTeleports)
    {
        if (heroTeleport.picture)
        {
            heroTeleport.picture->Destroy();
            heroTeleport.picture = nullptr;
        }
    }

    if (pic)
    {
        pic->Destroy();
        pic = nullptr;
    }

    return result;
}
