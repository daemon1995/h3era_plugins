#include "pch.h"
#include "TestDlg.h"

TestDlg::TestDlg(int width, int height, int x, int y)
	:H3Dlg(width,height,x,y,false, false)
{

	this->CreateOKButton();
}

BOOL TestDlg::DialogProc(H3Msg& msg)
{
	return 1;
}

H3Vector<H3DlgItem*>& TestDlg::items()
{
	return this->dlgItems;
	// TODO: вставьте здесь оператор return
}

TestDlg::~TestDlg()
{

}
