#include "framework.h"
namespace list
{
	ModListDlg::ModListDlg(const int width, const int height, const int x, const int y)
		:H3Dlg(width, height, x, y, 1, 0)
	{

		CreateOKButton();


	}

	ModListDlg::~ModListDlg()
	{

	}

	BOOL ModListDlg::DialogProc(H3Msg& msg)
	{

		return 0;
	}

	main::Mod* ModListDlg::ResultMod() const noexcept
	{
		return nullptr;
	}

}

