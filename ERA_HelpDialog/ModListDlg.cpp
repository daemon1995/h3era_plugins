#include "framework.h"
namespace list
{
ModListDlg::ModListDlg(const int width, const int height, const int x, const int y)
    : H3Dlg(width, height, x, y, false, false)
{

    CreateOKButton();
}

ModListDlg::~ModListDlg()
{
}

BOOL ModListDlg::DialogProc(H3Msg &msg)
{

    if (msg.ClickOutside())
    {
        Stop();
    }

    return 0;
}

ModInformation *ModListDlg::ResultMod() const noexcept
{
    return nullptr;
}

} // namespace list
