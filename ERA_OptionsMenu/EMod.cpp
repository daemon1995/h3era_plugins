#include "EMod.h"

EMod::EMod(LPCSTR name, MenuItem *parent, LPCSTR hint, LPCSTR popup, LPCSTR tag)
    : MenuItem(name, parent, hint, popup), tag(tag)
{
    //	pages.clear();
    //	pages.reserve(MOD_MAX_PAGES);
    //	this->name = name;
    //	this->popup = popup;
    //	this->hint = hint;
    // this->tag = tag;
}

void EMod::FillPages()
{
    for (size_t i = 0; i < MOD_MAX_PAGES; i++)
    {
        H3String str = key_ptr.PrintfAppend(".%s.page_%d.name", name,
                                            i); // +name.Append("page._").Append(i).Append(".name").PrintfAppend;

        if (!EraJS::isEmpty(str.String()))
        {
        }
    }
}

void EMod::GetKey()
{
    key_ptr = "2";
}

EMod::~EMod()

{
    //	pages.clear();
}

// void em::EMod::SetupText()
//{
// }
