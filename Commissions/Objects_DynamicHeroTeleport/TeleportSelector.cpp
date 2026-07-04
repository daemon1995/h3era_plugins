#include "TeleportSelector.h"

#pragma comment(linker, "/EXPORT:DisplayTeleportSelector=_DisplayTeleportSelector@0")
DllExport int __stdcall DisplayTeleportSelector()
{
    TeleportSelector dlg;
    dlg.Start();

    return 1;
}

TeleportSelector::TeleportSelector(int width, int height, int x, int y) : H3Dlg(width, height, x, y, false, false)
{
    this->AddBackground(0, 0, 1);
    background->SimpleFrameRegion(0, 0, width, height);
    CreateOKButton();
    CreateCancelButton();

    constexpr int marging = 4;

    // header
    background->SimpleFrameRegion(marging, 40, width - marging * 2, 132);

    // 2nd frame
    background->SimpleFrameRegion(marging, marging, width - marging * 2, height - marging * 2);

    CreateItems();
}

BOOL TeleportSelector::DialogProc(H3Msg &msg)
{

    return 1;
}

TeleportSelector::~TeleportSelector()
{
}

H3DlgFrame *CreateThickFrameOverItem(H3DlgItem *item)
{
    const H3RGB565 frameColor(0x7A, 0x65, 0x48);

    H3DlgFrame *frame = H3DlgFrame::Create(item, frameColor, -1, 1);
    frame->DeActivate();
    return frame;
}

void TeleportSelector::CreateItems()
{
    constexpr int teleportsNum = 8;
    teleportItems.resize(teleportsNum);
    int itY = ITEMS_TOP_MARGIN;

    constexpr int width = TeleportItem::ITEM_WIDTH;
    constexpr int height = TeleportItem::ITEM_HEIGHT;
    for (size_t i = 0; i < teleportsNum; i++)
    {

        const BOOL isPair = i & 1;
        int itX = isPair * (width + 24) + 26;

        TeleportItem &tpIt = teleportItems[i];
        tpIt.type = i;

        auto &items = tpIt.items;

        items.picture = CreatePcx16(itX, itY, width, height, i + 10, 0);

        auto pcx = H3LoadedPcx16::Create(width, height);
        memset(pcx->buffer, 0, pcx->buffSize);
        items.picture->SetPcx(pcx);
        items.name =
            CreateText(itX, itY + height - 24, width, 24, "text", NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
        items.name->DeActivate();

        AddItem(CreateThickFrameOverItem(items.picture));
        AddItem(CreateThickFrameOverItem(items.name));

        if (isPair)
            itY += height + 16;
    }
}
