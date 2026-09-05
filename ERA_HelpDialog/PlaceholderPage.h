#pragma once

#include "HelpPage.h"

namespace main
{

class PlaceholderSection final : public HelpSection, private HelpPage
{
    H3DlgScrollableText *text = nullptr;
    H3String title;

  public:
    PlaceholderSection(int x, int y, int width, int height, H3Dlg *dialog);

    void SetTitle(LPCSTR title);
    void SetVisible(BOOL state) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
