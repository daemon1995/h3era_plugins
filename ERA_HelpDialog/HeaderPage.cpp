#include "HeaderPage.h"
#include "HelpUI.h"
namespace main
{
HeaderPage::HeaderPage(int x, int y, int width, int height, H3Dlg *parent) : DlgPage(parent)
{
    AddFrame(x, y, width, height);
    const eHelpPage pages[] = {eHelpPage::MODS,      eHelpPage::HOTKEYS, eHelpPage::CREATURES,
                               eHelpPage::ARTIFACTS, eHelpPage::HEROES,  eHelpPage::SECONDARY_SKILLS,
                               eHelpPage::SPELLS,    eHelpPage::TOWNS};
    const int cellWidth = (width - 12) / 4;
    for (int index = 0; index < 8; ++index)
    {
        auto *button =
            helpdlg::Button(nullptr, x + 6 + (index % 4) * cellWidth, y + 4 + (index / 4) * 27, cellWidth - 4, 25,
                            static_cast<int>(pages[index]), helpdlg::PageName(pages[index]));
        AddItem(button);
        tabs.push_back(button);
    }
    AddItem(H3DlgText::Create(x + 10, y + 59, 54, 22, helpdlg::Text("help.ui.search_label", "Search:"),
                              NH3Dlg::Text::SMALL, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT));
    searchEdit = H3DlgEdit::Create(x + 68, y + 58, width - 328, 24, 128, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                                   eTextAlignment::MIDDLE_LEFT, nullptr, buttons::SEARCH, true, 2, 2);
    AddItem(searchEdit);
    if (searchEdit)
    {
        searchEdit->SetAutoredraw(TRUE);
        searchEdit->SetHints(
            helpdlg::Text("help.ui.search_hint", "Titles only. Ctrl+F: focus. Results: left to go, right for preview."),
            nullptr, false);
    }
    AddItem(helpdlg::Button(nullptr, x + width - 254, y + 59, 46, 21, buttons::SEARCH_CLEAR,
                            helpdlg::Text("help.ui.clear", "Clear")));
    AddItem(helpdlg::Button(nullptr, x + width - 204, y + 59, 98, 21, buttons::RESIZE_DLG,
                            helpdlg::Text("help.ui.resize", "Resize")));
    AddItem(helpdlg::Button(nullptr, x + width - 104, y + 59, 98, 21, buttons::HELP,
                            helpdlg::Text("help.ui.guide", "User guide")));
}
void HeaderPage::SetActiveButton(int id) noexcept
{
    for (auto *button : tabs)
        if (button)
            button->SetFrame(button->GetID() == id ? 1 : 0);
    RedrawDialog();
}

std::string HeaderPage::Query() const
{
    return searchEdit ? helpdlg::Safe(searchEdit->GetText()) : "";
}

void HeaderPage::SetQuery(const std::string &query)
{
    if (searchEdit)
        searchEdit->SetText(query.c_str());
}

void HeaderPage::FocusSearch()
{
    if (searchEdit)
        searchEdit->SetFocus(TRUE);
}
void HeaderPage::ReleaseInputFocus(int keepItemId) noexcept
{
    auto *edit = searchEdit;
    if (edit && edit->IsFocused() && edit->GetID() != keepItemId)
        edit->SetFocus(FALSE);
}
} // namespace main
