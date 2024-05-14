#include "pch.h"
#include "LanguageSelectionDlg.h"

constexpr UINT32 WIDGET_WIDTH = 213;
constexpr UINT32 WIDGET_HEIGHT = 23;
constexpr UINT16 FRAME_WIDGET_ID= 2;
constexpr UINT16 FIRST_SELECTION_WIDGET_ID = 3;
constexpr UINT16 MAX_ITEMS_PER_HEIGHT = 9;

H3LoadedPcx* LanguageSelectionDlg::m_widgetBackground = nullptr;

LanguageSelectionDlg::LanguageSelectionDlg(int width, int height, int x , int y, BOOL statusBar, BOOL makeBackground )
	: H3Dlg(WIDGET_WIDTH , MAX_ITEMS_PER_HEIGHT * WIDGET_HEIGHT, x, y, statusBar, makeBackground), m_localeHandler(nullptr)
{
//	
	//this->widthDlg = 2;
	flags ^=0x10; // disable dlg shadow
	m_localeHandler = new LocaleHandler();
	CreateDlgItems();
}
LanguageSelectionDlg::LanguageSelectionDlg(const H3DlgItem* calledItem)
	:LanguageSelectionDlg(WIDGET_WIDTH + 16, WIDGET_HEIGHT, calledItem->GetAbsoluteX(), calledItem->GetAbsoluteY())
{

}
void __fastcall DlgSroll_Proc(INT32 tickId, H3BaseDlg* dlg)
{
	
	// try to cast dlg to our dlg
	if (auto localeDlg = dynamic_cast<LanguageSelectionDlg*>(dlg))
	{
		//localeDlg->HideFrame();
		// iterate all items by id
		for (size_t i = 0; i < MAX_ITEMS_PER_HEIGHT; i++)
		{
			// assure those support lcoale edits
			auto selectionWidget = dlg->Get<H3DlgTextPcxLocale>(i  + FIRST_SELECTION_WIDGET_ID);
			if (selectionWidget)
			{	// set new locale according on the scroll offset
				auto locale = localeDlg->Handler()->LocaleAt(i + tickId);
				selectionWidget->SetLocale(locale);
			}

		}

		bool found = false;
		for (size_t i = 0; i < MAX_ITEMS_PER_HEIGHT; i++)
		{
			// assure those support lcoale edits
			auto selectionWidget = dlg->Get<H3DlgTextPcxLocale>(i + FIRST_SELECTION_WIDGET_ID);
			if (selectionWidget)
			{	// set new locale according on the scroll offset
				auto selected = localeDlg->Handler()->SelectedLocale();

				auto locale = selectionWidget->GetLocale();
				if (selected && selected == locale)
				{
					found = true;
					localeDlg->PlaceFrameAtWidget(selectionWidget);
					localeDlg->Redraw();
					break;
				}
			}
		}
		if (!found)
			localeDlg->HideFrame();

		// redraw dlg to show changes properly
		localeDlg->Redraw();
	}
}
void LanguageSelectionDlg::CreateDlgItems()
{

	// here create items

	// get number of the locales
	const UINT SIZE = m_localeHandler->GetCount();
	
	// if items more than need
	const int overload = SIZE - MAX_ITEMS_PER_HEIGHT;
	if (overload > 0)
	{
		constexpr int SCROLLBAR_WIDTH = 16;
		CreateScrollbar(widthDlg, 0, SCROLLBAR_WIDTH, heightDlg, 1, overload, DlgSroll_Proc);
		widthDlg += SCROLLBAR_WIDTH;
	}

	const int ITEMS_NUM = SIZE < MAX_ITEMS_PER_HEIGHT ? SIZE : MAX_ITEMS_PER_HEIGHT;

	// change dlg height accordingly
	heightDlg = ITEMS_NUM * WIDGET_HEIGHT;

	for (size_t i = 0; i < ITEMS_NUM; i++)
	{
		const int y = i * WIDGET_HEIGHT;
		auto textPcx = H3DlgTextPcxLocale::Create(0, y, WIDGET_WIDTH, WIDGET_HEIGHT, m_localeHandler->LocaleAt(i), h3::NH3Dlg::Text::MEDIUM, m_widgetBackground, 1, i + FIRST_SELECTION_WIDGET_ID);
		AddItem(textPcx);
	}

	H3RGB565 color(H3RGB888::Highlight());
	m_selectionFrame = this->CreateFrame(1, 1, WIDGET_WIDTH - 2, WIDGET_HEIGHT - 2, FRAME_WIDGET_ID, color);
	m_selectionFrame->HideDeactivate();

	//auto frame = CreateFrame(0, 0, widthDlg, heightDlg, -1, color);
	//frame->DeActivate();
}
BOOL LanguageSelectionDlg::OnMouseHover(H3DlgItem* it)
{
	if (it->GetID()>= FIRST_SELECTION_WIDGET_ID)
	{
		if (auto pcxLocale = it->Cast<H3DlgTextPcxLocale>() )
		{
			m_localeHandler->SetSelected(pcxLocale->GetLocale());

			PlaceFrameAtWidget(pcxLocale);
			this->Redraw();
		}
	}

	return 0;
}

const void LanguageSelectionDlg::HideFrame() const noexcept
{
	m_selectionFrame->HideDeactivate();
	m_localeHandler->SetSelected(nullptr);
}

void LanguageSelectionDlg::PlaceFrameAtWidget(const H3DlgTextPcxLocale* it) const noexcept
{
	m_selectionFrame->SetX(it->GetX() + 1);
	m_selectionFrame->SetY(it->GetY() + 1);
	m_selectionFrame->ShowActivate();
	m_localeHandler->SetSelected(it->GetLocale());
}

BOOL LanguageSelectionDlg::DialogProc(H3Msg& msg)
{
	if (msg.IsLeftDown() && msg.itemId == FRAME_WIDGET_ID)
	{
		P_SoundManager->ClickSound();

		const Locale* locale = m_localeHandler->SelectedLocale();
		if (*locale != *m_localeHandler->CurrentLocale())
		{
			const char* format = EraJS::read("era.locale.dlg.question");
			sprintf(h3_TextBuffer, format, locale->displayedName.c_str(), locale->name.c_str());
			//ask question
			if (H3Messagebox::Choice())
			{
				m_localeHandler->ChangeLocale(locale);
				Stop();
			}
		}


	}

	if (msg.ClickOutside())
	{
		Stop();
	}

	return 0;
}




void LanguageSelectionDlg::CreateAssets()
{

	if (!m_widgetBackground)
	{
		// create pcx for back
		m_widgetBackground = H3LoadedPcx::Create(h3_NullString, WIDGET_WIDTH, WIDGET_HEIGHT);
		auto basePcx = H3LoadedPcx::Load("comopbck.pcx");
		basePcx->DrawToPcx(245, 253, 245 + WIDGET_WIDTH, 253 + WIDGET_HEIGHT, m_widgetBackground, 0, 0, 1);
		basePcx->Dereference();
	}
}




const LocaleHandler* LanguageSelectionDlg::Handler() const noexcept
{
	return m_localeHandler;
}

LanguageSelectionDlg::~LanguageSelectionDlg()
{
	if (m_localeHandler)
	{
		delete m_localeHandler;
		m_localeHandler = nullptr;
	}
}

constexpr UINT16 LOCALEDLG_BUTTON_ID = 4445;
void CreateDlgButton(H3BaseDlg* dlg, int x, int y)
{

	auto def = H3LoadedDef::Load("OVBUTN3.def");
	auto captionButton = H3DlgCaptionButton::Create(x, y, def->widthDEF, def->heightDEF, LOCALEDLG_BUTTON_ID, "OVBUTN3.def", "text", h3::NH3Dlg::Text::SMALL, 0, 0, 0, 0, 0);
	captionButton->SetClickFrame(1);

	dlg->AddItem(captionButton);
	def->Dereference();
}
_LHF_(DlgMainMenu_BeforeRun)
{
	auto dlg = reinterpret_cast<H3Dlg*>(c->ecx);    // before any dlg run get dlg

	//H3Messagebox(Era::IntToStr((int)H3CreatureInformation::Get()).c_str());
	//H3Messagebox(Era::IntToStr((int)P_CreatureInformation).c_str());
	if (dlg) // if exists
		CreateDlgButton(dlg, 0, 0);

	return EXEC_DEFAULT;
}
_LHF_(DlgSystemOptions_AtCreate)
{
	H3Dlg* dlg = *reinterpret_cast<H3Dlg**>(c->ebp - 0x20);    // before any dlg run get dlg

	if (dlg) // if exists
		CreateDlgButton(dlg, 235, 270);
	return EXEC_DEFAULT;
}

void HandleDlgButton(const H3Msg* msg)
{
	if (msg->IsLeftClick() && msg->itemId == LOCALEDLG_BUTTON_ID)
	{
		auto bttn = msg->GetDlg()->GetCaptionButton(msg->itemId);
		if (bttn)
		{
			LanguageSelectionDlg langDlg(bttn);// = direction ? {500, 500} : {0};
			langDlg.Start();
		}
	}
}

int __stdcall DlgSystemOption_Proc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg)
{
	HandleDlgButton(msg);
	return THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);;
}

int __stdcall DlgMainMenu_Proc(HiHook* h, H3Msg* msg)
{
	HandleDlgButton(msg);
	return FASTCALL_1(int, h->GetDefaultFunc(), msg);;
}





void LanguageSelectionDlg::Init()
{
	LanguageSelectionDlg::CreateAssets();
	//Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
	_PI->WriteLoHook(0x5B2F9B, DlgSystemOptions_AtCreate); //System options dlg from Adventure Mgr and Main Menu (by HD mod only - RMB or MRM)

	_PI->WriteLoHook(0x4EF259, DlgMainMenu_BeforeRun); //MAIN Main Menu Dlg Run
  //  _PI->WriteLoHook(0x4EF331, DlgMainMenu_BeforeRun); // New Game Main Menu Dlg Run
  //  _PI->WriteLoHook(0x4EF668, DlgMainMenu_BeforeRun); // Load Game Main Menu Dlg Run
   // _PI->WriteLoHook(0x4F0799, DlgMainMenu_BeforeRun);// Campaign Game Main Menu Dlg Run
	_PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc);
	_PI->WriteHiHook(0x5B3450, THISCALL_, DlgSystemOption_Proc);
}


