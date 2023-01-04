#pragma once
//#include "header.h"
#include "EMod.cpp"
//#include "EModPage.h"
//#include "EModPageCategory.h"
//#include "EOption.h"
#define MAX_OPTION_COUNT 1000
using namespace h3;
class EraMenuDlg :
	public H3Dlg
{

	bool need_restart = false;

public:
	INT16 options[MAX_OPTION_COUNT] = {0};
	int ratio = 1;
	std::vector<em::EMod> mods;

	EraMenuDlg(int width, int height, int x = -1, int y = -1,
		BOOL statusBar = 0, BOOL makeBackGround = 1, INT32 colorIndex = IntAt(0x69CCF4)) :H3Dlg(width, height, x, y, statusBar, makeBackGround, colorIndex)
	{
		mods.clear();//mods = dlg_mods;
	}
	bool NeedRestart() const;
	~EraMenuDlg()
	{
		mods.clear();
	}
	//void 

	BOOL DialogProc(H3Msg& msg) override;
	BOOL OnLeftClick(INT itemId, H3Msg& msg) override;
	BOOL OnLeftClickOutside() override;
	BOOL OnMouseWheel(INT32 direction) override;
	BOOL OnNotify(H3DlgItem* it, H3Msg& msg) override;
	//BOOL OnM
//	BOOL OnRightClickOutside() override;
//	BOOL OnRightClick(H3DlgItem* it) override;

	
};


