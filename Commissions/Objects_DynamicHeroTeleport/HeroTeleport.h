#pragma once
#include "framework.h"

struct HeroTeleport
{
    static int objectType;
    static int objectSubtype;
    static constexpr LPCSTR NAME_FORMAT = "mrart.teleport_names.%d";

  public:
    const int index;
    H3Position position;
    H3String displayedName;

  public:
    inline HeroTeleport(const int index) : index(index)
    {
        displayedName = EraJS::read(displayedName.Format(NAME_FORMAT, index -1).String());
        H3Point point;
        CDECL_6(int, 0x072F539, objectType, objectSubtype, index, &point.x, &point.y, &point.z);
        position = point;
    }
};

class TeleportDlg : public H3Dlg
{
    static constexpr size_t MAX_DESTINATIONS = 9;
    static constexpr size_t DESTINATION_PANEL_HEIGHT = 40;
    H3Hero *hero = nullptr;
    H3DlgScrollbar *scrollBar = nullptr;
    struct DestinationPanel
    {
        H3DlgDef *icon = nullptr;
        H3DlgText *text = nullptr;
        const HeroTeleport *target = nullptr;

      public:
        void SetTarget(const HeroTeleport *teleport, const BOOL refresh)
        {

            if (teleport)
            {
                text->SetText(teleport->displayedName);
                icon->ShowActivate();
            }
            else
            {
                text->SetText(h3_NullString);
                icon->HideDeactivate();
            }
            target = teleport;
            if (refresh)
            {
                text->Draw();
                text->Refresh();
                icon->Draw();
                icon->Refresh();
            }
        }
    };

    std::vector<DestinationPanel> destinationPanels;
    std::vector<HeroTeleport> &destinations;

  public:
    int selectedIndex = -1;

  public:
    TeleportDlg(H3Hero *hero, std::vector<HeroTeleport> &destinations)
        : H3Dlg(306, 469, -1, -1, TRUE, TRUE), hero(hero), destinations(destinations)
    {
        CreateItems();
    }

    virtual ~TeleportDlg()
    {
    }

  public:
    //  virtual inline BOOL DialogProc(H3Msg &msg);

  protected:
    static void __fastcall ScrollBarHandler(INT32 itemID, H3BaseDlg *dlg);

    void RedrawDestinationPanels(const int index, const BOOL redrawDlg);
    void CreateItems();
};
