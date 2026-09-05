#include "HelpPage.h"

namespace main
{

HelpPage::HelpPage(H3Dlg *dialog) : dialog(dialog)
{
}

void HelpPage::AddItem(H3DlgItem *item)
{
    if (!item || !dialog)
    {
        return;
    }

    dialog->AddItem(item);
    items.emplace_back(item);
    item->HideDeactivate();
}

void HelpPage::AddScrollableText(H3DlgScrollableText *scrollableText)
{
    if (!scrollableText || !dialog)
        return;

    dialog->AddItem(scrollableText);
    scrollableTexts.emplace_back(scrollableText);
    SetScrollableTextVisible(scrollableText, FALSE);
}

void HelpPage::SetScrollableText(H3DlgScrollableText *scrollableText, LPCSTR text)
{
    if (!scrollableText)
        return;

    // SetText rebuilds the composite control's internal text items. Restore
    // their state immediately so a hidden page cannot leak freshly-created
    // lines into the active page.
    scrollableText->SetText(text ? text : h3_NullString);
    SetScrollableTextVisible(scrollableText, isVisible);
}

void HelpPage::SetScrollableTextVisible(H3DlgScrollableText *scrollableText, const BOOL state) const noexcept
{
    if (!scrollableText)
        return;

    state ? scrollableText->ShowActivate() : scrollableText->HideDeactivate();

    if (auto *textItems = scrollableText->GetItems())
    {
        for (auto *textItem : *textItems)
        {
            if (textItem)
                state ? textItem->ShowActivate() : textItem->HideDeactivate();
        }
    }

    if (auto *scrollBar = scrollableText->GetTextScrollBar())
    {
        if (state && scrollBar->GetTicksCount() > 1)
            scrollBar->ShowActivate();
        else
            scrollBar->HideDeactivate();
    }
}

void HelpPage::AddFrame(const int x, const int y, const int width, const int height)
{
    if (!dialog)
    {
        return;
    }

    H3RGB565 frameColor(H3RGB888::Highlight());
    H3DlgItem *frame = H3DlgFrame::Create(x, y, width, height, -1, frameColor);
    if (!frame)
    {
        return;
    }

    // A frame is decoration, not an interactive item. Keep it outside of
    // the active page item list so it cannot intercept button clicks.
    dialog->AddItem(frame);
    decorations.emplace_back(frame);
    frame->HideDeactivate();
}

void HelpPage::SetVisible(const BOOL state) noexcept
{
    if (isVisible == state)
    {
        return;
    }

    isVisible = state;
    for (auto *decoration : decorations)
    {
        if (decoration)
        {
            state ? decoration->Show() : decoration->HideDeactivate();
        }
    }
    for (auto *item : items)
    {
        if (item)
        {
            state ? item->ShowActivate() : item->HideDeactivate();
        }
    }
    for (auto *scrollableText : scrollableTexts)
        SetScrollableTextVisible(scrollableText, state);
}

BOOL HelpPage::IsVisible() const noexcept
{
    return isVisible;
}

void HelpPage::RedrawDialog() const noexcept
{
    // OnCreate runs before H3Dlg::Start() lets the window manager save the
    // screen beneath the dialog. Drawing at that point would put our own
    // controls into that backup and make them reappear after closing.
    if (dialog && isVisible && P_WindowManager->lastDlg == dialog)
    {
        dialog->Redraw();
    }
}

} // namespace main
