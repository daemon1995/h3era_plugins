#include "framework.h"
namespace help
{
GuideDlg::GuideDlg(const int width, const int height, const int x, const int y)
    : H3Dlg(width, height, x, y, 1, 0)
{

    // create okay button with hotkeys
    auto okBttn = CreateOKButton();
    okBttn->AddHotkey(eVKey::H3VK_H);
    okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
}

GuideDlg::~GuideDlg()
{
}

BOOL GuideDlg::DialogProc(H3Msg &msg)
{

    return 0;
}
} // namespace help
