#pragma once
#include <cstdint>

namespace rmginput
{
inline bool PointerDown(const h3::H3Msg &msg)
{
    return msg.command == h3::eMsgCommand::MOUSE_BUTTON &&
           (msg.subtype == h3::eMsgSubtype::LBUTTON_DOWN || msg.subtype == h3::eMsgSubtype::RBUTTON_DOWN);
}
inline bool RightClick(const h3::H3Msg &msg)
{
    // Native right-select notifications carry the right-button qualifier.
    // A control command with the same numeric subtype is not a mouse click.
    return msg.IsRightClick() && (static_cast<int>(msg.flags) & 0x200) != 0;
}
inline bool Edit(const h3::H3DlgItem *item)
{
    return item && *reinterpret_cast<const std::uintptr_t *>(item) == h3::H3DlgEdit::VTABLE;
}
inline bool Contains(const h3::H3DlgItem &item, const h3::H3Msg &msg)
{
    const int x = item.GetAbsoluteX(), y = item.GetAbsoluteY();
    return msg.GetX() >= x && msg.GetX() < x+item.GetWidth() &&
           msg.GetY() >= y && msg.GetY() < y+item.GetHeight();
}
template<class Finish> bool BlurOutside(h3::H3BaseDlg &dlg, const h3::H3Msg &msg, Finish finish)
{
    if (!PointerDown(msg) && !msg.ClickOutside()) return false;
    bool changed = false;
    for (auto *item : dlg.GetList()) if (Edit(item))
    {
        auto *edit = static_cast<h3::H3DlgEdit *>(item);
        // Native edits turn keyboard input into ITEM_COMMAND/LBUTTON_DOWN
        // addressed to themselves. Its coordinates are not a mouse click.
        if (!msg.ClickOutside() && msg.itemId == edit->GetID()) continue;
        if (edit->IsFocused() && (!edit->IsVisible() || !Contains(*edit,msg)))
        {
            edit->SetFocus(FALSE); finish(edit); changed = true;
        }
    }
    return changed;
}
template<class Finish> bool BlurAll(h3::H3BaseDlg &dlg, Finish finish)
{
    bool changed = false;
    for (auto *item : dlg.GetList()) if (Edit(item))
    {
        auto *edit = static_cast<h3::H3DlgEdit *>(item);
        if (edit->IsFocused()) { edit->SetFocus(FALSE); finish(edit); changed = true; }
    }
    return changed;
}
}
