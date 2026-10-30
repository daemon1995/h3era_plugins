#pragma once
#include "DlgEnums.h"
#include "DlgPage.h"
namespace main
{
class HeaderPage final : public DlgPage
{
    std::vector<H3DlgCaptionButton *> tabs;
    H3DlgCaptionButton *modButton = nullptr;
    H3DlgEdit *searchEdit = nullptr;
    H3DlgText *searchPlaceholder = nullptr;

  public:
    virtual ~HeaderPage() override = default;
    void ReleaseInputFocus(int keepItemId = -1) noexcept;
    HeaderPage(int x, int y, int width, int height, int leftWidth, H3Dlg *dialog);
    void SetActiveButton(int buttonId, LPCSTR modName = nullptr) noexcept;
    LPCSTR Query() const;
    void SetQuery(const std::string &query);
    void FocusSearch();
    bool SearchFocused() const noexcept;
    bool SearchContains(int dialogX, int dialogY) const noexcept;
    void RefreshSearchPlaceholder() noexcept;
};
} // namespace main
