
//#include "header.h"
#include "EraMenuDlg.cpp"
#include <list>
#include <set>
#include <cctype>
//#include "era.h"
std::vector<H3String> dlg_mods(0);

using namespace h3;
using namespace em;

#define SCROLL_BAR_ID 40
#define o_WogOptions ((_DlgSetup_*)0x2918390)
#define o_ChooseFile ((_ChooseFile_*)0x7B3614)
//H3DlgScrollbar_proc (Era_Dlg_Scroll_Proc, H3BaseDlg*)
void __fastcall Era_Dlg_Scroll_Proc(INT32 a1, H3BaseDlg* dlg) { dlg->GetScrollbar(SCROLL_BAR_ID)->Refresh(); }//return;

																											  
// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

//PatcherInstance* _PI = nullptr;
void Dlg_EraOption_DrawMods(EraMenuDlg& dlg, em::EMod& mod, int i)

{

	H3DlgText* txt = H3DlgText::Create(50, 50 + i * 50, 200, 25, mod.GetName().String(), NH3Dlg::Text::BIG, 1, mod.GetId(), eTextAlignment::HLEFT);
	//H3DlgCaptionButton *bttn =H3DlgCaptionButton
	//"OVBUTN2.def"// up/duwn btn
	dlg.AddItem(txt);
}


int Dlg_EraOptions_Create(EraMenuDlg& dlg)
{

	for (INT8 i = 0; i < 4; i++)
	{
		H3DlgDefButton* bttn = H3DlgDefButton::Create(i * 60 + 30, 40, 110 + i, "icm003.def", 0, 1, 0, eVKey::H3VK_1 + i);
			
		dlg.AddItem(bttn);
		for (size_t j = 0; j < 20; j++)
		{
		//	H3DlgFrame* fr = H3DlgFrame::Create(32 *j +24, i * 60 + 24, 24, 24, j* i +j + 1, h3::H3RGB565::Black() + i * 12 + (j * 5 - i));
		//	dlg.AddItem(fr);
		}
		H3LoadedDef* def;
		def->GetGroupFrame(0, 1);


	}
	constexpr int step = 1;
	H3DlgScrollbar* scroll = H3DlgScrollbar::Create(INT32(dlg.GetWidth() * 0.25f), 14, 16, dlg.GetHeight() - 46, SCROLL_BAR_ID, 6, Era_Dlg_Scroll_Proc, false, step, true);
	//h3dlgc
	int size = dlg.mods.size();
	for (int i = 0; i < size; i++)
	{
		Dlg_EraOption_DrawMods(dlg, dlg.mods[i], i);

	}

//	H3Messagebox(Era::tr(dlg_mods[dlg_mods.size()-1].name.String()));

	dlg.AddItem(scroll);
	//dlg.CreateOK32Button(200, 200);
	dlg.CreateOKButton();

	dlg.GetDefButton(eControlId::OK)->AddHotkey(eVKey::H3VK_E);
	dlg.CreateCancelButton(100, 100);

/**	int cats_num = 0;
	char delimeter = ',';
	std::set<std::string>* my_set = new std::set<std::string>;

	H3String my_txt = Era::tr("gem_plugin.aa");
	if (!my_txt.Empty())
	{
		char* ptr;
		ptr = strtok((char*)my_txt.String(), &delimeter); // use strtok() function to separate string using comma (,) delimiter.  
		while (ptr != NULL)
		{
			H3String temp = ptr;

			// remove any spaces (the real trim)
			std::string st(temp.String());
			trim(st);
			temp = st.c_str();

			if (!temp.Empty())
			{
				my_set->emplace(temp.String());
				H3DlgText* txt = H3DlgText::Create(200 + cats_num * 10, 500 + cats_num * 10, 200, 25, temp.String(), NH3Dlg::Text::BIG, 1, 123 + cats_num, eTextAlignment::HLEFT);
				//dlg.AddItem(txt);
				cats_num++;
			}
			ptr = strtok(NULL, &delimeter);
		}

	}

	delete my_set;
**/

	return EXIT_SUCCESS;
}



void __stdcall NewScenarioDlg_Create(HiHook* hook, H3SelectScenarioDialog* dlg, H3Msg* msg)
{
	THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);

	H3DlgCaptionButton* bttn = dlg->GetCaptionButton(4444);
	if (bttn)
	{
		bttn->AddHotkey(h3::eVKey::H3VK_W);

		dlg->CreateCaptionButton(bttn->GetX(), bttn->GetY() + 45, bttn->GetWidth(), bttn->GetHeight(), 4500, bttn->GetDef()->GetName(), "ERA options", h3::NH3Dlg::Text::SMALL, 0);
		bttn = dlg->GetCaptionButton(4500);

		if (bttn)
			bttn->AddHotkey(h3::eVKey::H3VK_E);

	}
}
int em::MenuItem::item_id = 100;
//int H3SelectScenarioDialog::DefaultProc(h3::H3Msg& msg)



const bool CreateSth()

{	int ratio = 1;

	bool need_resize = false;
	

	do
	{
		int width = o_HD_X, height = o_HD_Y, x = -1, y = -1;

		if (ratio >1 && ratio <4)
		{
			width = o_HD_X - (o_HD_X - 800) / (ratio ^1);
			height = o_HD_Y - (o_HD_Y - 600) / (ratio ^1);
		}
		if (ratio == 4)
		{
			width = 800;
			height = 600;
		}

		Era::y[1] = width;
		//Era::y[2] = height;

		//*(int*)0x728540 = 2;
		h3::CharAt(0x728540) = 2;
		EraMenuDlg dlg(width, height, x, y, 1, 1);
		//_PI->WriteByte(0x728540, 2);

		
		for (H3String& name : dlg_mods)
			dlg.mods.emplace_back(name, nullptr);

		if (Dlg_EraOptions_Create(dlg) == EXIT_SUCCESS)
		{

			dlg.Start();			
			ratio = dlg.ratio;
			need_resize = dlg.NeedRestart();
		}
		dlg.Stop();
			
		h3::CharAt(0x728540) = 18;
		//_PI->WriteByte(0x728540, 18);
	} while (need_resize);

	//_era_dlg.AddBackground()



	return false;
}

void __stdcall Dlg_WoG_Options_Show(HiHook* hook, int a1)
{


	return;
}

