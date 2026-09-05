#pragma once

#include "HelpDialogDependencies.h"

#include <vector>

namespace main
{

class HelpSection
{
  protected:
    int activeSubtype = 0;

  public:
    virtual ~HelpSection() = default;

    int Subtype() const noexcept
    {
        return activeSubtype;
    }
    virtual void SetVisible(BOOL state) noexcept = 0;
    virtual BOOL ProcessMessage(H3Msg &msg) = 0;
    virtual void Redraw() = 0;
};

// A page owns ordinary H3 dialog items registered in MainDlg. It is not an
// H3 dialog/panel and never draws directly to the screen.
class HelpPage
{
  protected:
    H3Dlg *dialog = nullptr;
    std::vector<H3DlgItem *> items;
    std::vector<H3DlgItem *> decorations;
    std::vector<H3DlgScrollableText *> scrollableTexts;
    BOOL isVisible = FALSE;

    explicit HelpPage(H3Dlg *dialog);
    void AddItem(H3DlgItem *item);
    void AddScrollableText(H3DlgScrollableText *scrollableText);
    void SetScrollableText(H3DlgScrollableText *scrollableText, LPCSTR text);
    void AddFrame(int x, int y, int width, int height);
    void RedrawDialog() const noexcept;

  private:
    void SetScrollableTextVisible(H3DlgScrollableText *scrollableText, BOOL state) const noexcept;

  public:
    virtual ~HelpPage() = default;

    virtual void SetVisible(BOOL state) noexcept;
    BOOL IsVisible() const noexcept;
};

} // namespace main
