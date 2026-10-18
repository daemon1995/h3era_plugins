#include "HeaderPage.h"
#include "HelpUI.h"
namespace main
{
namespace
{
constexpr LPCSTR kResizeDef = "ERAHDRES.DEF";
constexpr LPCSTR kSettingsDef = "ERAHDSET.DEF";
} // namespace
HeaderPage::HeaderPage(int x, int y, int width, int height, int leftWidth, H3Dlg *parent) : DlgPage(parent)
{
    AddFrame(x, y, width, height);

    const int panelX = x + 4;
    const int panelY = y + 4;
    const int panelWidth = std::max(120, leftWidth - 8);
    const int rowHeight = 26;
    AddFrame(panelX, panelY, panelWidth, rowHeight * 2 + 4);

    modButton = helpdlg::Button(nullptr, panelX + 2, panelY + 2, panelWidth - 4, rowHeight, buttons::MODLIST,
                                helpdlg::PageName(eHelpPage::MODS), PAGE_BUTTON_DEF);
    AddItem(modButton);
    if (modButton)
        modButton->SetHints(helpdlg::Text("help.ui.mod_select_hint"), nullptr, false);

    constexpr int utilityWidth = 26;
    const int searchY = panelY + rowHeight + 4;
    const int editX = panelX + 2;
    const int editWidth = panelWidth - utilityWidth * 2 - 8;
    searchEdit = H3DlgEdit::Create(editX, searchY, editWidth, 24, 128, helpdlg::Text("help.ui.search_placeholder"),
                                   NH3Dlg::Text::SMALL, eTextColor::REGULAR, eTextAlignment::MIDDLE_LEFT, nullptr,
                                   buttons::SEARCH_EDIT, true, 2, 2);
    AddItem(searchEdit);
    if (searchEdit)
    {
        searchEdit->SetAutoredraw(TRUE);
        searchEdit->SetHints(helpdlg::Text("help.ui.search_hint"), nullptr, false);
    }
    auto *clear = helpdlg::Button(nullptr, editX + editWidth + 2, searchY, utilityWidth, 24, buttons::SEARCH_CLEAR,
                                  helpdlg::Text("help.ui.symbol_clear"), "OVBUTN2.def");
    AddItem(clear);
    auto *go = helpdlg::Button(nullptr, editX + editWidth + utilityWidth + 4, searchY, utilityWidth, 24,
                               buttons::SEARCH, helpdlg::Text("help.ui.symbol_search"), "OVBUTN2.def");
    AddItem(go);
    if (clear)
        clear->SetHints(helpdlg::Text("help.ui.clear"), nullptr, false);
    if (go)
        go->SetHints(helpdlg::Text("help.ui.search"), nullptr, false);

    const int iconWidth = 30;
    const int iconX = x + width - iconWidth - 6;
    const int tableX = x + leftWidth + 4;
    const int tableWidth = std::max(120, iconX - tableX - 4);
    const int cellWidth = tableWidth / 3;
    AddFrame(tableX - 2, panelY, tableWidth + 2, rowHeight * 2 + 4);
    const eHelpPage pages[] = {eHelpPage::CREATURES, eHelpPage::ARTIFACTS,        eHelpPage::HEROES,
                               eHelpPage::SPELLS,    eHelpPage::SECONDARY_SKILLS, eHelpPage::HOTKEYS};
    for (int index = 0; index < 6; ++index)
    {
        auto *button = helpdlg::Button(
            nullptr, tableX + (index % 3) * cellWidth, panelY + 2 + (index / 3) * (rowHeight + 2), cellWidth - 4,
            rowHeight, static_cast<int>(pages[index]), helpdlg::PageName(pages[index]), PAGE_BUTTON_DEF);
        AddItem(button);
        tabs.push_back(button);
    }

    auto *resize = helpdlg::IconButton(nullptr, iconX, panelY + 1, iconWidth, rowHeight, buttons::RESIZE_DLG,
                                       kResizeDef, helpdlg::Text("help.ui.symbol_resize"));
    AddItem(resize);
    auto *settings = helpdlg::IconButton(nullptr, iconX, panelY + rowHeight + 3, iconWidth, rowHeight,
                                         buttons::SETTINGS, kSettingsDef, helpdlg::Text("help.ui.symbol_settings"));
    AddItem(settings);
    if (resize)
        resize->SetHints(helpdlg::Text("help.ui.resize"), nullptr, false);
    if (settings)
        settings->SetHints(helpdlg::Text("help.ui.settings"), nullptr, false);

    RefreshSearchPlaceholder();
}
void HeaderPage::SetActiveButton(int id, LPCSTR modName) noexcept
{
    for (auto *button : tabs)
        if (button)
        {
            const bool state = button->GetID() == id;
            button->SendCommand(6 - state, 4096);
        }
    if (modButton)
    {
        const LPCSTR label = modName ? modName : helpdlg::PageName(eHelpPage::MODS);
        modButton->SetText(label);
        modButton->SetHints(label, helpdlg::Text("help.ui.mod_select_hint"), false);
        // The mod selector is always an active dropdown, never a selected/disabled page tab.
        modButton->SetFrame(0);
        modButton->SetClickFrame(1);
        modButton->UnShade();
        modButton->ShowActivate();
    }
    RedrawDialog();
}

LPCSTR HeaderPage::Query() const
{
    if (!searchEdit)
        return h3_NullString;
    LPCSTR value = helpdlg::Borrow(searchEdit->GetText());
    return !strcmp(value, helpdlg::Text("help.ui.search_placeholder")) ? h3_NullString : value;
}

void HeaderPage::SetQuery(const std::string &query)
{
    if (!searchEdit)
        return;
    if (!query.empty())
        searchEdit->SetText(query.c_str());
    else if (SearchFocused())
        searchEdit->SetText(h3_NullString);
    else
        searchEdit->SetText(helpdlg::Text("help.ui.search_placeholder"));
}

void HeaderPage::FocusSearch()
{
    if (!searchEdit)
        return;
    LPCSTR value = helpdlg::Borrow(searchEdit->GetText());
    if (!strcmp(value, helpdlg::Text("help.ui.search_placeholder")))
        searchEdit->SetText(h3_NullString);
    if (!SearchFocused())
        searchEdit->SetFocus(TRUE);
}
bool HeaderPage::SearchFocused() const noexcept
{
    return searchEdit && searchEdit->IsFocused();
}
bool HeaderPage::SearchContains(int dialogX, int dialogY) const noexcept
{
    return searchEdit && dialogX >= searchEdit->GetX() && dialogX < searchEdit->GetX() + searchEdit->GetWidth() &&
           dialogY >= searchEdit->GetY() && dialogY < searchEdit->GetY() + searchEdit->GetHeight();
}
void HeaderPage::RefreshSearchPlaceholder() noexcept
{
    if (!searchEdit)
        return;
    LPCSTR value = helpdlg::Borrow(searchEdit->GetText());
    const LPCSTR placeholder = helpdlg::Text("help.ui.search_placeholder");
    if (SearchFocused())
    {
        if (!strcmp(value, placeholder))
            searchEdit->SetText(h3_NullString);
    }
    else if (!*value)
        searchEdit->SetText(placeholder);
}
void HeaderPage::ReleaseInputFocus(int keepItemId) noexcept
{
    auto *edit = searchEdit;
    if (edit && edit->IsFocused() && edit->GetID() != keepItemId)
        edit->SetFocus(FALSE);
    RefreshSearchPlaceholder();
}
} // namespace main
