#pragma once

#include "HelpDialogDependencies.h"
#include "HelpLogic.h"
#include "DlgEnums.h"

#include <string>

namespace helpdlg
{
inline LPCSTR Text(LPCSTR key, LPCSTR fallback)
{
    bool success = false;
    LPCSTR value = EraJS::read(key, success);
    return success && value && *value ? value : fallback;
}

inline std::string Safe(LPCSTR value)
{
    return value ? value : "";
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

inline H3DlgCaptionButton *Button(H3Dlg *dialog, int x, int y, int width, int height, int id, LPCSTR text)
{
    auto *button = H3DlgCaptionButton::Create(x, y, width, height, id, "OVBUTN3.def", text, NH3Dlg::Text::SMALL, 0, 0,
                                              false, 0, eTextColor::REGULAR);
    if (button)
    {
        button->SetClickFrame(1);
        if (dialog)
            dialog->AddItem(button);
    }
    return button;
}

inline LPCSTR PageName(main::eHelpPage page)
{
    switch (page)
    {
    case main::eHelpPage::MODS:
        return Text("help.ui.mods", "Mods");
    case main::eHelpPage::HOTKEYS:
        return Text("help.ui.hotkeys", "Hotkeys");
    case main::eHelpPage::CREATURES:
        return Text("help.ui.creatures", "Creatures");
    case main::eHelpPage::ARTIFACTS:
        return Text("help.ui.artifacts", "Artifacts");
    case main::eHelpPage::TOWNS:
        return Text("help.ui.towns", "Towns");
    case main::eHelpPage::HEROES:
        return Text("help.ui.heroes", "Heroes");
    case main::eHelpPage::SECONDARY_SKILLS:
        return Text("help.ui.skills", "Secondary skills");
    case main::eHelpPage::SPELLS:
        return Text("help.ui.spells", "Spells");
    default:
        return "Help";
    }
}

inline LPCSTR ContextName(int context)
{
    static LPCSTR names[] = {"Unspecified", "Adventure map", "Hero window", "Town window",
                             "Battle",      "Main menu",     "Other"};
    if (context == -1)
        return Text("help.contexts.everywhere", "Everywhere");
    return Text(H3String::Format("help.contexts.%d", context).String(), names[Bound(context, 0, 6)]);
}
} // namespace helpdlg
