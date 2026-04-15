#pragma once
#include "pch.h"

// namespace em {
class EModPageCategory : public MenuItem
{
  public:
    // std::vector<EOption*> options;

    EModPageCategory(LPCSTR name, MenuItem *parent, LPCSTR hint = h3_NullString, LPCSTR popup = h3_NullString,
                     LPCSTR tag = h3_NullString);
    ~EModPageCategory();
    void SetupText() override;
};

//}
