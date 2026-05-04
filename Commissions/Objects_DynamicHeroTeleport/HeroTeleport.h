#pragma once
#include "framework.h"
struct HeroTeleport
{
    static eObject objectType;
    static int objectSubtype;
    H3Position position;
    H3PlayersBitfield playerBitfield;
    H3String displayedName;
};

class TeleportDlg : public H3Dlg
{
    static constexpr int MAX_DESTINATIONS = 9;
    static constexpr int DESTINATION_PANEL_HEIGHT = 40;
    struct DestinationPanel
    {
        H3DlgDef *icon = nullptr;
        H3DlgText *text = nullptr;
        HeroTeleport *target = nullptr;
    };

    std::vector<DestinationPanel> destinationPanels;
    std::vector<HeroTeleport> destinations;

  public:
    TeleportDlg() : H3Dlg(306, 469, -1, -1, TRUE, TRUE)
    {
    }

    virtual ~TeleportDlg()
    {
    }

  protected:
    void RedrawDestinationPanels(const int index, const BOOL redrawDlg);
    static void __fastcall ScrollBarHandler(INT32 itemID, H3BaseDlg *dlg);
    void CreateItems();
};
