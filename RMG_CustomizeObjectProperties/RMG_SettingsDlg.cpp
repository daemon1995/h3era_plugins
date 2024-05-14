#include "pch.h"
#include "RMG_SettingsDlg.h"
constexpr INT16 SELECTION_DLG_RMG_SETTINGS_BUTTON_ID = 1221;
constexpr INT16 SELECTION_DLG_RMG_SETTINGS_TEXT_ID = 1222;

RMG_SettingsDlg* RMG_SettingsDlg::DlgPage::dlg = nullptr;
RMG_SettingsDlg::RMG_SettingsDlg(int width, int height, int x = -1, int y = -1)
	:H3Dlg(width, height, x, y, false, false), m_settings(nullptr), m_currentPage(nullptr)
{

	RMG_SettingsDlg::MapObject_DlgPanel::id = 0;
	static bool m_objListRead = false;
	//THISCALL_1(void, 0x5063F0, &P_Game->mainSetup);

	AddBackground(0, 0, width, height,false,false, IntAt(0x69CCF4),true);
	if (background)
	{
		background->FrameRegion(0, 0, width, height, true, IntAt(0x69CCF4), false);
	
	// blue 
	}
	if (!m_objListRead)
	{
		m_objListRead = true;
	}

	RMG_SettingsDlg::DlgPage::dlg = this;

	auto okBttn = this->CreateOKButton();
	//okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
	this->CreateCancelButton();

	// get objects list from main

	m_objectsList = P_Game->mainSetup.objectLists;

	m_settings = ReadIniSettings();

	GetCreatureBanks();
	m_pages.emplace_back( new CreatureBanks_DlgPage{ "banks",m_banks });
}


void RMG_SettingsDlg::GetCreatureBanks()
{
	constexpr int SIZE = 5;
	constexpr int objects[SIZE] = {
		eObject::CREATURE_BANK ,
		eObject::DERELICT_SHIP ,
		eObject::DRAGON_UTOPIA ,
		eObject::CRYPT,
		eObject::SHIPWRECK
	};




	// get all creature banks by type/subptye
	for (size_t i = 0; i < SIZE; i++)
	{
		for (const auto& obj : P_Game->mainSetup.objectLists[objects[i]])
			if (std::find_if(m_banks.begin(), m_banks.end(), [&](const H3ObjectAttributes& atr)
				{
					return atr.type == obj.type && atr.subtype == obj.subtype;
				}
			) == m_banks.end())
			{
				m_banks.Push(obj);
			}
	}


	std::reverse(m_banks.begin(), m_banks.end());

	
	//std::set<

	for (size_t i = 0; i < m_banks.Size(); i++)
	{
	//	H3Messagebox::RMB(H3String::Format("type %d, subtype %d",m_banks[i].type, m_banks[i].subtype));
	}

	//return;
	std::sort(m_banks.begin(), m_banks.end(), [](const H3ObjectAttributes& first, const H3ObjectAttributes& second) ->bool
		{
		//	if (first.type < second.type)
				return first.subtype > second.subtype;
		//	else if (first.subtype <= second.subtype)
				return false;

		}
	);

//	std::vector<H3CreatureBankSetup>* bankSetups = *reinterpret_cast<std::vector<H3CreatureBankSetup>**>(0x67029C);
//	const int creatureBanksNum = ByteAt(0x47A3BA + 0x1);
	//H3Messagebox::RMB(Era::IntToStr(CreatureBanksExtender::instance->banks.cbId.size()).c_str());

	auto& banks = CreatureBanksExtender::instance;
	banks->defaultBanksNumber;
	for (size_t i = 0; i < banks->defaultBanksNumber; i++)
	{

	}
	//CreatureBanksExtender::instance->banks.size;
	//
}


BOOL RMG_SettingsDlg::DialogProc(H3Msg& msg)
{


	auto* mapObjectsPage = dynamic_cast<MapObjects_DlgPage*>(m_currentPage);
	if (mapObjectsPage)
	{


	}

	if (m_currentPage)
	{


		int needRedraw = m_currentPage->Proc(msg);
		if (needRedraw)
			Redraw();
	}
	return 0;
}

VOID RMG_SettingsDlg::OnOK()
{
	if (!WriteIniSettings())
		H3Messagebox(PluginText::text().iniError);
}
VOID RMG_SettingsDlg::OnCancel()
{
	//this->Stop()
}
RMG_Settings* RMG_SettingsDlg::ReadIniSettings()
{



	return nullptr;
}

bool RMG_SettingsDlg::WriteIniSettings()
{
	//Era::ReadStrFromIni("a", "a", m_iniPath, h3_TextBuffer);

//	bool result = false;
	for (const auto& page : m_pages)
	{
		// iterate pages
		if (const auto* mapObjectPage = dynamic_cast<MapObjects_DlgPage*>(page))
		{// try to cast to map object page

			for (auto& parameter : mapObjectPage->objectParameters)
			{
				if (!RMG_ObjectsSettings::WriteParametersToIni(m_iniPath, *parameter))
					return false;
			}
		}
	}
		//page.se	
		
	return Era::SaveIni(m_iniPath);
}




RMG_SettingsDlg::CreatureBanks_DlgPage::CreatureBanks_DlgPage(const char* name, const H3Vector<H3ObjectAttributes>& data)
	:MapObjects_DlgPage(name,data)
{
	//auto* dlg = dlg;
	int x = 0;
	int y = 0;
	int itemId = 100;

	const int itemsToDisplay = data.Size();

	constexpr int FIELD_WIDTH = 300;

	constexpr int FIELD_HEIGHT = 50;
	const int layoutWidth = dlg->widthDlg - 100;
	const int layoutHeight = dlg->heightDlg - 40;
	const int itemsPerPage = layoutHeight / FIELD_HEIGHT;

	const int PANELS_NUM = dlgPanels.size();
	const int DATA_NUM = objectParameters.size();

	// if needed size > space -> create custom scroll bar
	if (PANELS_NUM < DATA_NUM)
	{
		scrollBar = H3DlgScrollbar::Create(dlg->widthDlg - 24,24,16, layoutHeight, itemId, (DATA_NUM - PANELS_NUM) >>1, CreatureBankPage_ScrollBarProc,false,1,true);
		
		dlg->AddItem(scrollBar);
		pageItemsIds.emplace_back(itemId++);
	}

	//dlg->heightDlg;

	H3RGB565 color(H3RGB888::Highlight());

	constexpr int PCX_WIDTH = 44;
	constexpr int PCX_HEIGHT = 44;
	//return;
	constexpr int columns = 2;

	//pageMapObjects;

		return;


}
UINT RMG_SettingsDlg::MapObject_DlgPanel::id = 0;
RMG_SettingsDlg::MapObject_DlgPanel::MapObject_DlgPanel(const int x, const int y,  DlgPage* parent)
	:x(x),y(y), visible(false), parentPage(parent)
{
	auto* dlg = parentPage->dlg;
	id++;
	int itemId = 100 * (id +1);

	constexpr int FIELD_WIDTH = 350;

	constexpr int FIELD_HEIGHT = 50;
	const int layoutWidth = dlg->widthDlg - 100;
	const int layoutHeight = dlg->heightDlg - 40;
	const int itemsPerPage = layoutHeight / FIELD_HEIGHT;

	//dlg->heightDlg;

	H3RGB565 color(H3RGB888::Highlight());

	constexpr int PCX_WIDTH = 44;
	constexpr int PCX_HEIGHT = 44;
	//return;
	constexpr int columns = 2;



	// add item into page list

	this->backDlgPcx = H3DlgPcx16::Create(x, y, FIELD_WIDTH, FIELD_HEIGHT, itemId++, 0);
	H3LoadedPcx16* pcx = H3LoadedPcx16::Create(FIELD_WIDTH, FIELD_HEIGHT);
	pcx->CopyRegion(parent->dlg->background, x, y);

	pcx->DarkenArea(0, 0, FIELD_WIDTH, FIELD_HEIGHT, 100);


	backDlgPcx->SetPcx(pcx);
	items.Add(backDlgPcx);
	items.Add(H3DlgFrame::Create(backDlgPcx, color, itemId++));


	pictureItem = H3DlgPcx16::Create(x + 3, y +3, PCX_WIDTH, PCX_HEIGHT, itemId++, 0); // create dlg pcx w/o picture
	items.Add(pictureItem);

	static int iss =0;

	objectNameItem = H3DlgText::Create(PCX_WIDTH +x +10, y, FIELD_WIDTH - PCX_WIDTH -160, FIELD_HEIGHT, h3_NullString , h3::NH3Dlg::Text::MEDIUM, 1, itemId++);
	items.Add(objectNameItem);

	const int X = x + FIELD_WIDTH - 130;

	items.Add(H3DlgFrame::Create(X - 20, y, 4, FIELD_HEIGHT, itemId++, color));

	PluginText& text = PluginText::text();
	mapLimitTitle = H3DlgText::Create(X, y + 5, 80, 20, text.mapLimit, h3::NH3Dlg::Text::MEDIUM, 1, itemId++);
	items.Add(mapLimitTitle);

	zoneLimitTitle = H3DlgText::Create(X, y +25, 80, 20, text.zoneLimit, h3::NH3Dlg::Text::MEDIUM, 1, itemId++);
	items.Add(zoneLimitTitle);


	constexpr int MAX_TEXT_LENGTH = 4;

	mapLimitEdit = H3DlgEdit::Create(X +100, y +5, 24, 20, MAX_TEXT_LENGTH, "0", h3::NH3Dlg::Text::MEDIUM, 1, eTextAlignment::MIDDLE_CENTER, "DIBOX128.PCX", itemId++, true, 1, 1);
	


	items.Add(mapLimitEdit);
	

	zoneLimitEdit = H3DlgEdit::Create(X + 100, y +25, 24, 20, MAX_TEXT_LENGTH, "0", h3::NH3Dlg::Text::MEDIUM, 1, eTextAlignment::MIDDLE_CENTER, "DIBOX128.PCX", itemId++, true, 1, 1);

	items.Add(zoneLimitEdit);
	//zoneLimitEdit->autoRedraw = true;// ();
	for (auto& wgt : items)
		dlg->AddItem(wgt);


}

RMG_SettingsDlg::MapObject_DlgPanel::~MapObject_DlgPanel()
{
	//pictureItem->GetPcx()->Destroy();

	pictureItem->SetPcx(nullptr);

	auto* pcx = backDlgPcx->GetPcx();
	if (pcx)
	{
		pcx->Destroy();
		backDlgPcx->SetPcx(nullptr);
	}


}

void RMG_SettingsDlg::MapObject_DlgPanel::HideDeactivate()
{

	visible = false;
	for (auto& wgt : items)
		wgt->HideDeactivate();
}

void RMG_SettingsDlg::MapObject_DlgPanel::ShowActivate()
{
	visible = true;
	for (auto& wgt : items)
		wgt->ShowActivate();
}


void RMG_SettingsDlg::MapObject_DlgPanel::SetData( MapObject* mapObject)
{
	if (mapObject != nullptr)
	{
		data = mapObject;
		objectNameItem->SetText(data->GetObjectName());

	}
	Refresh();
}
void RMG_SettingsDlg::MapObject_DlgPanel::Refresh()
{
	if (data != nullptr)
	{
		mapLimitEdit->SetText(std::to_string(data->mapLimit).c_str());
		zoneLimitEdit->SetText(std::to_string(data->zoneLimit).c_str());
		pictureItem->SetPcx(data->enabled ? data->enabledObjectsPcx : data->disabledObjectsPcx);
	}
}


RMG_SettingsDlg::DlgPage::~DlgPage()
{
//	H3Messagebox::RMB(Era::IntToStr(resizedPcx16.Size()).c_str());
//	for (auto* dlgPcx : resizedPcx16)
	{
	//	dlgPcx->GetPcx()->Destroy();
	//	dlgPcx->SetPcx(nullptr);
	}
	pageItemsIds.clear();
	//resizedPcx16.RemoveAll();
}


RMG_SettingsDlg::MapObjects_DlgPage::~MapObjects_DlgPage()
{

	dlgPanels.clear();
	objectParameters.clear();
}


void RMG_SettingsDlg::SetPatches(PatcherInstance* _pi)
{

	_pi->WriteHiHook(0x579CE0, THISCALL_, NewScenarioDlg_Create);

	_pi->WriteLoHook(0x5806D4, H3SelectScenarioDialog_ShowRandomMapsSettings);
	_pi->WriteLoHook(0x58207F, H3SelectScenarioDialog_HideRandomMapsSettings);
	_pi->WriteLoHook(0x586924, Dlg_SelectScenario_Proc);

}
RMG_SettingsDlg::~RMG_SettingsDlg()
{
	m_objectsList = nullptr;

	m_pages.clear();

	RMG_SettingsDlg::DlgPage::dlg = nullptr;

}



void RMG_SettingsDlg::MapObjects_DlgPage::SetVisible(bool state)
{
	for (auto& p : dlgPanels)
		state ? p->ShowActivate() : p->HideDeactivate();
	visible = state;

}

void RMG_SettingsDlg::MapObjects_DlgPage::SaveData()
{
	for (auto&panel: dlgPanels)
	{
		panel->data->mapLimit = panel->mapLimitEdit->GetH3String().ToSigned();
		panel->data->zoneLimit = panel->zoneLimitEdit->GetH3String().ToSigned();
	}
}





RMG_SettingsDlg::MapObjects_DlgPage::MapObjects_DlgPage(const char* name, const H3Vector<H3ObjectAttributes>& data)
	:DlgPage(name), h3ObjectAttributes(data)
{
	//resizedPcx16.RemoveAll();
	objectParameters.clear();


	const int SIZE = data.Size();

	//H3Messagebox();
	objectParameters.reserve(SIZE);
	//disabledObjectsPcx.Reserve(SIZE);

//	objectParameters.resize(SIZE);



	constexpr int PCX_WIDTH = 44;
	constexpr int PCX_HEIGHT = 44;
	LPCSTR fileName = dlg->m_iniPath;

	for (size_t i = 0; i < SIZE; i++)
	{

		auto def = H3LoadedDef::Load(data[i].defName.String());
		auto &obj = data[i];
		double ratio = static_cast<double>(def->widthDEF) / def->heightDEF;

		//continue;
		// create temp pcx to draw def there
		H3LoadedPcx16* tempPcx = H3LoadedPcx16::Create(def->widthDEF, def->heightDEF);
		// fill with background
	//	tempPcx->CopyRegion(dlg->GetBackgroundPcx(), 25, 25);
		memset(tempPcx->buffer, 0, tempPcx->buffSize);
		//	tempPcx->FillRectangle(0, 0, tempPcx->width, tempPcx->height, 0, 255, 255);

		//copy def to temp pcx
		def->DrawToPcx16(0, 0, tempPcx, 0, 0);

		// create pcx to draw resized temp pcx there
		H3LoadedPcx16* enabledPcx = H3LoadedPcx16::Create(PCX_WIDTH, PCX_HEIGHT);
		H3LoadedPcx16* disabledPcx = H3LoadedPcx16::Create(PCX_WIDTH, PCX_HEIGHT);
		int dstX = 0, dstY = 0;
		if (ratio != 1)
		{
			dstX = (static_cast<double>(PCX_WIDTH) / ratio);
			dstY = (static_cast<double>(PCX_HEIGHT) / ratio);
		}

		int dstWitdth = (PCX_WIDTH - dstX);
		int dstHeight = (PCX_HEIGHT - dstY);
		DrawPcx16ResizedBicubic(enabledPcx, tempPcx, def->widthDEF, def->heightDEF, dstX, dstY, dstWitdth, dstHeight);

		// create copy of the picture and grayscale
		memcpy(disabledPcx->buffer, enabledPcx->buffer, enabledPcx->buffSize);
		disabledPcx->GrayScaleArea(0, 0, PCX_WIDTH, PCX_HEIGHT);


		MapObject *param = new MapObject(obj);
		param->enabledObjectsPcx = enabledPcx; //.emplace_back()
		param->disabledObjectsPcx = disabledPcx;

		objectParameters.emplace_back(param);// [i] .attributes = data[i];
		// read from INI
		RMG_ObjectsSettings::ReadParametersFromIni(fileName, *objectParameters[i]);

		// destroy temp picture
		tempPcx->Destroy();

		// assign resized pic
		def->Dereference();

	}
	CreateDlgPanels();
	//H3Messagebox(Era::IntToStr(objectParameters[12]).c_str());
	//H3Messagebox(objectParameters[1].GetObjectName());



//	pageItemsIds.reserve(data.Size());

}






void RMG_SettingsDlg::MapObjects_DlgPage::CreateDlgPanels()
{
	constexpr int FIELD_WIDTH = 300;

	constexpr int FIELD_HEIGHT = 50;
	const int layoutWidth = dlg->widthDlg - 100;
	const int layoutHeight = dlg->heightDlg - 40;
	const int itemsPerPage = layoutHeight / FIELD_HEIGHT;
	//dlgPanels.reserve(16);
	//H3Messagebox();

	// prepare creating page dlg Items
	for (size_t i = 0; i < 8; i++)
	{
		for (size_t k = 0; k < 2; k++)
		{
			dlgPanels.emplace_back(new MapObject_DlgPanel(360 * k + 24, i * 55 + 94, this));
		}
	}


	if (itemsPerPage > SIZE)
	{



	}

}



void RMG_SettingsDlg::MapObjects_DlgPage::FillObjects(int firstItem)
{
	//return;
	const int PANELS_NUM = dlgPanels.size();
	const int DATA_NUM = objectParameters.size();


	for (size_t i = 0; i < PANELS_NUM; i++)
	{

		auto& objPanel = dlgPanels[i];
		int dataIndex = i + firstItem;



		if (dataIndex < DATA_NUM)
		{
			if (!objPanel->visible) objPanel->ShowActivate();
			objPanel->SetData(objectParameters[dataIndex]);

		}
		else
			objPanel->HideDeactivate();



	}
	for (size_t i = 0; i < SIZE; i++)
	{

	}
}

BOOL RMG_SettingsDlg::MapObjects_DlgPage::Proc(H3Msg& msg)
{
	int itemId = msg.itemId;
	int result = false;
//	Era::ExecErmCmd("IF:L^^");

	if (itemId
		&& msg.command == eMsgCommand::ITEM_COMMAND
		&& msg.subtype == eMsgSubtype::LBUTTON_DOWN )
	{
		auto* clickedItem = dlg->GetH3DlgItem(itemId);

		for (auto& panel : dlgPanels)
		{
			if (panel->visible)
			{
				if (panel->pictureItem == clickedItem)
				{
					panel->data->enabled ^=  true;
					result = true;
					//break;
				}

			//	if (panel->mapLimitEdit == clickedItem)
				auto* editItem = panel->mapLimitEdit;
				bool currentItem = editItem == clickedItem;
				editItem->SetFocus(currentItem);
				editItem->SetAutoredraw(currentItem);

				//editItem->Refresh();

				editItem = panel->zoneLimitEdit;
				currentItem = editItem == clickedItem;
				editItem->SetFocus(currentItem);
				editItem->SetAutoredraw(currentItem);
			//	editItem->Refresh();

			//	result += currentItem;


				
				panel->zoneLimitEdit->SetFocus(panel->zoneLimitEdit == clickedItem);

			//	else

				for (auto& wgt : panel->items)
				{
					if (wgt == clickedItem)
					{
						//Era::ExecErmCmd("IF:L^^");
					//	panel->HideDeactivate();
					//	wgt->Refresh();
					//	dlg->Redraw();
					//	break;
					}
				}


				if (result)
				{
					panel->Refresh();
				//	break;
				}
			}
		}
	}

	return result;
}



//H3Dlgsc
RMG_SettingsDlg::DlgPage::DlgPage(const char* name)
	:name(name), scrollBar(nullptr)
{
	//resizedPcx16.RemoveAll();
	pageItemsIds.clear();

}

void __fastcall RMG_SettingsDlg::CreatureBanks_DlgPage::CreatureBankPage_ScrollBarProc(INT32 tick, H3BaseDlg* dlg)
{

	if (auto* objDlg = dynamic_cast<RMG_SettingsDlg*>(dlg))
	{

		objDlg->m_currentPage->FillObjects(tick <<1);
		objDlg->Redraw();
	}

}


_LHF_(RMG_SettingsDlg::Dlg_SelectScenario_Proc)
{
//	THISCALL_1(int, h->GetDefaultFunc(), msg);
	H3Msg* msg = reinterpret_cast<H3Msg*>(c->esi);
	if (msg && msg->itemId == SELECTION_DLG_RMG_SETTINGS_BUTTON_ID && msg->subtype == eMsgSubtype::LBUTTON_CLICK)
	{
		H3BaseDlg*parent=  H3SelectScenarioDialog::Get();
		RMG_SettingsDlg dlg(parent->widthDlg -3, parent->heightDlg-4);
		dlg.Start();

	}

	return EXEC_DEFAULT;
}

_LHF_(RMG_SettingsDlg::H3SelectScenarioDialog_ShowRandomMapsSettings)
{
	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->esi);
	if (dlg)
	{
		if (H3DlgItem* it = dlg->GetH3DlgItem(SELECTION_DLG_RMG_SETTINGS_TEXT_ID))		it->ShowActivate();
		if (H3DlgItem* it = dlg->GetH3DlgItem(SELECTION_DLG_RMG_SETTINGS_BUTTON_ID))	it->ShowActivate();
	}

	return EXEC_DEFAULT;
}
_LHF_(RMG_SettingsDlg::H3SelectScenarioDialog_HideRandomMapsSettings)
{
	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->esi);
	if (dlg)
	{
		if (H3DlgItem* it = dlg->GetH3DlgItem(SELECTION_DLG_RMG_SETTINGS_TEXT_ID))			it->HideDeactivate();
		if (H3DlgItem* it = dlg->GetH3DlgItem(SELECTION_DLG_RMG_SETTINGS_BUTTON_ID))			it->HideDeactivate();
	}
	return EXEC_DEFAULT;
}
BOOL RMG_SettingsDlg::OnCreate()
{
	for (size_t i = 0; i < 1; i++)
	{
		this->m_pages[i]->FillObjects();
	}
	SetActivePage(m_pages[0]);

	return true;
}
void RMG_SettingsDlg::SetActivePage(DlgPage* page) noexcept
{
	if (page != m_currentPage)
	{

		if (m_currentPage)
			m_currentPage->SetVisible(false);
		if (page)
			page->SetVisible(true);
		m_currentPage = page;
	}

}
void __stdcall RMG_SettingsDlg::NewScenarioDlg_Create(HiHook* hook, H3SelectScenarioDialog* dlg, H3Msg* msg)
{
	THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);

	H3DlgCaptionButton* bttn = dlg->CreateCaptionButton(246, 291, 128, 20, SELECTION_DLG_RMG_SETTINGS_BUTTON_ID, (char*)IntAt(0x57A93B + 1), "RMG", h3::NH3Dlg::Text::SMALL, 0);
	if (bttn)
	{
		bttn->SetClickFrame(1);
		bttn->AddHotkey(h3::eVKey::H3VK_S);
		bttn->HideDeactivate();
	}
	H3DlgText* text = dlg->CreateText(248, 264, 128, 20, "RMG", h3::NH3Dlg::Text::SMALL, eTextColor::YELLOW, SELECTION_DLG_RMG_SETTINGS_TEXT_ID, eTextAlignment::MIDDLE_RIGHT);
	if (text)
	{
		text->HideDeactivate();
	}
}

MapObject::MapObject(const H3ObjectAttributes& attributes)
	:enabledObjectsPcx(nullptr), disabledObjectsPcx(nullptr), attributes(attributes),enabled(true),mapLimit(32000),zoneLimit(32000)
{

}

MapObject::~MapObject()
{
	enabledObjectsPcx->Destroy();
	disabledObjectsPcx->Destroy();
}

LPCSTR MapObject::GetObjectName() const
{
	LPCSTR result;

	int objectType = attributes.type;
	int objectSubtype = attributes.subtype;

	int cbId = CreatureBanksExtender::GetCreatureBankId(objectType, objectSubtype);
	if (cbId >= 0)
		return CreatureBanksExtender::instance->banks.setups[cbId].name.String();
	switch (objectType)
	{
	case eObject::CREATURE_GENERATOR1:
		result = P_DwellingNames1[objectSubtype];
		break;
	case eObject::CREATURE_GENERATOR4:
		result = P_DwellingNames4[objectSubtype];
		break;
	default:
		result = P_ObjectName[objectType];
		break;
	}

	return result;
}
