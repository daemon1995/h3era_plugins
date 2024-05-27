#include "pch.h"
#include "LanguageSelectionDlg.h"

//constexpr UINT32 WIDGET_WIDTH = 213;
//constexpr UINT32 WIDGET_HEIGHT = 23;
constexpr UINT16 FRAME_WIDGET_ID = 2;
constexpr UINT16 FIRST_SELECTION_WIDGET_ID = 3;
constexpr UINT16 LOCALEDLG_BUTTON_ID = 4445;


LanguageSelectionDlg::LanguageSelectionDlg(const int width, const int height, const int x, const int y, const DlgStyle* style, LocaleHandler* handler)
	:H3Dlg(width, height, x, y, false, false), m_style(style), m_localeHandler(handler)
{

	flags ^= 0x10; // disable dlg shadow
	CreateDlgItems();
	m_text.Load();

}


/// function to get last locale from vector
// write commentaries

void CurrentDlg_HandleLocaleDlgStart(const H3Msg* msg, const DlgStyle* style, const int yOffset = 0)
{
	if (msg->itemId == LOCALEDLG_BUTTON_ID)
	{
		auto dlgPcx = msg->GetDlg()->GetTextPcx(msg->itemId);
		if (dlgPcx)
		{
			if (msg->IsLeftDown())
			{
				// create locale handler
				if (LocaleHandler* localeHandler = new LocaleHandler())
				{

					const UINT LOCALES_NUM = localeHandler->GetCount();

					const UINT16 MAX_ITEMS_PER_HEIGHT = style->MAXIMUM_ROWS;
					const bool ITEMS_OVERFLOW = LOCALES_NUM > MAX_ITEMS_PER_HEIGHT;
					const int ITEMS_NUM = ITEMS_OVERFLOW ? MAX_ITEMS_PER_HEIGHT : LOCALES_NUM;
					P_SoundMgr->ClickSound();


					// if there are locales
					if (ITEMS_NUM)
					{
						// calc width based on the dlg asset width + assumed scroll width
						const UINT16 DLG_WIDTH = style->WIDGET_WIDTH + 16 * ITEMS_OVERFLOW;

						const UINT16 DLG_HEIGHT = style->WIDGET_HEIGHT * ITEMS_NUM;

						const INT16 DLG_X = dlgPcx->GetAbsoluteX();
						const INT16 DLG_Y = dlgPcx->GetY() < 0 ? dlgPcx->GetAbsoluteY() - dlgPcx->GetY() : dlgPcx->GetAbsoluteY() + dlgPcx->GetHeight();
						//const INT16 DLG_Y = bttn->GetAbsoluteY();// +yOffset;

						LanguageSelectionDlg langDlg(DLG_WIDTH, DLG_HEIGHT, DLG_X, DLG_Y, style, localeHandler);// = direction ? {500, 500} : {0};


						std::string currentLocale = LocaleHandler::ReadLocaleFromIni("heroes3.ini");
						langDlg.Start();
						std::string selectedLocale = LocaleHandler::ReadLocaleFromIni("heroes3.ini");
						if (currentLocale != selectedLocale)
						{
							dlgPcx->SetText(LocaleHandler::GetDisplayedName());
							dlgPcx->Draw();
							dlgPcx->ParentRedraw();
						}

					}
					else
					{
						H3Messagebox("ERROR:\nNo locales");
					}
					delete localeHandler;
				}


			}
			else if (msg->IsRightClick())
			{
				//H3Messagebox::RMB("");
			}
		}
	}
}



void __fastcall DlgSroll_Proc(INT32 tickId, H3BaseDlg* dlg)
{
	// try to cast dlg to our dlg
	if (auto localeDlg = dynamic_cast<LanguageSelectionDlg*>(dlg))
	{

		localeDlg->RedrawLocales(tickId);
		localeDlg->Redraw();
		return;
	}
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

void LanguageSelectionDlg::RedrawLocales(UINT32 firstItemId)  noexcept
{
	// iterate all items by id
	for (size_t i = 0; i < m_LOCALES_TO_DRAW; i++)
	{
		// assure those support lcoale edits
		if (auto selectionWidget = Get<H3DlgTextPcxLocale>(i + FIRST_SELECTION_WIDGET_ID))
		{	// set new locale according on the scroll offset
			auto locale = Handler()->LocaleAt(i + firstItemId);
			selectionWidget->SetLocale(locale);
			if (selectionWidget->GetX() == m_selectionFrame->GetX()

				&& selectionWidget->GetY() == m_selectionFrame->GetY())
			{
				PlaceFrameAtWidget(selectionWidget);
			}
		}
	}
}
void LanguageSelectionDlg::HideFrame() const noexcept
{
	m_selectionFrame->HideDeactivate();
	m_localeHandler->SetSelected(nullptr);
}
void LanguageSelectionDlg::PlaceFrameAtWidget(const H3DlgTextPcxLocale* it) const noexcept
{
	m_selectionFrame->SetX(it->GetX());
	m_selectionFrame->SetY(it->GetY());
	m_selectionFrame->ShowActivate();
	m_localeHandler->SetSelected(it->GetLocale());

}

BOOL LanguageSelectionDlg::DialogProc(H3Msg& msg)
{

	if (msg.IsLeftDown() && msg.itemId == FRAME_WIDGET_ID)
	{
		P_SoundManager->ClickSound();

		const Locale* locale = m_localeHandler->GetSelected();

		bool samelocale = locale == m_localeHandler->GetCurrent();

		LPCSTR formatPtr = samelocale ? m_text.sameLocaleFormat : m_text.questionformat;

		LPCSTR comment = h3_NullString;
		// check if locale description is missed
		if (!locale->hasDescription)
		{
			LPCSTR keyName = H3String::Format(m_localeHandler->LocaleFormat(), locale->name.c_str()).String();
			comment = H3String::Format(m_text.localeHasNoDescriptionFormat.String(), locale->name.c_str(), keyName).String();
		}
		sprintf(h3_TextBuffer, formatPtr, locale->displayedName.c_str(), locale->name.c_str(), comment);


		if (samelocale)
		{
			H3Messagebox::Show(h3_TextBuffer);

		}
		else
		{
			//ask question
			if (H3Messagebox::Choice(h3_TextBuffer))
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


void CurrentDlg_CreateDlgCallPcx(H3BaseDlg* dlg, int x, int y, const DlgStyle* style)
{

	// set nop for loading pcx to skip default proc
	auto* blockLoadingPcx = _PI->WriteHexPatch(0x5BCBD0, "90 90 90 90 90");
	auto* captionPcx = H3DlgTextPcx::Create(x, y, style->WIDGET_WIDTH, style->WIDGET_HEIGHT, LocaleHandler::GetDisplayedName(), h3::NH3Dlg::Text::SMALL, nullptr, 7, LOCALEDLG_BUTTON_ID);

	//restore orginal code;
	blockLoadingPcx->Undo();
	blockLoadingPcx->Destroy();
	// set pcx
	*reinterpret_cast<H3LoadedPcx**>(reinterpret_cast<int>(captionPcx) + 0x50) = style->localeBackgroundLoadedPcx;
	dlg->AddItem(captionPcx);
}




H3BaseDlg* DlgSystemOptions_Create(HiHook* h, H3BaseDlg* dlg)
{
	dlg = THISCALL_1(H3BaseDlg*, h->GetDefaultFunc(), dlg);
	CurrentDlg_CreateDlgCallPcx(dlg, 235, 269, &DlgStyle::styles[0]);
	return dlg;
}

_LHF_(DlgMainMenu_BeforeRun)
{
	if (auto dlg = reinterpret_cast<H3Dlg*>(c->ecx))    // before any dlg run get dlg) // if exists
	{

		const DlgStyle* style = &DlgStyle::styles[2];
		const auto pcx = style->localeBackgroundLoadedPcx;
		//auto def = H3LoadedDef::Load(style->dlgCallAssetPcxName);
		if (pcx)
		{
			const int x = dlg->GetWidth() - pcx->width;
			const int borderHeight = H3GameHeight::Get() - 600;

			const int offset = pcx->height + 4;
			const int y = borderHeight > offset << 1 ? -offset : 0;
			CurrentDlg_CreateDlgCallPcx(dlg, x, y, style);
		}

	}
	return EXEC_DEFAULT;
}
_LHF_(DlgSystemOptions_AtCreate)
{
	if (H3Dlg* dlg = *reinterpret_cast<H3Dlg**>(c->ebp - 0x20))    // before any dlg run get dlg
		CurrentDlg_CreateDlgCallPcx(dlg, 235, 269, &DlgStyle::styles[0]);

	return EXEC_DEFAULT;
}



int __stdcall DlgSystemOption_Proc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg)
{
	int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
	CurrentDlg_HandleLocaleDlgStart(msg, &DlgStyle::styles[0]);
	return  result;
}

int __stdcall DlgMainMenu_Proc(HiHook* h, H3Msg* msg)
{
	CurrentDlg_HandleLocaleDlgStart(msg, &DlgStyle::styles[2], 20);
	return FASTCALL_1(int, h->GetDefaultFunc(), msg);

}


void LanguageSelectionDlg::Init()
{
	if (DlgStyle::CreateAssets())
	{
		//Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
		//_PI->WriteLoHook(0x5B2F9B, DlgSystemOptions_AtCreate); //System options dlg from Adventure Mgr and Main Menu (by HD mod only - RMB or MRM)
		//_PI->WriteLoHook(0x4EF259, DlgMainMenu_BeforeRun); //MAIN Main Menu Dlg Run
		_PI->WriteLoHook(0x4EF259, DlgMainMenu_BeforeRun); // Main Main Menu
		_PI->WriteLoHook(0x4EF331, DlgMainMenu_BeforeRun); // New Game
		_PI->WriteLoHook(0x4EF668, DlgMainMenu_BeforeRun); // Load Game
		//_PI->WriteLoHook(0x4F0799, DlgMainMenu_BeforeRun); // Campaign

		_PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
		_PI->WriteHiHook(0x4D5B50, THISCALL_, DlgMainMenu_Proc); // New/ Load Game Menu Dlg Proc
		_PI->WriteHiHook(0x5B1AA0, THISCALL_, DlgSystemOptions_Create); // Campaign (and some several) Game Menu Dlg Proc

		_PI->WriteHiHook(0x5B3450, THISCALL_, DlgSystemOption_Proc);
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
		CreateScrollbar(WIDGET_WIDTH, 0, SCROLLBAR_WIDTH, heightDlg, 1, overload + 1, DlgSroll_Proc);
	}

	m_LOCALES_TO_DRAW = SIZE < MAX_ITEMS_PER_HEIGHT ? SIZE : MAX_ITEMS_PER_HEIGHT;


	for (size_t i = 0; i < m_LOCALES_TO_DRAW; i++)
	{
		const int y = i * WIDGET_HEIGHT;
		auto textPcx = H3DlgTextPcxLocale::Create(0, y, WIDGET_WIDTH, WIDGET_HEIGHT, m_localeHandler->LocaleAt(i), h3::NH3Dlg::Text::MEDIUM, m_style->localeBackgroundLoadedPcx, 1, i + FIRST_SELECTION_WIDGET_ID);
		AddItem(textPcx);
	}

	H3RGB565 color(H3RGB888::Highlight());
	m_selectionFrame = this->CreateFrame(0, 0, WIDGET_WIDTH, WIDGET_HEIGHT, FRAME_WIDGET_ID, color);
	m_selectionFrame->HideDeactivate();

	//auto frame = CreateFrame(0, 0, widthDlg, heightDlg, -1, color);
	//frame->DeActivate();
}
void LanguageSelectionDlg::DlgText::Load() noexcept
{
	questionformat = EraJS::read("era.locale.dlg.question");
	localeHasNoDescriptionFormat = EraJS::read("era.locale.dlg.noDescription");
	sameLocaleFormat = EraJS::read("era.locale.dlg.sameLocale");
}
