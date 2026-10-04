#include "GuideDlg.h"
#include "HelpUI.h"
namespace help
{
GuideDlg::GuideDlg(int width, int height, int x, int y)
    : H3Dlg(std::min(width, H3GameWidth::Get() - 20), std::min(height, H3GameHeight::Get() - 20), x, y, false, false)
{
    helpdlg::AddSafeBackground(*this);
    CreateText(16, 12, widthDlg - 32, 28, helpdlg::Text("help.ui.guide", "User guide"), NH3Dlg::Text::BIG,
               eTextColor::GOLD, -1);
    LPCSTR content = helpdlg::Text(
        "help.guide.content",
        "Browse\nChoose a tab. Objects are sorted by ID. Left click an object for details; hold right click for a "
        "preview.\n\nFilters\nCategories are on "
        "the left; other filters are above the objects. Left click: next value. Right click: previous value. "
        "Reset clears filters.\n\nSearch\nType a title in the top Search field. Ctrl+F focuses it. Left click a result "
        "to "
        "go to and highlight it. Hold right click for a preview. Up/Down and Enter navigate too. Clear or Escape "
        "returns to browsing.\n\nHotkeys\nBindings are grouped by "
        "where they work.");
    auto *body = H3DlgScrollableText::Create(content, 16, 48, widthDlg - 46, heightDlg - 108, NH3Dlg::Text::MEDIUM,
                                             eTextColor::REGULAR, false);
    if (body)
        AddItem(body);
    auto *ok = helpdlg::CloseButtonRight(*this);
    if (ok)
    {
        ok->AddHotkey(eVKey::H3VK_ESCAPE);
        ok->AddHotkey(eVKey::H3VK_H);
    }
}
GuideDlg::~GuideDlg() {}
BOOL GuideDlg::DialogProc(H3Msg &)
{
    return TRUE;
}
} // namespace help
