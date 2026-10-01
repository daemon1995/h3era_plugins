#include "TeleportSelector.h"
#include "HeroTeleport.h"

#pragma comment(linker, "/EXPORT:DisplayTeleportSelector=_DisplayTeleportSelector@8")

DllExport int __stdcall DisplayTeleportSelector(const DWORD *visitedObjectIndexes, const DWORD currentObjectIndex)
{
    if (!visitedObjectIndexes)
        return -1;

    TeleportSelector selector(visitedObjectIndexes, currentObjectIndex);
    selector.Start();
    if (selector.selectedVariant < 0)
        return -1;

    TeleportDlg dialog(selector.selectedVariant, currentObjectIndex);
    dialog.Start();
    if (dialog.selectedQuadrant < 0)
        return -1;

    return mrart::PackSelection(selector.selectedVariant + 1, dialog.selectedQuadrant + 1);
}

TeleportSelector::TeleportSelector(const DWORD *visitedObjectIndexes, const DWORD currentObjectIndex)
    : H3Dlg(592, 744, -1, -1, FALSE, TRUE, 1), currentObjectIndex(currentObjectIndex)
{
    for (int i = 0; i < mrart::TELEPORT_VARIANT_COUNT; ++i)
        this->visitedObjectIndexes[i] = visitedObjectIndexes[i];

    CreateItems();
}

BOOL TeleportSelector::IsAvailable(const int variant) const noexcept
{
    if (variant < 0 || variant >= mrart::TELEPORT_VARIANT_COUNT)
        return FALSE;

    const DWORD objectIndex = visitedObjectIndexes[variant];
    return objectIndex != 0 && objectIndex != currentObjectIndex;
}

void TeleportSelector::CreateItems()
{
    CreateText(0, 12, widthDlg, 34, mrart::TeleportConfig::DialogText("mrart.teleport_dlg.title", ""),
               NH3Dlg::Text::BIG, eTextColor::GOLD, 1);
    CreateText(0, 166, widthDlg, 34,
               mrart::TeleportConfig::DialogText("mrart.teleport_dlg.destination_question", ""),
               NH3Dlg::Text::MEDIUM, eTextColor::WHITE, 2);

    hitAreas.reserve(mrart::TELEPORT_VARIANT_COUNT);
    for (int variant = 0; variant < mrart::TELEPORT_VARIANT_COUNT; ++variant)
    {
        const auto config = mrart::TeleportConfig::LoadVariant(variant);
        const BOOL available = IsAvailable(variant);
        const int x = CARD_X[variant & 1];
        const int y = CARD_FIRST_Y + (variant >> 1) * CARD_STEP_Y;

        LPCSTR preview = available ? config.previewPurplePcx : config.previewGreenPcx;
        CreatePcx16(x, y, CARD_WIDTH, CARD_HEIGHT - 24, 20 + variant, preview);
        CreateText(x, y + CARD_HEIGHT - 24, CARD_WIDTH, 24, config.name, NH3Dlg::Text::MEDIUM,
                   available ? eTextColor::WHITE : eTextColor::GRAY, 40 + variant);

        const H3RGB565 frameColor = available ? H3RGB565::Purple() : H3RGB565::Gray();
        H3DlgFrame *frame = H3DlgFrame::Create(x, y, CARD_WIDTH, CARD_HEIGHT, 0, frameColor);
        if (frame)
        {
            frame->DeActivate();
            AddItem(frame);
        }

        H3DlgTransparentItem *hitArea = CreateHidden(x, y, CARD_WIDTH, CARD_HEIGHT, VARIANT_FIRST_ID + variant);
        if (!available && hitArea)
            hitArea->Disable();
        hitAreas.push_back(hitArea);
    }

    selectionFrame = H3DlgFrame::Create(CARD_X[0], CARD_FIRST_Y, CARD_WIDTH, CARD_HEIGHT, 0, H3RGB565::Green());
    if (selectionFrame)
    {
        selectionFrame->HideDeactivate();
        AddItem(selectionFrame);
    }

    CreateCancelButton(105, 645);
    if (H3DlgDefButton *okButton = CreateOKButton(409, 645))
        okButton->Disable();
}

void TeleportSelector::SelectVariant(const int variant)
{
    if (!IsAvailable(variant))
        return;

    selectedVariant = variant;
    if (selectionFrame)
    {
        selectionFrame->SetX(CARD_X[variant & 1]);
        selectionFrame->SetY(CARD_FIRST_Y + (variant >> 1) * CARD_STEP_Y);
        selectionFrame->Show();
    }
    if (H3DlgDefButton *okButton = GetDefButton(eControlId::OK))
        okButton->Enable();
    Redraw();
}

BOOL TeleportSelector::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftClick() && msg.itemId >= VARIANT_FIRST_ID &&
        msg.itemId < VARIANT_FIRST_ID + mrart::TELEPORT_VARIANT_COUNT)
    {
        SelectVariant(msg.itemId - VARIANT_FIRST_ID);
        return TRUE;
    }
    return FALSE;
}

VOID TeleportSelector::OnOK()
{
    if (selectedVariant >= 0)
        Stop();
}

VOID TeleportSelector::OnCancel()
{
    selectedVariant = -1;
}
