#pragma once
#include "DlgEnums.h"
#include "DlgPage.h"
namespace main
{
class HeaderPage final : public DlgPage
{
    std::vector<H3DlgCaptionButton *> tabs;
    H3DlgEdit *searchEdit = nullptr;

  public:
    void ReleaseInputFocus(int keepItemId = -1) noexcept;
    HeaderPage(int x, int y, int width, int height, H3Dlg *dialog);
    void SetActiveButton(int buttonId) noexcept;
    std::string Query() const;
    void SetQuery(const std::string &query);
    void FocusSearch();
};
} // namespace main
