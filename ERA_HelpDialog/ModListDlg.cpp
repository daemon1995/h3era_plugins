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
                       const ModInformation *selectedMod)
    : H3Dlg(layout.width, layout.height, layout.x, layout.y, false, false), mods(mods), visibleCount(layout.rows)
{
    instance = this;
    flags &= ~16; // Draw the dropdown without a dialog shadow.
    helpdlg::AddSafeBackground(*this, false, false);
    const auto selected = std::find(mods.begin(), mods.end(), selectedMod);
    if (selected != mods.end())
        activeIndex = selectedIndex = static_cast<int>(selected - mods.begin());
    firstIndex = helpdlg::Bound(selectedIndex - visibleCount + 1, 0, static_cast<int>(mods.size()) - visibleCount);
    CreateDlgItems();
}

ModListDlg::~ModListDlg()
{
    if (instance == this)
        instance = nullptr;
}

void ModListDlg::CreateDlgItems()
{
    const int count = static_cast<int>(mods.size());
    for (int row = 0; row < visibleCount; ++row)
    {
        auto *button = helpdlg::Button(this, 0, row * kRowHeight, widthDlg, helpdlg::kPageButtonHeight,
                                       kFirstItemId + row, "", main::DlgPage::PAGE_BUTTON_DEF);
        buttons.emplace_back(button);
    }
    const int maxFirst = std::max(0, count - visibleCount);
    if (maxFirst > 0)
    {
        scrollBar = H3DlgScrollbar::Create(widthDlg - 16, 0, 16, heightDlg, kFirstItemId - 1, maxFirst + 1, ScrollProc,
                                           false, 1, true);
        AddItem(scrollBar);
    }
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
            const LPCSTR name = mod && !mod->name.Empty() ? mod->name.String() : "Unknown mod";
            button->SetText(name);
            button->SetHints(name, nullptr, false);
            const bool state = index == activeIndex;
            // Keyboard focus does not change which mod is already open.
            button->SetFrame(index == selectedIndex && !state ? 1 : 0);
            button->ShowActivate();
            button->SendCommand(6 - (state), 4096);
        }
        else if (button)
            button->HideDeactivate();
    }
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
            if (selectedIndex == activeIndex)
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
            if (nextIndex == activeIndex)
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
    if (msg.IsLeftClick() && row >= 0 && row < visibleCount && firstIndex + row < static_cast<int>(mods.size()))
    {
        if (firstIndex + row == activeIndex)
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
