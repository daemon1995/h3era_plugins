#include "framework.h"
namespace main
{
	namespace buttons
	{
		enum eButton
		{
			HELP,
			OK,
			MODLIST
		};
	}


	HelpDlg* HelpDlg::instance = nullptr;
	HelpDlg::HelpDlg(const int width, const int height, const int x, const int y)
		:H3Dlg(width, height, x, y, 1, 0)
	{
		

		// set black background
		background =  H3LoadedPcx16::Create(h3_NullString,width, height);
		memset(background->buffer, 0, background->buffSize);

		H3DlgPcx16* bg = H3DlgPcx16::Create(0, 0, background->width, background->height, 0, nullptr);
		bg->SetPcx(background);

		AddItem(bg);

		// create "ok"

		const int okX = width - 70;
		const int okY = height - 55;

		auto bttn = this->CreateOK32Button(okX, okY);
		bttn->AddHotkey(eVKey::H3VK_ESCAPE);
		bttn->AddHotkey(59);

		// create hint
		hintBar = CreateHint(20, okY, okX - 40, bttn->GetHeight());


	}

	HelpDlg::~HelpDlg()
	{

	}

	BOOL HelpDlg::DialogProc(H3Msg& msg)
	{

		return 0;
	}



}
