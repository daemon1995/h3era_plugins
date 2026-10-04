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
constexpr int kMargin = helpdlg::kModDropdownMargin;
} // namespace

ModListDlg *ModListDlg::instance = nullptr;

ModListDlg::ModListDlg(const helpdlg::ModDropdownLayout &layout, const std::vector<ModInformation *> &mods,
                       const ModInformation *selectedMod)
    : H3Dlg(layout.width, layout.height, layout.x, layout.y, false, false), mods(mods), visibleCount(layout.rows)
{
    instance = this;
    // A dropdown uses a thin border instead of the full dialog's ornate frame.
    helpdlg::AddSafeBackground(*this, false, false);
    if (auto *border = H3DlgFrame::Create(0, 0, widthDlg, heightDlg, -1, H3RGB565(H3RGB888::Highlight())))
    {
        AddItem(border);
        border->DeActivate();
    }
    const auto selected = std::find(mods.begin(), mods.end(), selectedMod);
    if (selected != mods.end())
        selectedIndex = static_cast<int>(selected - mods.begin());
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
    const int listHeight = visibleCount * kRowHeight;
    const int count = static_cast<int>(mods.size());
    const bool scrolling = count > visibleCount;
    for (int row = 0; row < visibleCount; ++row)
    {
        auto *button =
            helpdlg::Button(this, kMargin, kMargin + row * kRowHeight, widthDlg - kMargin * 2 - (scrolling ? 18 : 0),
                            kRowHeight - 2, kFirstItemId + row, "");
        buttons.emplace_back(button);
    }
    const int maxFirst = std::max(0, count - visibleCount);
    if (maxFirst > 0)
    {
        scrollBar = H3DlgScrollbar::Create(widthDlg - kMargin - 16, kMargin, 16, listHeight, kFirstItemId - 1,
                                           maxFirst + 1, ScrollProc, false, 1, true);
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
            button->SetFrame(index == selectedIndex ? 1 : 0);
            button->ShowActivate();
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
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(mods.size()))
                resultMod = mods[selectedIndex];
            Stop();
            return FALSE;
        }
        if (key == eVKey::H3VK_UP || key == eVKey::H3VK_DOWN || key == eVKey::H3VK_HOME || key == eVKey::H3VK_END)
        {
            const int last = static_cast<int>(mods.size()) - 1;
            selectedIndex = helpdlg::Bound(key == eVKey::H3VK_HOME  ? 0
                                           : key == eVKey::H3VK_END ? last
                                                                    : selectedIndex + (key == eVKey::H3VK_UP ? -1 : 1),
                                           0, last);
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
