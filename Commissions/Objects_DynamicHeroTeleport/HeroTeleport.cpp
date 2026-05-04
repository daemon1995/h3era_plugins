#include "HeroTeleport.h"
eObject HeroTeleport::objectType = eObject::NO_OBJ;
int HeroTeleport::objectSubtype = 0;

void __fastcall TeleportDlg::ScrollBarHandler(INT32 itemID, H3BaseDlg* dlg)
{
	TeleportDlg* teleportDlg = static_cast<TeleportDlg*>(dlg);
	const int scrollPos = teleportDlg->scrollBar->GetScrollPos();
	teleportDlg->UpdateDestinations(scrollPos);
}

void TeleportDlg::CreateItems()
{


	constexpr int scrollbarHeight = MAX_DESTINATIONS * DESTINATION_PANEL_HEIGHT;
	const BOOL ticksCount = destinations.size() > MAX_DESTINATIONS;
	CreateScrollbar(widthDlg - 50, 60, 16, scrollbarHeight, 12,ticksCount, ScrollBarHandler, , );





    CreateOKButton();
    CreateCancelButton();
}
