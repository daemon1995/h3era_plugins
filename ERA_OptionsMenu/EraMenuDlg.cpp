#include "pch.h"
extern std::vector<H3String> modList;

EraMenuDlg::EraMenuDlg(int width, int height, std::vector<H3String>& modNames)// int x, int y, BOOL statusBar, BOOL makeBackGround, INT32 colorIndex)
	:H3Dlg(width, height,-1,-1,1,1)//, -1, -1, false, makeBackGround, colorIndex)
{



	// create dlg mmods based on the loaded mods (/Mods/list.txt)
	const int SIZE = modNames.size();
	//mods.reserve(SIZE);
	for (auto& name: modNames)
	{
	//	mods.emplace_back(new EMod(name, nullptr));
	}


	CreateWidgets(); // init base dlg view
	//mods.clear();//mods = dlg_mods;
} 

bool EraMenuDlg::NeedRestart() const { return m_needRestart; }

EraMenuDlg::~EraMenuDlg()
{
	mods.clear();
}
void Debug(int a)
{
	H3Messagebox(Era::IntToStr(a).c_str());

}
BOOL EraMenuDlg::DialogProc(H3Msg& msg)
{


	if (this->focusedItemId == m_searchWidget->GetID()
		&& msg.KeyPressed())
	{
	//	m_searchWidget->Draw();
		//m_searchWidget->Refresh();
		m_searchWidget->ParentRedraw();
		Redraw();
		GetHintBar()->ShowMessage("");
	}
	if (msg.IsLeftDown()
		&& msg.ItemAtPosition(this) == m_searchWidget
		//::ITEM_COMMAND
	//	&& msg.ItemAtPosition(this)==m_searchWidget)
		)
	{
		//Debug(msg.command);
		//m_searchWidget->SetCaret(1);
		//this->focusedItemId = m_searchWidget->GetID();
		//m_searchWidget->Draw();
		m_searchWidget->autoRedraw = 1;
	//	m_searchWidget->Refresh();
		m_searchWidget->Refresh();


	}

	if (msg.IsLeftClick())
	{
		if (msg.itemId >= 1 && msg.itemId <= 8)
		{




		//	if (H3RGB565 rgb = this->GetFrame(msg.itemId)->GetColor())
		//	{
		//		BYTE r = rgb.GetRed8();
		//		BYTE g = rgb.GetGreen8();
		//		BYTE b = rgb.GetBlue8();

			//	this->background->FillRectangle(0 + 12, 0 + 12, this->widthDlg - 24, this->heightDlg - 24, r, g, b);
				//this->background->
			//	this->Redraw();
			//	H3DlgItem* fr = this->GetH3DlgItem(msg.itemId);
				//fr->Cast<H3DlgFrame>()->no
		//	}

		}
		else if (msg.itemId>=h3::eControlId::OK)
		{
			this->Stop();
		}
	}


	return 0;
}
 
//BOOL EraMenuDlg::OnLeftClick(INT itemId, H3Msg& msg)
//{
//	if (msg.subtype == eMsgSubtype::LBUTTON_CLICK && itemId >= 110 && itemId <= 115)
//	{
//		m_needRestart = true;
//		m_ratio = itemId - 110 +1;
//		this->Stop();
//		//this->widthDlg = itemi;
//	}
//
//	return 0;
//}
//
//BOOL EraMenuDlg::OnLeftClickOutside()
//{
//	//this->Stop();	
//	return 1;
//}
//
//BOOL EraMenuDlg::OnMouseWheel(INT32 direction)
//{
//	//this->GetScrollbar(40)->Refresh();
//	return 0;
//}
//


constexpr UINT16 SCROLL_BAR_ID = 40;

#define o_WogOptions ((_DlgSetup_*)0x2918390)
#define o_ChooseFile ((_ChooseFile_*)0x7B3614)
//H3DlgScrollbar_proc (Era_Dlg_Scroll_Proc, H3BaseDlg*)
void __fastcall Era_Dlg_Scroll_Proc(INT32 a1, H3BaseDlg* dlg) { dlg->GetScrollbar(SCROLL_BAR_ID)->Refresh(); }//return;


BOOL EraMenuDlg::CreateWidgets()
{
	for (INT8 i = 0; i < 4; i++)
	{
		H3DlgDefButton* bttn = H3DlgDefButton::Create(i * 60 + 30, 40, 110 + i, "icm003.def", 0, 1, 0, eVKey::H3VK_1 + i);

		AddItem(bttn);
		for (size_t j = 0; j < 20; j++)
		{
			//	H3DlgFrame* fr = H3DlgFrame::Create(32 *j +24, i * 60 + 24, 24, 24, j* i +j + 1, h3::H3RGB565::Black() + i * 12 + (j * 5 - i));
			//	AddItem(fr);
		}
	}
	constexpr int step = 1;
	H3DlgScrollbar* scroll = H3DlgScrollbar::Create(INT32(widthDlg * 0.25f), 14, 16, heightDlg - 46, SCROLL_BAR_ID, 6, Era_Dlg_Scroll_Proc, false, step, true);
	AddItem(scroll);
	//h3dlgc

	//	H3Messagebox(Era::tr(dlg_mods[dlg_mods.size()-1].name.String()));
	m_searchWidget = this->CreateEdit(600, 25, 150, 25, 25, "Search",h3::NH3Dlg::Text::MEDIUM,1,5,0,777);
	m_searchWidget->SetCaret(11);
	//AddItem(scroll);
	//dlg.CreateOK32Button(200, 200);
	CreateOKButton();

	GetDefButton(eControlId::OK)->AddHotkey(eVKey::H3VK_E);
	CreateCancelButton(100, 100);

	return EXIT_SUCCESS;

}


int MenuItem::item_id = 100;

 
BOOL EraMenuDlg::Create()
{

	int m_ratio = 1;

	bool needResize = false;

	int gameWidth = H3GameWidth::Get();
	int gameHeight = H3GameHeight::Get();


	int width = gameWidth, height = gameHeight, x = -1, y = -1;

	if (m_ratio > 1 && m_ratio < 4)
	{
		width = gameWidth - (gameWidth - 800) / (m_ratio ^ 1);
		height = gameHeight - (gameHeight - 600) / (m_ratio ^ 1);
	}
	if (m_ratio == 4)
	{
		width = 800;
		height = 600;
	}
	
	//_PI->WriteByte(0x728540, 2);
	// 
	// create dlg mods based on the game loaded mods
	EraMenuDlg* ptr = 0;
	do
	{
	
		{
			EraMenuDlg dlg(width, height, modList);
			ptr = &dlg;
			dlg.Start();


			m_ratio = dlg.m_ratio;
			needResize = dlg.NeedRestart();
		}
		
		ptr =0;

	} while (needResize);



	return false;
}

void __stdcall Dlg_WoG_Options_Show(HiHook* hook, int a1)
{
	EraMenuDlg::Create();

	return;
}

//PatcherInstance* _PI = nullptr;
BOOL EraMenuDlg::CreateMods(EMod& mod, int i)
{
	H3DlgText* txt;
	for (auto &m : mods)
	{
		txt = H3DlgText::Create(50, 50 + i * 50, 200, 25, m->GetName().String(), NH3Dlg::Text::BIG, 1, mod.GetId(), eTextAlignment::HLEFT);
		AddItem(txt);
	}
	 
	//H3DlgCaptionButton *bttn =H3DlgCaptionButton
	//"OVBUTN2.def"// up/duwn btn


	return 0;
}
