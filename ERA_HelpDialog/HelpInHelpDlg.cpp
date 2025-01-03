#include "framework.h"
namespace help
{
	HelpInHelpDlg::HelpInHelpDlg(const int width, const int height, const int x, const int y)
		:H3Dlg(width, height, x, y, 1, 0)
	{


		// create okay button with hotkeys
		auto okBttn=CreateOKButton();
		okBttn->AddHotkey(eVKey::H3VK_H);
		okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
	}

	HelpInHelpDlg::~HelpInHelpDlg()
	{

	}

	BOOL HelpInHelpDlg::DialogProc(H3Msg& msg)
	{

		return 0;
	}
}