#include "MenuItem.h"

void em::MenuItem::GetKey()
{
}

void em::MenuItem::SetupText()
{
}

H3String em::MenuItem::GetName(){	return Era::tr(this->key_ptr.AppendA(".name").String());}

H3String em::MenuItem::GetHint() { return Era::tr(this->key_ptr.AppendA(".hint").String()); }

H3String em::MenuItem::GetPopup() { return Era::tr(this->key_ptr.AppendA(".popup").String()); }
