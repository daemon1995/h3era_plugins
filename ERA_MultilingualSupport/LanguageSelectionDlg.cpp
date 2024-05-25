#include "pch.h"
#include "LanguageSelectionDlg.h"

//constexpr UINT32 WIDGET_WIDTH = 213;
//constexpr UINT32 WIDGET_HEIGHT = 23;
constexpr UINT16 FRAME_WIDGET_ID = 2;
constexpr UINT16 FIRST_SELECTION_WIDGET_ID = 3;


LanguageSelectionDlg::LanguageSelectionDlg(int width, int height, int x, int y, BOOL statusBar, BOOL makeBackground)
	: H3Dlg(width, height, x, y, false, false), m_localeHandler(nullptr)
{
	//	m_style
		//this->widthDlg = 2;
	flags ^= 0x10; // disable dlg shadow
	m_localeHandler = &LocaleHandler::Get();
}
LanguageSelectionDlg::LanguageSelectionDlg(const H3DlgItem* calledItem, const DlgStyle* style, const UINT itemsToDraw)
	:LanguageSelectionDlg(style->WIDGET_WIDTH + 16 * (itemsToDraw > style->MAXIMUM_ROWS), style->WIDGET_HEIGHT* itemsToDraw, calledItem->GetAbsoluteX(), calledItem->GetAbsoluteY())
{
	m_style = style;
	CreateDlgItems();

}

/// function to get last locale from vector
// write commentaries
	




void __fastcall DlgSroll_Proc(INT32 tickId, H3BaseDlg* dlg)
{



	// try to cast dlg to our dlg
	if (auto localeDlg = dynamic_cast<LanguageSelectionDlg*>(dlg))
	{
		const UINT16 MAX_ITEMS_PER_HEIGHT = localeDlg->Style()->MAXIMUM_ROWS;

		//localeDlg->HideFrame();
		// iterate all items by id
		for (size_t i = 0; i < MAX_ITEMS_PER_HEIGHT; i++)
		{
			// assure those support lcoale edits
			auto selectionWidget = dlg->Get<H3DlgTextPcxLocale>(i + FIRST_SELECTION_WIDGET_ID);
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
	const UINT WIDGET_WIDTH = m_style->WIDGET_WIDTH;
	const UINT WIDGET_HEIGHT = m_style->WIDGET_HEIGHT;

	const UINT SIZE = m_localeHandler->GetCount();

	const UINT16 MAX_ITEMS_PER_HEIGHT = m_style->MAXIMUM_ROWS;
	// if items more than need
	const int overload = SIZE - MAX_ITEMS_PER_HEIGHT;
	if (overload > 0)
	{
		// create Scroll Bar
		constexpr int SCROLLBAR_WIDTH = 16;
		CreateScrollbar(WIDGET_WIDTH, 0, SCROLLBAR_WIDTH, heightDlg, 1, overload, DlgSroll_Proc);
	}

	const int ITEMS_NUM = SIZE < MAX_ITEMS_PER_HEIGHT ? SIZE : MAX_ITEMS_PER_HEIGHT;



	for (size_t i = 0; i < ITEMS_NUM; i++)
	{
		const int y = i * WIDGET_HEIGHT;
		auto textPcx = H3DlgTextPcxLocale::Create(0, y, WIDGET_WIDTH, WIDGET_HEIGHT, m_localeHandler->LocaleAt(i), h3::NH3Dlg::Text::MEDIUM, m_style->localeBackgroundLoadedPcx, 1, i + FIRST_SELECTION_WIDGET_ID);
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
	if (it->GetID() >= FIRST_SELECTION_WIDGET_ID)
	{
		if (auto pcxLocale = it->Cast<H3DlgTextPcxLocale>())
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
				m_localeHandler->SetForUser(locale);
				Stop();
			}
		}


	}

	if (msg.ClickOutside())
		Stop();

	return 0;
}





const LocaleHandler* LanguageSelectionDlg::Handler() const noexcept
{
	return m_localeHandler;
}

const DlgStyle* LanguageSelectionDlg::Style() const noexcept
{
	return m_style;
}

LanguageSelectionDlg::~LanguageSelectionDlg()
{

}

constexpr UINT16 LOCALEDLG_BUTTON_ID = 4445;
void CreateDlgButton(H3BaseDlg* dlg, int x, int y, const DlgStyle* style)
{

	auto def = H3LoadedDef::Load(style->dlgCallButtonName);
	auto captionButton = H3DlgCaptionButton::Create(x, y, def->widthDEF, def->heightDEF, LOCALEDLG_BUTTON_ID, style->dlgCallButtonName, "text", h3::NH3Dlg::Text::SMALL, 0, 0, 0, 0, 0);
	captionButton->SetClickFrame(1);

	dlg->AddItem(captionButton);
	def->Dereference();
}


_LHF_(DlgMainMenu_BeforeRun)
{
	auto dlg = reinterpret_cast<H3Dlg*>(c->ecx);    // before any dlg run get dlg


	if (dlg) // if exists
		CreateDlgButton(dlg, 0, 0, &DlgStyle::styles[0]);

	return EXEC_DEFAULT;
}
_LHF_(DlgSystemOptions_AtCreate)
{
	H3Dlg* dlg = *reinterpret_cast<H3Dlg**>(c->ebp - 0x20);    // before any dlg run get dlg

	if (dlg) // if exists
		CreateDlgButton(dlg, 235, 269, &DlgStyle::styles[1]);
	return EXEC_DEFAULT;
}

void HandleDlgButton(const H3Msg* msg, const DlgStyle* style)
{
	if (msg->IsLeftClick() && msg->itemId == LOCALEDLG_BUTTON_ID)
	{
		auto bttn = msg->GetDlg()->GetCaptionButton(msg->itemId);
		if (bttn)
		{

			const UINT SIZE = LocaleHandler::Get().GetCount();

			const UINT16 MAX_ITEMS_PER_HEIGHT  = style->MAXIMUM_ROWS;
			const int ITEMS_NUM = SIZE < MAX_ITEMS_PER_HEIGHT ? SIZE : MAX_ITEMS_PER_HEIGHT;

			//auto& style = DlgStyle::styles[0];
			LanguageSelectionDlg langDlg(bttn, style, ITEMS_NUM);// = direction ? {500, 500} : {0};
			langDlg.Start();
		}
	}
}

int __stdcall DlgSystemOption_Proc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg)
{

	HandleDlgButton(msg, &DlgStyle::styles[0]);
	return THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);;
}

int __stdcall DlgMainMenu_Proc(HiHook* h, H3Msg* msg)
{
	HandleDlgButton(msg, &DlgStyle::styles[1]);
	return FASTCALL_1(int, h->GetDefaultFunc(), msg);;
}

_LHF_(LanguageSelectionDlg::MyClassLoHook)
{
	

	return EXEC_DEFAULT;
}
void LanguageSelectionDlg::Init()
{
	if (DlgStyle::CreateAssets())
	{
		//Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
		_PI->WriteLoHook(0x5B2F9B, DlgSystemOptions_AtCreate); //System options dlg from Adventure Mgr and Main Menu (by HD mod only - RMB or MRM)
		_PI->WriteLoHook(0x5B2F9B, MyClassLoHook);
		_PI->WriteLoHook(0x4EF259, DlgMainMenu_BeforeRun); //MAIN Main Menu Dlg Run
		//_PI->WriteLoHook(0x4EF331, DlgMainMenu_BeforeRun); // New Game Main Menu Dlg Run
		//_PI->WriteLoHook(0x4EF668, DlgMainMenu_BeforeRun); // Load Game Main Menu Dlg Run
		//_PI->WriteLoHook(0x4F0799, DlgMainMenu_BeforeRun);// Campaign Game Main Menu Dlg Run
		_PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc);
		_PI->WriteHiHook(0x5B3450, THISCALL_, DlgSystemOption_Proc);
	}

}


