#pragma once

#include "DlgEnums.h"
#include "DlgPage.h"

namespace main
{

class HeaderPage final : public DlgPage
{
  public:
    static constexpr LPCSTR defName = "OVBUTN3.def";

  protected:
    H3DlgCaptionButton *buttons[6] = {};
    H3DlgCaptionButton *utilityButtons[4] = {};
    H3DlgCaptionButton *activeButton = nullptr;

  public:
    HeaderPage(int x, int y, int width, int height, H3Dlg *dialog);

    void SetActiveButton(int buttonId) noexcept;
};

} // namespace main
