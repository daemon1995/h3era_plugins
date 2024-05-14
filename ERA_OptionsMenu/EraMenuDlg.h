#pragma once
//#include "header.h"
#include "pch.h"

//#include "EModPage.h"
//#include "EModPageCategory.h"
//#include "EOption.h"
constexpr int MAX_OPTION_COUNT = 1000;
using namespace h3;
struct EMod;
class EraMenuDlg :
	public H3Dlg, MenuItem 
{

	bool m_needRestart = false;
	H3String m_searchInput;

	H3DlgEdit* m_searchWidget;
public:
	INT16 options[MAX_OPTION_COUNT]{};
	int m_ratio{};
	std::vector<EMod*> mods;

	EraMenuDlg(int width, int height, std::vector<H3String> &modNames);
	bool NeedRestart() const;
	virtual ~EraMenuDlg();

	//void 

	BOOL DialogProc(H3Msg& msg) override;
	//BOOL OnLeftClick(INT itemId, H3Msg& msg) override;
	//BOOL OnLeftClickOutside() override;
	//BOOL OnMouseWheel(INT32 direction) override;
	//BOOL OnNotify(H3DlgItem* it, H3Msg& msg) override;

	
	BOOL CreateMods(EMod& mod, int i);


	BOOL CreateWidgets();
	//BOOL InitMods(std::vector<H3String>& modNames);

	static BOOL Create();


	//BOOL OnM
//	BOOL OnRightClickOutside() override;
//	BOOL OnRightClick(H3DlgItem* it) override;

	
};


