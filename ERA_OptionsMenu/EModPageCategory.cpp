#include "EModPageCategory.h"

EModPageCategory::EModPageCategory(LPCSTR name, MenuItem *parent, LPCSTR hint, LPCSTR popup, LPCSTR tag)
    : MenuItem(name, parent, hint, popup)

{
    //
    // options.clear();
    //	options.reserve(150);

    //	this->name = name;
    //	this->popup = popup;
    //	this->hint = hint;
    // this->tag = tag;
}

EModPageCategory::~EModPageCategory()
{
    //	options.clear();
}
void EModPageCategory::SetupText()
{
    this->SetEnabled(false);
}
