#pragma once
#include "framework.h"

struct HeroTeleport
{
    static int objectType;
    static int objectSubtype;
    static constexpr LPCSTR NAME_FORMAT = "mrart.teleport_names.%d";
    static H3LoadedPcx16 *defaultPicture;

  public:
    const int index;
    H3Position position;
    H3String displayedName;
    H3LoadedPcx16 *picture = nullptr;

  public:
    inline HeroTeleport(const int index) : index(index)
    {

        displayedName = EraJS::read(displayedName.Format(NAME_FORMAT, index - 1).String());
        H3Point point;
        CDECL_6(int, 0x072F539, objectType, objectSubtype, index, &point.x, &point.y, &point.z);
        position = point;
    }
    BOOL CreatePcx16()
    {
        picture = H3LoadedPcx16::Create(48, 32);
       // libc::memset(picture->buffer, 0, picture->buffSize);
        defaultPicture->DrawToPcx16(0, 0, 1, picture, 0, 0);

        auto mapItem = P_Game->GetMapItem(position);

        LPCSTR terrainDefName = (*reinterpret_cast<LPCSTR **>(0x04071E7 + 1))[mapItem->land];
        auto defTerrain = H3LoadedDef::Load(terrainDefName);
        if (defTerrain)
        {
            const int frameIndex = defTerrain->groups[0]->count < 60 ? 15 : 60;
            defTerrain->DrawToPcx16(0, mapItem->landSprite, picture, 8, 0);
            defTerrain->Dereference();
        }
        return picture != nullptr;
    }
};

class TeleportDlg : public H3Dlg
{
    static constexpr size_t MAX_DESTINATIONS = 9;
    static constexpr size_t DESTINATION_PANEL_HEIGHT = 32;
    H3Hero *hero = nullptr;
    H3DlgScrollbar *scrollBar = nullptr;
    H3DlgFrame *selectionFrame = nullptr;
    struct DestinationPanel
    {
        H3DlgPcx16 *icon = nullptr;
        H3DlgText *text = nullptr;
        const HeroTeleport *target = nullptr;

      public:
        void SetTarget(const HeroTeleport *teleport, const BOOL refresh)
        {

            if (teleport)
            {
                text->SetText(teleport->displayedName);
                icon->SetPcx(teleport->picture);
                icon->ShowActivate();
            }
            else
            {
                icon->SetPcx(HeroTeleport::defaultPicture);
                text->SetText(h3_NullString);
                icon->HideDeactivate();
            }
            target = teleport;
            if (refresh)
            {
                text->Draw();
                text->Refresh();
                if (icon->GetPcx())
                {
                    icon->Draw();
                    icon->Refresh();
                }
            }
        }
    };

    std::vector<DestinationPanel> destinationPanels;
    std::vector<HeroTeleport> &heroTeleports;

  public:
    int selectedIndex = -1;

  public:
    TeleportDlg(H3Hero *hero, std::vector<HeroTeleport> &heroTeleports)
        : H3Dlg(322, 565, -1, -1, TRUE, TRUE), hero(hero), heroTeleports(heroTeleports)
    {

        if (hero->objectBelow)
        {
            // hero->objectBelowSetup;
            // auto &objList = P_Game->mainSetup.objectLists[HeroTeleport::objectType];
            // for (auto &i : objList)
            //{
            //     if (i.type == HeroTeleport::objectType && i.subtype == HeroTeleport::objectSubtype)
            //     {
            //         const auto defName = i.defName.String();
            //     }
            // }
        }

        CreateDestinationPanels();
        CreateOKButton();
        CreateCancelButton();
    }

    virtual ~TeleportDlg()
    {
        for (auto &panel : destinationPanels)
        {
            panel.icon->SetPcx(nullptr);
        }
    }

  public:
    virtual BOOL DialogProc(H3Msg &msg);

  protected:
    static void __fastcall ScrollBarHandler(INT32 itemID, H3BaseDlg *dlg);

    void RedrawDestinationPanels(const int index, const BOOL redrawDlg);
    void CreateDestinationPanels();
};
