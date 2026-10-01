#include "HeroTeleport.h"
#include "TeleportSelector.h"

#pragma comment(linker, "/EXPORT:DisplayHeroTeleporter=_DisplayHeroTeleporter@20")

DllExport int __stdcall DisplayHeroTeleporter(const int heroId, const int objectType, const int objectSubtype,
                                              const int *objectIndexes, const int arraySize)
{
    (void)heroId;
    (void)objectType;
    (void)objectSubtype;

    // Compatibility entry point for the old ERM call. The new call must pass exactly eight
    // object indexes and the current object index through DisplayTeleportSelector.
    if (!objectIndexes || arraySize != mrart::TELEPORT_VARIANT_COUNT)
        return -1;
    return DisplayTeleportSelector(reinterpret_cast<const DWORD *>(objectIndexes), UINT_MAX);
}

TeleportDlg::TeleportDlg(const int variant, const DWORD currentObjectIndex)
    : H3Dlg(592, 744, -1, -1, FALSE, TRUE, 1), variant(variant), currentObjectIndex(currentObjectIndex),
      config(mrart::TeleportConfig::LoadVariant(variant))
{
    CreateDialogItems();
}

void TeleportDlg::CreateDialogItems()
{
    CreateText(0, 12, widthDlg, 34, mrart::TeleportConfig::DialogText("mrart.teleport_dlg.title", ""),
               NH3Dlg::Text::BIG, eTextColor::GOLD, 1);
    CreateText(0, 574, widthDlg, 34,
               mrart::TeleportConfig::DialogText("mrart.teleport_dlg.quadrant_question", ""),
               NH3Dlg::Text::MEDIUM, eTextColor::WHITE, 2);

    minimap = CreatePcx16(MAP_X, MAP_Y, MAP_SIZE, MAP_SIZE, 10, config.minimapPcx);

    panels.resize(config.quadrantCount);
    for (int quadrant = 0; quadrant < config.quadrantCount; ++quadrant)
    {
        const auto &item = config.quadrants[quadrant];
        if (item.width <= 0 || item.height <= 0)
            continue;

        const int x = MAP_X + item.x;
        const int y = MAP_Y + item.y;
        panels[quadrant].picture = CreatePcx16(x, y, item.width, item.height, 100 + quadrant, item.greenPcx);
        panels[quadrant].hitArea = CreateHidden(x, y, item.width, item.height, QUADRANT_FIRST_ID + quadrant);
    }

    if (config.quadrantCount > 0)
    {
        const auto &first = config.quadrants[0];
        selectionFrame = H3DlgFrame::Create(MAP_X + first.x, MAP_Y + first.y, first.width, first.height, 0,
                                             H3RGB565::Green());
        if (selectionFrame)
        {
            selectionFrame->HideDeactivate();
            AddItem(selectionFrame);
        }
    }

    CreateCancelButton(105, 645);
    if (H3DlgDefButton *okButton = CreateOKButton(409, 645))
        okButton->Disable();
}

void TeleportDlg::SelectQuadrant(const int quadrant)
{
    if (quadrant < 0 || quadrant >= config.quadrantCount)
        return;

    const auto &item = config.quadrants[quadrant];
    if (item.width <= 0 || item.height <= 0)
        return;

    selectedQuadrant = quadrant;
    if (selectionFrame)
    {
        selectionFrame->SetX(MAP_X + item.x);
        selectionFrame->SetY(MAP_Y + item.y);
        selectionFrame->Show();
    }
    if (H3DlgDefButton *okButton = GetDefButton(eControlId::OK))
        okButton->Enable();
    Redraw();
}

BOOL TeleportDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftClick() && msg.itemId >= QUADRANT_FIRST_ID &&
        msg.itemId < QUADRANT_FIRST_ID + config.quadrantCount)
    {
        SelectQuadrant(msg.itemId - QUADRANT_FIRST_ID);
        return TRUE;
    }
    return FALSE;
}

VOID TeleportDlg::OnOK()
{
    if (selectedQuadrant >= 0)
        Stop();
}

VOID TeleportDlg::OnCancel()
{
    selectedQuadrant = -1;
}
