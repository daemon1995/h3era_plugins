#include "PlaceholderPage.h"

namespace main
{

PlaceholderSection::PlaceholderSection(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog)
{
    AddFrame(x, y, width, height);
    text = H3DlgScrollableText::Create(h3_NullString, x + 10, y + 8, width - 28, height - 16,
                                       NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, true);
    AddScrollableText(text);
}

void PlaceholderSection::SetTitle(LPCSTR value)
{
    title = value ? value : h3_NullString;
    SetScrollableText(text, title.Empty() ? h3_NullString : title.String());
}

void PlaceholderSection::SetVisible(const BOOL state) noexcept
{
    HelpPage::SetVisible(state);
}

BOOL PlaceholderSection::ProcessMessage(H3Msg &)
{
    return FALSE;
}

void PlaceholderSection::Redraw()
{
}

} // namespace main
