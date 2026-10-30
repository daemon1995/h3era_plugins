#include "ModListDlg.h"
#include "ScrollbarUtils.h"
#include "HelpUI.h"

#include <algorithm>

namespace list
{
namespace
{
constexpr int kFirstItemId = 600;
constexpr int kRowHeight = helpdlg::kModDropdownRowHeight;
} // namespace

ModListDlg *ModListDlg::instance = nullptr;

ModListDlg::ModListDlg(const helpdlg::ModDropdownLayout &layout, const std::vector<ModInformation *> &mods,
                       const ModInformation *selectedMod, bool selectedModIsOpen)
    : H3Dlg(layout.width, layout.height, layout.x, layout.y, false, false), mods(mods), visibleCount(layout.rows)
{
    instance = this;
    flags &= ~16; // Draw the dropdown without a dialog shadow.
    helpdlg::AddSafeBackground(*this, false);
    const auto selected = std::find(mods.begin(), mods.end(), selectedMod);
    if (selected != mods.end())
    {
        selectedIndex = static_cast<int>(selected - mods.begin());
        if (selectedModIsOpen)
            disabledIndex = selectedIndex;
    }
    firstIndex = helpdlg::Bound(selectedIndex - visibleCount + 1, 0, static_cast<int>(mods.size()) - visibleCount);
    CreateDlgItems();
}

ModListDlg::~ModListDlg()
{
    for (auto *button : buttons)
        helpdlg::ReleaseSelectorText(button);
    if (instance == this)
        instance = nullptr;
}

void ModListDlg::CreateDlgItems()
{
    const int count = static_cast<int>(mods.size());
    for (int row = 0; row < visibleCount; ++row)
    {
        const int rowWidth = widthDlg - (count > visibleCount ? 16 : 0);
        auto *button =
            helpdlg::SelectorText(0, row * kRowHeight, rowWidth, helpdlg::kPageButtonHeight, kFirstItemId + row);
        if (button)
            AddItem(button);
        buttons.emplace_back(button);
    }
    const int maxFirst = std::max(0, count - visibleCount);
    if (maxFirst > 0)
    {
        scrollBar = H3DlgScrollbar::Create(widthDlg - 16, 0, 16, heightDlg, kFirstItemId - 1, maxFirst + 1, ScrollProc,
                                           false, 1, true);
        AddItem(scrollBar);
    }
    helpdlg::GoldFrame(*this, 0, 0, widthDlg, heightDlg);
    hoverFrame = helpdlg::GoldFrame(*this, 0, 0, widthDlg, helpdlg::kPageButtonHeight);
    if (hoverFrame)
        hoverFrame->HideDeactivate();
    RedrawItems(firstIndex);
}

void ModListDlg::RedrawItems(const int requestedFirstIndex)
{
    const int maxFirst = std::max(0, static_cast<int>(mods.size()) - visibleCount);
    firstIndex = helpdlg::UpdateScrollbar(scrollBar, maxFirst, requestedFirstIndex, TRUE);
    for (size_t row = 0; row < buttons.size(); ++row)
    {
        auto *button = buttons[row];
        const int index = firstIndex + static_cast<int>(row);
        if (button && index < static_cast<int>(mods.size()))
        {
            const auto *mod = mods[index];
            const LPCSTR name = mod->name;
            button->SetText(name);
            button->SetHints(name, nullptr, false);
            const bool state = index == disabledIndex;
            button->ShowActivate();
            if (state)
                button->Disable();
            else
                button->Enable();
        }
        else if (button)
            button->HideDeactivate();
    }
    const int hoverIndex = firstIndex + hoverRow;
    helpdlg::PlaceSelectorFrame(hoverFrame, hoverRow >= 0 && hoverRow < static_cast<int>(buttons.size()) &&
                                                    hoverIndex != disabledIndex
                                                ? buttons[hoverRow]
                                                : nullptr);
    if (P_WindowManager->lastDlg == this)
        Redraw();
}

void __fastcall ModListDlg::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->RedrawItems(tick);
}

BOOL ModListDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsMouseOver())
    {
        const int mouseX = msg.GetX() - GetX(), mouseY = msg.GetY() - GetY();
        const int rowWidth = widthDlg - (scrollBar ? 16 : 0);
        int row = mouseX >= 0 && mouseX < rowWidth && mouseY >= 0 && mouseY < heightDlg ? mouseY / kRowHeight : -1;
        if (row >= 0 && mouseY % kRowHeight >= helpdlg::kPageButtonHeight)
            row = -1;
        if (hoverRow != row)
        {
            hoverRow = row;
            if (row >= 0 && firstIndex + row != disabledIndex)
                selectedIndex = firstIndex + row;
            RedrawItems(firstIndex);
        }
        return FALSE;
    }
    if (msg.IsKeyPress())
    {
        const auto key = msg.GetKey();
        if (key == eVKey::H3VK_ESCAPE)
        {
            Stop();
            return FALSE;
        }
        if (key == eVKey::H3VK_ENTER)
        {
            if (selectedIndex == disabledIndex)
                return FALSE;
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(mods.size()))
                resultMod = mods[selectedIndex];
            Stop();
            return FALSE;
        }
        if (key == eVKey::H3VK_UP || key == eVKey::H3VK_DOWN || key == eVKey::H3VK_HOME || key == eVKey::H3VK_END)
        {
            const int last = static_cast<int>(mods.size()) - 1;
            int nextIndex = helpdlg::Bound(key == eVKey::H3VK_HOME  ? 0
                                           : key == eVKey::H3VK_END ? last
                                                                    : selectedIndex + (key == eVKey::H3VK_UP ? -1 : 1),
                                           0, last);
            if (nextIndex == disabledIndex)
            {
                const int direction = key == eVKey::H3VK_HOME || key == eVKey::H3VK_DOWN ? 1 : -1;
                const int alternative = nextIndex + direction;
                nextIndex = alternative >= 0 && alternative <= last ? alternative : selectedIndex;
            }
            selectedIndex = nextIndex;
            if (selectedIndex < firstIndex)
                firstIndex = selectedIndex;
            else if (selectedIndex >= firstIndex + visibleCount)
                firstIndex = selectedIndex - visibleCount + 1;
            hoverRow = selectedIndex - firstIndex;
            RedrawItems(firstIndex);
            return FALSE;
        }
    }
    if (msg.ClickOutside())
    {
        Stop();
        return FALSE;
    }
    const int row = msg.itemId - kFirstItemId;
    if (msg.IsLeftDown() && row >= 0 && row < visibleCount && firstIndex + row < static_cast<int>(mods.size()))
    {
        if (firstIndex + row == disabledIndex)
            return FALSE;
        resultMod = mods[firstIndex + row];
        Stop();
        return FALSE;
    }
    return TRUE;
}

ModInformation *ModListDlg::ResultMod() const noexcept
{
    return resultMod;
}

} // namespace list
