#include "HeaderPage.h"
#include "HelpUI.h"
namespace main
{
namespace
{
constexpr LPCSTR kModButtonDef = "iHlpMod.def";
constexpr int kSearchPadding = 6;
} // namespace
HeaderPage::HeaderPage(int x, int y, int width, int height, int leftWidth, H3Dlg *parent) : DlgPage(parent)
{
    // Match the category and content columns below the header.
    const int panelX = x;
    const int panelY = y;
    const int panelWidth = leftWidth;
    const int rowHeight = 26;
    // Leave two pixels below the second row, including the search controls.
    const int panelHeight = height;
    AddFrame(panelX, panelY, panelWidth, panelHeight);

    modButton = H3DlgCaptionButton::Create(panelX + 6, panelY + 2, buttons::MODLIST, kModButtonDef,
                                            helpdlg::PageName(eHelpPage::MODS), NH3Dlg::Text::SMALL, 0, 0, false,
                                            0, eTextColor::REGULAR);
    AddItem(modButton);
    if (modButton)
    {
        modButton->SetClickFrame(1);
        modButton->SetHints(helpdlg::Text("help.ui.mod_select_hint"), nullptr, false);
    }

    constexpr int utilityWidth = 26;
    const int searchY = panelY + rowHeight + 4;
    const int searchX = panelX + 6;
    const int searchWidth = modButton ? modButton->GetWidth() : panelWidth - utilityWidth - 12;
    const int editX = searchX + kSearchPadding;
    const int editWidth = searchWidth - 2 * kSearchPadding;
    if (auto *background = parent->GetBackgroundPcx())
    {
        H3PcxLoader searchBackground;
        searchBackground.Set(helpdlg::CreateSelectorBackground(searchWidth, "ERAHDEDT.PCX", 24, true));
        if (searchBackground.Get())
            searchBackground->DrawToPcx16(0, 0, searchWidth, 24, background, searchX, searchY, FALSE);
    }
    AddFrame(searchX, searchY, searchWidth, 24);
    searchEdit = H3DlgEdit::Create(editX, searchY + 2, editWidth, 20, 128, h3_NullString,
                                   NH3Dlg::Text::SMALL, eTextColor::REGULAR, eTextAlignment::MIDDLE_LEFT, nullptr,
                                   buttons::SEARCH_EDIT, false, 0, 0);
    AddItem(searchEdit);
    if (searchEdit)
    {
        searchEdit->SetAutoredraw(TRUE);
        searchEdit->SetHints(helpdlg::Text("help.ui.search_hint"), nullptr, false);
    }
    searchPlaceholder = H3DlgText::Create(editX, searchY + 2, editWidth, 20,
                                          helpdlg::Text("help.ui.search_placeholder"), NH3Dlg::Text::SMALL,
                                          eTextColor::REGULAR, buttons::SEARCH_PLACEHOLDER,
                                          eTextAlignment::MIDDLE_LEFT);
    if (searchPlaceholder)
    {
        dialog->AddItem(searchPlaceholder);
        decorations.push_back(searchPlaceholder);
        searchPlaceholder->Disable();
        searchPlaceholder->HideDeactivate();
    }
    auto *clear = H3DlgDef::Create(searchX + searchWidth + 2, searchY, buttons::SEARCH_CLEAR, "tpthchk.def", 1);
    AddItem(clear);
    if (clear)
    {
        clear->SetY(searchY + (24 - clear->GetHeight()) / 2);
        clear->SetHints(helpdlg::Text("help.ui.clear"), nullptr, false);
    }

    const int tableX = x + leftWidth + 4;
    const int tableWidth = x + width - tableX;
    constexpr int padding = 4, columnGap = 6, rowGap = 4;
    const int gridWidth = tableWidth - 2 * padding + columnGap;
    const int cellHeight = (panelHeight - 2 * padding - rowGap) / 2;
    AddFrame(tableX, panelY, tableWidth, panelHeight);
    const eHelpPage pages[] = {eHelpPage::CREATURES, eHelpPage::ARTIFACTS,        eHelpPage::HEROES,
                               eHelpPage::SPELLS,    eHelpPage::SECONDARY_SKILLS, eHelpPage::HOTKEYS};
    for (int index = 0; index < 6; ++index)
    {
        auto *button = H3DlgCaptionButton::Create(
            0, 0,
            static_cast<int>(pages[index]), PAGE_BUTTON_DEF, helpdlg::PageName(pages[index]), NH3Dlg::Text::SMALL,
            0, 0, false, 0, eTextColor::REGULAR);
        if (button)
        {
            const int column = index % 3;
            const int cellLeft = tableX + padding + column * gridWidth / 3;
            const int cellWidth = (column + 1) * gridWidth / 3 - column * gridWidth / 3 - columnGap;
            button->SetX(cellLeft + (cellWidth - button->GetWidth()) / 2);
            button->SetY(panelY + padding + (index / 3) * (cellHeight + rowGap) +
                         (cellHeight - button->GetHeight()) / 2);
            button->SetClickFrame(1);
        }
        AddItem(button);
        tabs.push_back(button);
    }

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
    return searchEdit ? helpdlg::Borrow(searchEdit->GetText()) : h3_NullString;
}

void HeaderPage::SetQuery(const std::string &query)
{
    if (!searchEdit)
        return;
    searchEdit->SetText(query.c_str());
    RefreshSearchPlaceholder();
}

void HeaderPage::FocusSearch()
{
    if (!searchEdit)
        return;
    if (!SearchFocused())
        searchEdit->SetFocus(TRUE);
    RefreshSearchPlaceholder();
}
bool HeaderPage::SearchFocused() const noexcept
{
    return searchEdit && searchEdit->IsFocused();
}
bool HeaderPage::SearchContains(int dialogX, int dialogY) const noexcept
{
    return searchEdit && dialogX >= searchEdit->GetX() - kSearchPadding &&
           dialogX < searchEdit->GetX() + searchEdit->GetWidth() + kSearchPadding &&
           dialogY >= searchEdit->GetY() - 2 && dialogY < searchEdit->GetY() + searchEdit->GetHeight() + 2;
}
void HeaderPage::RefreshSearchPlaceholder() noexcept
{
    if (!searchEdit || !searchPlaceholder)
        return;
    const bool show = isVisible && !SearchFocused() && !*Query();
    if (show == (searchPlaceholder->IsVisible() != FALSE))
        return;
    if (show)
        searchPlaceholder->Show()->DeActivate();
    else
        searchPlaceholder->HideDeactivate();
    RedrawDialog();
}
void HeaderPage::ReleaseInputFocus(int keepItemId) noexcept
{
    auto *edit = searchEdit;
    if (edit && edit->IsFocused() && edit->GetID() != keepItemId)
        edit->SetFocus(FALSE);
    RefreshSearchPlaceholder();
}
} // namespace main
