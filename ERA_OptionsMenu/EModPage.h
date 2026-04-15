#pragma once
#include "pch.h"
#define PAGE_MAX_CATEGORIES 20
using namespace h3;
// namespace em
//{
// class EModPageCategory; //:public MenuItem {};
class EModPage : public MenuItem
{
  public:
    UINT8 index = -1;
    // EModPage();
    EModPage(LPCSTR name, MenuItem *parent, UINT8 index, LPCSTR hint = h3_NullString, LPCSTR popup = h3_NullString,
             LPCSTR tag = h3_NullString);
    virtual ~EModPage();
    void GetKey() override;
    void SetupText() override;

    //	std::vector<EModPageCategory> cats;// = { "", nullptr };

    // EMod* GetMod() { return reinterpret_cast<EMod*>(parent); }
};
//}
