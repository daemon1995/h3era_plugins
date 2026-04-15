#include "MenuItem.h"

MenuItem::MenuItem()
{
}

MenuItem::MenuItem(LPCSTR name, MenuItem *parent, LPCSTR hint, LPCSTR popup)
    : name(name), parent(parent), hint(hint), popup(popup)
{
    // parent = nullptr;
    if (parent != nullptr)
        key_ptr = parent->key_ptr.PrintfAppendH3(".%s", name);
    else
        // key_ptr = BASE_KEY + name;
        key_ptr = "null";
    GetKey();
    enabled = true;
    id = item_id++;
}

void MenuItem::SendCommand(h3::H3DlgItem &it, h3::H3Msg &msg)
{
}

MenuItem *MenuItem::GetParent() const noexcept
{
    return parent;
}

void MenuItem::GetKey()
{
}

void MenuItem::SetupText()
{
}
void MenuItem::SetEnabled(bool state) noexcept
{
    enabled = state;
}
LPCSTR MenuItem::GetName()
{
    return Era::tr(this->key_ptr.AppendA(".name").String());
}

LPCSTR MenuItem::GetHint()
{
    return Era::tr(this->key_ptr.AppendA(".hint").String());
}

LPCSTR MenuItem::GetPopup()
{
    return Era::tr(this->key_ptr.AppendA(".popup").String());
}
MenuItem::~MenuItem()
{
    // delete parent;
    --item_id;
}
