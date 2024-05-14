#include "pch.h"

MenuItem::MenuItem()
{
}

MenuItem::MenuItem(H3String name, MenuItem* parent, H3String hint, H3String popup)
	: name(name), parent(parent), hint(hint), popup(popup)
{
	//parent = nullptr;
	if (parent != nullptr)
		key_ptr = parent->key_ptr.PrintfAppendH3(".%s", name);
	else
		//key_ptr = BASE_KEY + name;
		key_ptr = "null";
	GetKey();
	enabled = true;
	id = item_id++;

}

void MenuItem::SendCommand(h3::H3DlgItem& it, h3::H3Msg& msg)
{
}

MenuItem* MenuItem::GetParent() const noexcept
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
H3String MenuItem::GetName(){	return Era::tr(this->key_ptr.AppendA(".name").String());}

H3String MenuItem::GetHint() { return Era::tr(this->key_ptr.AppendA(".hint").String()); }

H3String MenuItem::GetPopup() { return Era::tr(this->key_ptr.AppendA(".popup").String()); }

MenuItem::~MenuItem()
{
		//delete parent;
	--item_id;
}
