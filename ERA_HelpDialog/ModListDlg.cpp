#include "ModListDlg.h"
#include "ScrollbarUtils.h"

#include <algorithm>

namespace list
{
namespace
{
constexpr int kFirstItemId = 600;
constexpr int kRowHeight = 34;
}

ModListDlg *ModListDlg::instance = nullptr;

ModListDlg::ModListDlg(const int width, const int height, const int x, const int y,
                       const std::vector<ModInformation *> &mods)
    : H3Dlg(width, height, x, y, false, false), mods(mods)
{
    instance = this;
    CreateDlgItems();
}

ModListDlg::~ModListDlg()
{
    if (instance == this)
        instance = nullptr;
}

void ModListDlg::CreateDlgItems()
{
    constexpr int margin = 12;
    const int listHeight = std::max(kRowHeight, heightDlg - 2 * margin);
    visibleCount = std::max(1, listHeight / kRowHeight);
    const int count = static_cast<int>(mods.size());
    for (int index = 0; index < count; ++index)
    {
        const LPCSTR modName = mods[index] && !mods[index]->name.Empty() ? mods[index]->name.String() : "Unknown mod";
        auto *button = H3DlgCaptionButton::Create(margin, margin + index * kRowHeight, widthDlg - margin * 2 - 18,
                                                  kRowHeight - 4, kFirstItemId + index, "OVBUTN3.def",
                                                  modName,
                                                  NH3Dlg::Text::SMALL, 0, 0, false, static_cast<eVKey>(0),
                                                  eTextColor::REGULAR);
        if (button)
        {
            button->SetClickFrame(1);
            AddItem(button);
        }
        buttons.emplace_back(button);
    }
    const int maxFirst = std::max(0, count - visibleCount);
    if (maxFirst > 0)
    {
        scrollBar = H3DlgScrollbar::Create(widthDlg - margin - 16, margin, 16, listHeight, kFirstItemId - 1,
                                            maxFirst + 1, ScrollProc, false, 1, true);
        AddItem(scrollBar);
    }
    RedrawItems(0);
}

void ModListDlg::RedrawItems(const int requestedFirstIndex)
{
    const int maxFirst = std::max(0, static_cast<int>(mods.size()) - visibleCount);
    firstIndex = helpdlg::UpdateScrollbar(scrollBar, maxFirst, requestedFirstIndex, TRUE);
    for (size_t index = 0; index < buttons.size(); ++index)
    {
        auto *button = buttons[index];
        const int row = static_cast<int>(index) - firstIndex;
        if (button && row >= 0 && row < visibleCount)
        {
            button->SetY(12 + row * kRowHeight);
            button->ShowActivate();
        }
        else if (button)
            button->HideDeactivate();
    }
}

void __fastcall ModListDlg::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->RedrawItems(tick);
}

BOOL ModListDlg::DialogProc(H3Msg &msg)
{
    if (msg.ClickOutside())
    {
        Stop();
        return 0;
    }
    if (msg.IsLeftClick() && msg.itemId >= kFirstItemId && msg.itemId < kFirstItemId + static_cast<int>(mods.size()))
    {
        resultMod = mods[msg.itemId - kFirstItemId];
        Stop();
    }
    return 0;
}

ModInformation *ModListDlg::ResultMod() const noexcept
{
    return resultMod;
}

} // namespace list
