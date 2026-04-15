#pragma once
// #include "MenuItem.h"
#include "pch.h"
#define MOD_MAX_PAGES 20

// class EModPage;
class EMod : public MenuItem
{

    H3Vector<int> opt_id;
    LPCSTR tag = nullptr;

  public:
    //		std::vector<EModPage> pages;

    // EMod();
    EMod(LPCSTR name, MenuItem *parent = nullptr, LPCSTR hint = h3_NullString, LPCSTR popup = h3_NullString,
         LPCSTR tag = h3_NullString);

    void FillPages();
    void GetKey() override;
    virtual ~EMod();
};
/** static EMod& Create(H3String name, H3String popup = "", H3String hint = "", H3String tag = "")
{
    return *new EMod(name, nullptr, hint, popup, tag);
}**/
