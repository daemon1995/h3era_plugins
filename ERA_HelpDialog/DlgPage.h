#pragma once

#include "HelpDialogDependencies.h"

#include <vector>

namespace main
{

class DlgSection
{
  protected:
    int activeSubtype = 0;

  public:
    virtual ~DlgSection() = default;

    int Subtype() const noexcept
    {
        return activeSubtype;
    }
    virtual void SetVisible(BOOL state) noexcept = 0;
    // Called for mouse-over messages before the active page processes the
    // message. Pages with more than one scrollbar use it to decide which
    // scrollbar may consume wheel/drag input.
    virtual void UpdateMousePosition(const H3Msg &msg) noexcept
    {
        (void)msg;
    }
    virtual BOOL ProcessMessage(H3Msg &msg) = 0;
    virtual void Redraw() = 0;
};

// A page owns ordinary H3 dialog items registered in MainDlg. It is not an
// H3 dialog/panel and never draws directly to the screen.
class DlgPage
{
  protected:
    H3Dlg *dialog = nullptr;
    std::vector<H3DlgItem *> items;
    std::vector<H3DlgItem *> decorations;
    std::vector<H3DlgScrollableText *> scrollableTexts;
    BOOL isVisible = FALSE;

    explicit DlgPage(H3Dlg *dialog);
    void AddItem(H3DlgItem *item);
    void AddScrollableText(H3DlgScrollableText *scrollableText);
    void SetScrollableText(H3DlgScrollableText *scrollableText, LPCSTR text);
    void AddFrame(int x, int y, int width, int height);
    void RedrawDialog() const noexcept;

  private:
    void SetScrollableTextVisible(H3DlgScrollableText *scrollableText, BOOL state) const noexcept;

  public:
    virtual ~DlgPage() = default;

    virtual void SetVisible(BOOL state) noexcept;
    BOOL IsVisible() const noexcept;
};

} // namespace main
