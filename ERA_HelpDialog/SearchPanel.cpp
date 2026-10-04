#include "SearchPanel.h"
#include "ScrollbarUtils.h"

namespace helpdlg
{
namespace
{
constexpr int kScroll = 28001, kBack = 28003;
constexpr int kScopeFirst = 28100, kRowFirst = 28200;
const main::eHelpPage kPages[] = {
    main::eHelpPage::CREATURES,        main::eHelpPage::ARTIFACTS, main::eHelpPage::HEROES, main::eHelpPage::SPELLS,
    main::eHelpPage::SECONDARY_SKILLS, main::eHelpPage::TOWNS,     main::eHelpPage::MODS,   main::eHelpPage::HOTKEYS};
} // namespace
SearchPanel *SearchPanel::visiblePanel = nullptr;
SearchPanel::SearchPanel(int cx, int top, int cw, int h, int left, int w, H3Dlg *parent,
                         std::vector<SearchEntry> source)
    : DlgPage(parent), entries(std::move(source)), x(left), y(top), width(w), height(h)
{
    // IDs are scoped by page and mod; never use translated names for order.
    std::stable_sort(entries.begin(), entries.end(), [](const SearchEntry &a, const SearchEntry &b) {
        if (a.page != b.page)
            return static_cast<int>(a.page) < static_cast<int>(b.page);
        if (a.modId != b.modId)
            return a.modId < b.modId;
        if (a.objectId != b.objectId)
            return a.objectId < b.objectId;
        if (a.category != b.category)
            return a.category < b.category;
        return a.hotkeyId < b.hotkeyId;
    });
    AddFrame(cx, y, cw, height);
    AddFrame(x, y, width, height);
    for (int index = 0; index < 9; ++index)
    {
        auto *button = Button(nullptr, cx + 6, y + 6 + index * 30, cw - 12, 28, kScopeFirst + index,
                              index ? PageName(kPages[index - 1]) : Text("help.ui.all", "All"));
        AddItem(button);
        scopes.push_back(button);
    }
    AddItem(Button(nullptr, x + width - 90, y + 6, 80, 28, kBack, Text("help.ui.back", "Back")));
    countText = H3DlgText::Create(x + 8, y + 6, width - 106, 28, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1,
                                  eTextAlignment::MIDDLE_LEFT);
    AddItem(countText);
    const int rowCount = std::max(1, (height - 48) / 34);
    for (int row = 0; row < rowCount; ++row)
    {
        auto *button = Button(nullptr, x + 8, y + 40 + row * 34, width - 34, 32, kRowFirst + row, "");
        if (button)
            button->SetHints(Text("help.ui.search_result_hint", "Left: go to and highlight. Right: hold for preview."),
                             nullptr, false);
        AddItem(button);
        rows.push_back(button);
    }
    scroll = H3DlgScrollbar::Create(x + width - 22, y + 40, 16, rowCount * 34, kScroll, 2, ScrollProc, false, 1, true);
    AddItem(scroll);
    Rebuild();
}
SearchPanel::~SearchPanel()
{
    if (visiblePanel == this)
        visiblePanel = nullptr;
}
void SearchPanel::SetQuery(const std::string &text)
{
    if (query == text)
        return;
    query = text;
    Rebuild();
}
void SearchPanel::SetVisible(BOOL state) noexcept
{
    DlgPage::SetVisible(state);
    if (state)
    {
        visiblePanel = this;
        Redraw();
    }
    else
    {
        if (visiblePanel == this)
            visiblePanel = nullptr;
    }
}
void SearchPanel::Rebuild()
{
    matches.clear();
    for (size_t index = 0; index < entries.size(); ++index)
        if ((!activeSubtype || entries[index].page == kPages[activeSubtype - 1]) &&
            MatchesWords(entries[index].searchable, query))
            matches.push_back(static_cast<int>(index));
    firstIndex = selectedIndex = 0;
    selection = -1;
    selectionPopup = false;
    Redraw();
}
void SearchPanel::Redraw()
{
    firstIndex = UpdateScrollbar(scroll, static_cast<int>(matches.size()) - static_cast<int>(rows.size()), firstIndex,
                                 isVisible);
    for (size_t row = 0; row < rows.size(); ++row)
    {
        auto *button = rows[row];
        if (!button)
            continue;
        const int index = firstIndex + static_cast<int>(row);
        if (isVisible && index < static_cast<int>(matches.size()))
        {
            button->SetText(entries[matches[index]].label.c_str());
            button->SetFrame(index == selectedIndex ? 1 : 0);
            button->ShowActivate();
        }
        else
            button->HideDeactivate();
    }
    for (size_t index = 0; index < scopes.size(); ++index)
        if (scopes[index])
            scopes[index]->SetFrame(index == static_cast<size_t>(activeSubtype) ? 1 : 0);
    if (countText)
        countText->SetText(
            H3String::Format("%d %s", static_cast<int>(matches.size()), Text("help.ui.results", "results")).String());
    RedrawDialog();
}
void SearchPanel::Accept()
{
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(matches.size()))
    {
        selection = matches[selectedIndex];
        selectionPopup = false;
    }
}
BOOL SearchPanel::ProcessMessage(H3Msg &msg)
{
    if (!isVisible)
        return FALSE;
    if (msg.IsKeyPress())
    {
        const auto key = msg.GetKey();
        if (key == eVKey::H3VK_ESCAPE)
        {
            backRequested = true;
            return TRUE;
        }
        if (key == eVKey::H3VK_ENTER)
        {
            Accept();
            return TRUE;
        }
        if (key == eVKey::H3VK_UP || key == eVKey::H3VK_DOWN)
        {
            selectedIndex =
                Bound(selectedIndex + (key == eVKey::H3VK_UP ? -1 : 1), 0, static_cast<int>(matches.size()) - 1);
            if (selectedIndex < firstIndex)
                firstIndex = selectedIndex;
            if (selectedIndex >= firstIndex + static_cast<int>(rows.size()))
                firstIndex = selectedIndex - static_cast<int>(rows.size()) + 1;
            Redraw();
            return TRUE;
        }
    }
    if (!msg.IsLeftClick() && !msg.IsRightClick())
        return FALSE;
    const int row = msg.itemId - kRowFirst;
    if (row >= 0 && row < static_cast<int>(rows.size()) && firstIndex + row < static_cast<int>(matches.size()))
    {
        if (msg.IsRightClick())
        {
            selection = matches[firstIndex + row];
            selectionPopup = true;
        }
        else
        {
            selectedIndex = firstIndex + row;
            Accept();
        }
        return TRUE;
    }
    if (!msg.IsLeftClick())
        return FALSE;
    if (msg.itemId == kBack)
    {
        backRequested = true;
        return TRUE;
    }
    if (msg.itemId >= kScopeFirst && msg.itemId < kScopeFirst + 9)
    {
        activeSubtype = msg.itemId - kScopeFirst;
        Rebuild();
        return TRUE;
    }
    return FALSE;
}
void SearchPanel::UpdateMousePosition(const H3Msg &msg) noexcept
{
    if (!scroll || !scroll->IsVisible() || !msg.IsMouseOver())
        return;
    const int mx = msg.GetX() - dialog->GetX(), my = msg.GetY() - dialog->GetY();
    if (mx >= x && mx < x + width && my >= y + 40 && my < y + height)
        scroll->Activate();
    else
        scroll->DeActivate();
}
void __fastcall SearchPanel::ScrollProc(INT32 tick, H3BaseDlg *)
{
    if (visiblePanel)
    {
        visiblePanel->firstIndex = tick;
        visiblePanel->Redraw();
    }
}
const SearchEntry *SearchPanel::TakeSelection(bool &popup)
{
    const int result = selection;
    popup = selectionPopup;
    selection = -1;
    selectionPopup = false;
    return result >= 0 && result < static_cast<int>(entries.size()) ? &entries[result] : nullptr;
}
bool SearchPanel::TakeBackRequest()
{
    const bool result = backRequested;
    backRequested = false;
    return result;
}
} // namespace helpdlg
