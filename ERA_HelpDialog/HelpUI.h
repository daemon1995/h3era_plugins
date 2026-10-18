#pragma once

#include "HelpDialogDependencies.h"
#include "HelpLogic.h"
#include "DlgEnums.h"
#include "DlgPage.h"

#include <string>

namespace helpdlg
{
// ERA owns these strings permanently. Callers borrow them; only formatted text
// needs its own storage.
inline LPCSTR Text(LPCSTR key)
{
    return EraJS::read(key);
}

inline LPCSTR Borrow(LPCSTR value) noexcept
{
    return value ? value : h3_NullString;
}

inline std::string Safe(LPCSTR value)
{
    return Borrow(value);
}

template <typename... Args> inline std::string FormatText(LPCSTR key, Args... args)
{
    return H3String::Format(Text(key), args...).String();
}

inline std::string ObjectTitle(LPCSTR name, int id)
{
    return id < 0 ? Safe(name) : FormatText("help.formats.object_title", Borrow(name), id);
}

constexpr LPCSTR kSelectorPcx = "ERAHDSLT.PCX";
constexpr int kSelectorArrowWidth = 22;

inline H3LoadedPcx *CreateSelectorBackground(int width)
{
    auto *pcx = H3LoadedPcx::Create(kSelectorPcx, width, kPageButtonHeight);
    if (!pcx)
        return nullptr;
    H3PcxLoader tile(NH3Dlg::Assets::DIBOXBACK);
    for (int index = 0; index < 256; ++index)
    {
        const auto color = tile.Get() ? tile->palette888.color[index] : H3RGB888(35, 30, 24);
        pcx->palette888.color[index] = H3RGB888(color.r / 2, color.g / 2, color.b / 2);
    }
    pcx->palette565.InitiateFromPalette888(pcx->palette888);
    for (int y = 0; y < pcx->height; ++y)
        for (int x = 0; x < pcx->width; ++x)
            *pcx->GetPixelIndex(x, y) = tile.Get() && tile->buffer && tile->width > 0 && tile->height > 0
                                            ? *tile->GetPixelIndex(x % tile->width, y % tile->height)
                                            : 0;
    // The dialog owns this reference. SelectorTextPcx explicitly releases the
    // references obtained by native TextPcx constructors before this owner dies.
    pcx->IncreaseReferences();
    // This H3API version's AddToResourceManager inserts only the name and leaves
    // m_item null. The native PCX loader dereferences it on a cache hit (0x55AA67).
    // Insert the complete record, as ERA's TBinaryTreeNode.AddItem does.
    H3ResourceItemData resource(pcx->GetName());
    resource.m_item = pcx;
    struct InsertResult
    {
        H3ResourceManager::Node *node;
        BOOL8 inserted;
    } result;
    THISCALL_3(InsertResult &, 0x55DDF0, H3ResourceManager::Get(), &result, &resource);
    return pcx;
}

struct SelectorTextPcx : H3DlgTextPcx
{
    void ReleaseBackground()
    {
        auto *pcx = loadedPcx;
        loadedPcx = nullptr;
        if (pcx)
            pcx->Dereference();
    }
};

inline H3DlgTextPcx *SelectorText(int x, int y, int width, int height, int id, LPCSTR text = h3_NullString)
{
    const auto *cached = H3ResourceManager::Get()->FindItem(kSelectorPcx);
    if (!cached || !cached->data.m_item)
        return nullptr;
    auto *item = H3ObjectAllocator<SelectorTextPcx>().allocate(1);
    if (item)
        THISCALL_12(SelectorTextPcx *, 0x5BCB70, item, x, y, width, height, text, NH3Dlg::Text::SMALL, kSelectorPcx,
                    eTextColor::REGULAR, id, eTextAlignment::MIDDLE_CENTER, 8);
    return item;
}

inline void ReleaseSelectorText(H3DlgTextPcx *item)
{
    // The native TextPcx destructor only destroys its text; it borrows the PCX.
    if (item)
        static_cast<SelectorTextPcx *>(item)->ReleaseBackground();
}

inline H3DlgFrame *GoldFrame(H3Dlg &dialog, int x, int y, int width, int height)
{
    auto *frame = dialog.CreateFrame(x, y, width, height, -1, H3RGB565(H3RGB888::Highlight()));
    if (frame)
    {
        frame->Disable();
        frame->DeActivate();
    }
    return frame;
}

inline void PlaceSelectorFrame(H3DlgFrame *frame, const H3DlgItem *row)
{
    if (!frame)
        return;
    if (row && row->IsVisible())
    {
        frame->SetX(row->GetX());
        frame->SetY(row->GetY());
        frame->SetWidth(row->GetWidth());
        frame->SetHeight(row->GetHeight());
        frame->Show()->DeActivate();
    }
    else
        frame->HideDeactivate();
}

inline bool AddSafeBackground(H3Dlg &dialog, bool statusBar = false, bool decorativeFrame = true)
{
    const int width = dialog.GetWidth(), height = dialog.GetHeight();
    auto *canvas = H3LoadedPcx16::Create(width, height);
    if (!canvas)
        return false;
    canvas->FillRectangle(0, 0, width, height, 35, 30, 24);
    H3PcxLoader tile(NH3Dlg::Assets::DIBOXBACK);
    if (tile.Get())
        ForEachBackgroundTile(width, height, tile->width, tile->height,
                              [&](int x, int y, int w, int h) { tile->DrawToPcx16(0, 0, w, h, canvas, x, y, FALSE); });
    // Avoid H3API::BackgroundRegion: it passes the remaining dialog width
    // instead of the tile width, reading beyond DIBOXBACK in HD's renderer.
    if (decorativeFrame && width >= 64 && height >= 64)
    {
        H3DefLoader frame(NH3Dlg::Assets::DLGBOX);
        if (frame.Get())
            canvas->FrameRegion(0, 0, width, height, statusBar, 0, FALSE);
    }
    const bool added = dialog.AddBackground(canvas) != FALSE;
    canvas->Destroy();
    return added;
}

inline H3DlgDefButton *CloseButtonRight(H3Dlg &dialog)
{
    const int x = dialog.GetWidth() - 82, y = dialog.GetHeight() - 48;
    auto *button = dialog.CreateOKButton(x, y);
    if (button)
    {
        // Set the position after construction as other plugins can hook the
        // native button constructor. Its surrounding box is created at x/y.
        button->SetX(x);
        button->SetY(y);
        button->AddHotkey(eVKey::H3VK_ESCAPE);
    }
    return button;
}
inline void ShowDetailsDialog(H3Dlg &dialog, bool popup)
{
    if (popup)
        dialog.RMB_Show();
    else
    {
        CloseButtonRight(dialog);
        dialog.Start();
    }
}

inline H3DlgCaptionButton *Button(H3Dlg *dialog, int x, int y, int width, int height, int id, LPCSTR text,
                                  LPCSTR defName = "OVBUTN3.def")
{
    auto *button = H3DlgCaptionButton::Create(x, y, width, height, id, defName, text, NH3Dlg::Text::SMALL, 0, 0, false,
                                              0, eTextColor::REGULAR);
    if (button)
    {
        button->SetClickFrame(1);
        if (dialog)
            dialog->AddItem(button);
    }
    return button;
}

inline H3DlgItem *IconButton(H3Dlg *dialog, int x, int y, int width, int height, int id, LPCSTR defName,
                             LPCSTR fallbackText = h3_NullString)
{
    H3DlgItem *item = nullptr;
    H3DefLoader def(defName);
    if (def.Get() && def->widthDEF > 0 && def->heightDEF > 0)
        item = H3DlgDefButton::Create(x, y, width, height, id, defName, 0, 1, FALSE, 0);
    if (!item)
        item = Button(nullptr, x, y, width, height, id, fallbackText, "OVBUTN2.def");
    if (item && dialog)
        dialog->AddItem(item);
    return item;
}

inline H3DlgCaptionButton *CategoryButton(int x, int y, int id, LPCSTR text)
{
    auto *button = H3DlgCaptionButton::Create(x, y, id, main::DlgPage::CATEGORY_BUTTON_DEF, text, NH3Dlg::Text::SMALL,
                                              0, 0, false, 0, eTextColor::REGULAR);
    if (button)
        button->SetClickFrame(1);
    return button;
}

inline int CategoryButtonStep()
{
    H3DefLoader def(main::DlgPage::CATEGORY_BUTTON_DEF);
    return def.Get() && def->heightDEF > 0 ? def->heightDEF + 4 : 40;
}

inline void SetCategoryState(H3DlgCaptionButton *button, bool selected)
{
    if (!button)
        return;
    button->SendCommand(6 - selected, 4096);
}

inline LPCSTR PageName(main::eHelpPage page)
{
    switch (page)
    {
    case main::eHelpPage::MODS:
        return Text("help.ui.mods");
    case main::eHelpPage::HOTKEYS:
        return Text("help.ui.hotkeys");
    case main::eHelpPage::CREATURES:
        return Text("help.ui.creatures");
    case main::eHelpPage::ARTIFACTS:
        return Text("help.ui.artifacts");
    case main::eHelpPage::TOWNS:
        return Text("help.ui.towns");
    case main::eHelpPage::HEROES:
        return Text("help.ui.heroes");
    case main::eHelpPage::SECONDARY_SKILLS:
        return Text("help.ui.skills");
    case main::eHelpPage::SPELLS:
        return Text("help.ui.spells");
    default:
        return Text("help.ui.help");
    }
}

inline LPCSTR ContextName(int context)
{
    if (context == -1)
        return Text("help.contexts.everywhere");
    return Text(H3String::Format("help.contexts.%d", Bound(context, 0, 6)).String());
}
} // namespace helpdlg
