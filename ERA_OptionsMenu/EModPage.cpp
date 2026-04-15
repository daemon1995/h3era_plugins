#include "EModPage.h"
EModPage::EModPage(LPCSTR name, MenuItem *parent, UINT8 index, LPCSTR hint, LPCSTR popup, LPCSTR tag)
    : MenuItem(name, parent, hint, popup), index(index)
{
    // cats.clear();
    //	cats.reserve(PAGE_MAX_CATEGORIES);
    //	this->name = name;
    //	this->popup = popup;
    //	this->hint = hint;
    // this->tag = tag;
}

EModPage::~EModPage()
{
    //	cats.clear();
}

void EModPage::GetKey()
{
    key_ptr = parent->key_ptr.PrintfAppend("page_%d.", index);
}

void EModPage::SetupText()
{
}
