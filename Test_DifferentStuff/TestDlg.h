#pragma once
#include "framework.h"
using namespace h3;

class TestDlg : public H3Dlg
{
    H3DlgPcx16 *skillsDlgPcx[h3::limits::SECONDARY_SKILLS] = {nullptr};
  public:
    TestDlg(int width, int height, int x = -1, int y = -1);

    BOOL DialogProc(H3Msg &msg) override;
    H3Vector<H3DlgItem *> &items();
    virtual ~TestDlg();
};
