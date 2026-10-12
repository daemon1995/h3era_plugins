#include "HeaderPage.h"
#include "HelpUI.h"
namespace main
{
HeaderPage::HeaderPage(int x, int y, int width, int height, H3Dlg *parent) : DlgPage(parent)
{
    AddFrame(x, y, width, height);
    const int modWidth = (width - 12) / 4;
    auto *mods = helpdlg::Button(nullptr, x + 6, y + 4, modWidth - 4, helpdlg::kPageButtonHeight, buttons::MODLIST,
                                 helpdlg::PageName(eHelpPage::MODS), PAGE_BUTTON_DEF);
    AddItem(mods);
    activeModButton = helpdlg::Button(nullptr, x + 6, y + 4 + helpdlg::kModDropdownRowHeight, modWidth - 4,
                                      helpdlg::kPageButtonHeight, buttons::ACTIVE_MOD, "", PAGE_BUTTON_DEF);
    AddItem(activeModButton);
    const eHelpPage pages[] = {eHelpPage::HOTKEYS, eHelpPage::CREATURES,        eHelpPage::ARTIFACTS,
                               eHelpPage::HEROES,  eHelpPage::SECONDARY_SKILLS, eHelpPage::SPELLS};
    const int tableX = x + 6 + modWidth;
    const int tableWidth = width - 12 - modWidth;
    const int cellWidth = tableWidth / 3;
    AddFrame(tableX - 2, y + 2, tableWidth, 56);
    for (int index = 0; index < 6; ++index)
    {
        auto *button = helpdlg::Button(nullptr, tableX + (index % 3) * cellWidth,
                                       y + 4 + (index / 3) * helpdlg::kModDropdownRowHeight, cellWidth - 4,
                                       helpdlg::kPageButtonHeight, static_cast<int>(pages[index]),
                                       helpdlg::PageName(pages[index]), PAGE_BUTTON_DEF);
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
void HeaderPage::SetActiveButton(int id, LPCSTR modName) noexcept
{
    for (auto *button : tabs)
        if (button)
        {
            const bool state = button->GetID() == id;
            button->SendCommand(6 - (state), 4096);
        }
    if (activeModButton)
    {
        const bool state = id == buttons::MODLIST && modName && *modName;
        if (state)
        {
            activeModButton->SetText(modName);
            activeModButton->SetHints(modName, nullptr, false);
            activeModButton->ShowActivate();
            activeModButton->SendCommand(6 - (state), 4096);
        }
        else
            activeModButton->HideDeactivate();
    }
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
