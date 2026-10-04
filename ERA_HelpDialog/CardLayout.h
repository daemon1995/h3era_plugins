#pragma once
#include "HelpUI.h"
namespace helpdlg
{
inline int CardTextHeight(H3Font *font, const std::string &text, int width)
{
    const int lines = font && !text.empty() ? font->GetLinesCountInText(text.c_str(), std::max(1, width)) : 1;
    const int lineHeight = font ? std::max(1, static_cast<int>(font->height)) + 1 : 14;
    return Bound(lines, 1, 1000) * lineHeight;
}
inline void AddCardCell(H3Dlg &dlg, const std::string &text, int x, int y, int width, int height, int requiredHeight)
{
    if (requiredHeight <= height)
        dlg.CreateText(x, y, width, height, text.c_str(), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1,
                       eTextAlignment::TOP_LEFT);
    else if (auto *cell = H3DlgScrollableText::Create(text.c_str(), x, y, width, height, NH3Dlg::Text::SMALL,
                                                      eTextColor::REGULAR, false))
        dlg.AddItem(cell);
}
} // namespace helpdlg
