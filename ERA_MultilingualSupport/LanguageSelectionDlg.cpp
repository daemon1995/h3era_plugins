#include "pch.h"

constexpr UINT16 FRAME_WIDGET_ID = 2;
constexpr UINT16 FIRST_SELECTION_WIDGET_ID = 3;
constexpr UINT16 LOCALEDLG_BUTTON_ID = 4445;

LanguageSelectionDlg::LanguageSelectionDlg(const int x, const int y, const int width, const int height,
                                           const DlgStyle *style, LocaleManager *handler)
    : H3Dlg(width, height, x, y, false, false), m_style(style), localeManager(handler)
{

    flags ^= 0x10; // disable dlg shadow
    CreateDlgItems();
    dlgText.Load();
}

/// function to get last locale from vector
// write commentaries

void CurrentDlg_HandleLocaleDlgStart(const H3Msg *msg, const DlgStyle *style, const int yOffset = 0)
{
    if (msg->itemId == LOCALEDLG_BUTTON_ID)
    {
        auto dlgPcx = msg->GetDlg()->GetTextPcx(msg->itemId);
        if (dlgPcx)
        {
            if (msg->IsLeftDown())
            {
                // create locale handler
                if (LocaleManager *localeHandler = new LocaleManager())
                {

                    const UINT LOCALES_NUM = localeHandler->GetCount();

                    const UINT16 MAX_ITEMS_PER_HEIGHT = style->maxRows;
                    const bool ITEMS_OVERFLOW = LOCALES_NUM > MAX_ITEMS_PER_HEIGHT;
                    const int ITEMS_NUM = ITEMS_OVERFLOW ? MAX_ITEMS_PER_HEIGHT : LOCALES_NUM;
                    P_SoundMgr->ClickSound();

                    // if there are locales
                    if (ITEMS_NUM)
                    {
                        // calc width based on the dlg asset width + assumed scroll width
                        const UINT16 DLG_WIDTH = style->width + 16 * ITEMS_OVERFLOW;

                        const UINT16 DLG_HEIGHT = style->height * ITEMS_NUM;

                        INT16 DLG_X = dlgPcx->GetAbsoluteX();
                        INT16 DLG_Y = dlgPcx->GetY() < 0 ? dlgPcx->GetAbsoluteY() - dlgPcx->GetY()
                                                         : dlgPcx->GetAbsoluteY() + dlgPcx->GetHeight();
                        // const INT16 DLG_Y = bttn->GetAbsoluteY();// +yOffset;
                        DLG_X = dlgPcx->GetAbsoluteX() - style->width;
                        DLG_Y = dlgPcx->GetAbsoluteY();

                        LanguageSelectionDlg langDlg(DLG_X, DLG_Y, DLG_WIDTH, DLG_HEIGHT, style, localeHandler);

                        std::string currentLocale = LocaleManager::ReadLocaleFromIni();
                        langDlg.Start();

                        std::string selectedLocale = LocaleManager::ReadLocaleFromIni();
                        if (currentLocale != selectedLocale)
                        {
                            dlgPcx->SetText(LocaleManager::GetDisplayedName());
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
                // H3Messagebox::RMB("");
            }
        }
    }
}

void __fastcall DlgSroll_Proc(INT32 tickId, H3BaseDlg *dlg)
{
    // try to cast dlg to our dlg
    if (auto localeDlg = dynamic_cast<LanguageSelectionDlg *>(dlg))
    {

        localeDlg->RedrawLocales(tickId);
        localeDlg->Redraw();
        return;
    }
}

BOOL LanguageSelectionDlg::OnMouseHover(H3DlgItem *it)
{
    if (it->GetID() >= firstLocaleItemId && it->GetID() <= lastLocaleItemId)
    {
        if (auto pcxLocale = it->Cast<H3DlgTextPcxLocale>())
        {
            localeManager->SetSelected(pcxLocale->GetLocale());

            PlaceFrameAtWidget(pcxLocale);
            this->Redraw();
        }
    }

    return 0;
}

void LanguageSelectionDlg::RedrawLocales(UINT16 firstItemId) noexcept
{
    // iterate all items by id
    for (uint16_t i = 0; i < localesToDraw; i++)
    {
        // assure those support locale edits
        if (auto selectionWidget = Get<H3DlgTextPcxLocale>(i + firstLocaleItemId))
        { // set new locale according on the scroll offset
            auto locale = Handler()->LocaleAt(i + firstItemId);
            selectionWidget->SetLocale(locale);
            if (selectionWidget->GetX() == selectionFrame->GetX() && selectionWidget->GetY() == selectionFrame->GetY())
            {
                PlaceFrameAtWidget(selectionWidget);
            }
        }
    }
}
void LanguageSelectionDlg::HideFrame() const noexcept
{
    selectionFrame->HideDeactivate();
    localeManager->SetSelected(nullptr);
}
void LanguageSelectionDlg::PlaceFrameAtWidget(const H3DlgTextPcxLocale *it) const noexcept
{
    selectionFrame->SetX(it->GetX());
    selectionFrame->SetY(it->GetY());
    selectionFrame->ShowActivate();
    localeManager->SetSelected(it->GetLocale());
}

BOOL LanguageSelectionDlg::DialogProc(H3Msg &msg)
{

    if (msg.IsLeftDown())
    {
        if (msg.itemId == FRAME_WIDGET_ID)
        {
            P_SoundManager->ClickSound();

            const Locale *locale = localeManager->GetSelected();

            bool samelocale = locale == localeManager->GetCurrent();

            LPCSTR formatPtr = samelocale ? dlgText.sameLocaleFormat : dlgText.questionformat;

            LPCSTR comment = h3_NullString;
            // check if locale description is missed
            if (!locale->hasDescription)
            {
                LPCSTR keyName = H3String::Format(localeManager->LocaleFormat(), locale->name.c_str()).String();
                comment = H3String::Format(dlgText.localeHasNoDescriptionFormat.String(), locale->name.c_str(), keyName)
                              .String();
            }

            sprintf(h3_TextBuffer, formatPtr, locale->name.c_str(), locale->displayedName.c_str(), comment);

            if (samelocale)
            {
                H3Messagebox::Show(h3_TextBuffer);
            }
            else
            {
                // ask question
                if (H3Messagebox::Choice(h3_TextBuffer))
                {
                    localeManager->SetForUser(locale);
                    Stop();
                }
            }
        }
        else if (exportDlgPcx && msg.itemId == exportDlgPcx->GetID())
        {
            ExportDlg exportDlg(630, 300, -1, -1, m_style);
            exportDlg.Start();
        }
    }

    if (msg.ClickOutside())
        Stop();

    return 0;
}
const LocaleManager *LanguageSelectionDlg::Handler() const noexcept
{
    return localeManager;
}
const DlgStyle *LanguageSelectionDlg::Style() const noexcept
{
    return m_style;
}
LanguageSelectionDlg::~LanguageSelectionDlg()
{
    if (exportDlgPcx)
    {
        if (auto pcx = exportDlgPcx->GetPcx())
        {
            pcx->Destroy();
            exportDlgPcx->SetPcx(nullptr);
        }
    }
}

void CurrentDlg_CreateDlgCallPcx(H3BaseDlg *dlg, int x, int y, const DlgStyle *style)
{

    auto *captionPcx = H3DlgTextPcx::Create(x, y, style->width, style->height, LocaleManager::GetDisplayedName(),
                                            style->fontName.String(), style->pcxName, 7, LOCALEDLG_BUTTON_ID);

    dlg->AddItem(captionPcx);
}

H3BaseDlg *DlgSystemOptions_Create(HiHook *h, H3BaseDlg *dlg)
{
    dlg = THISCALL_1(H3BaseDlg *, h->GetDefaultFunc(), dlg);
    CurrentDlg_CreateDlgCallPcx(dlg, 235, 269, &DlgStyle::styles[DlgStyle::BROWN_BACK]);
    return dlg;
}

void AddButtonToMainMenuDlg(H3BaseDlg *dlg)
{
    if (dlg) // before any dlg run get dlg) // if exists
    {

        const DlgStyle *style = &DlgStyle::styles[DlgStyle::BLUE_BACK];
        const auto pcx = style->pcxName;
        if (pcx)
        {
            const int x = dlg->GetWidth(); // -pcx->width;
            const int borderHeight = H3GameHeight::Get() - 600;

            // const int offset = pcx->height + 4;
            const int y = 0; // borderHeight > offset << 1 ? -offset : 0;
            CurrentDlg_CreateDlgCallPcx(dlg, x, y, style);
        }
    }
}

_LHF_(DlgSystemOptions_AtCreate)
{
    if (H3Dlg *dlg = *reinterpret_cast<H3Dlg **>(c->ebp - 0x20)) // before any dlg run get dlg
        CurrentDlg_CreateDlgCallPcx(dlg, 235, 269, &DlgStyle::styles[DlgStyle::BROWN_BACK]);

    return EXEC_DEFAULT;
}

int __stdcall DlgSystemOption_Proc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg)
{
    int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
    CurrentDlg_HandleLocaleDlgStart(msg, &DlgStyle::styles[DlgStyle::BROWN_BACK]);
    return result;
}

int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{
    CurrentDlg_HandleLocaleDlgStart(msg, &DlgStyle::styles[DlgStyle::BLUE_BACK], 20);
    return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}
int __stdcall DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg)
{
    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    AddButtonToMainMenuDlg(dlg);
    return result;
}
int __stdcall DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val)
{
    int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, val);
    AddButtonToMainMenuDlg(dlg);
    return result;
}
int __stdcall DlgMainMenu_Campaign_Run(HiHook *h, H3BaseDlg *dlg)
{
    AddButtonToMainMenuDlg(dlg);
    auto _h = _PI->WriteHiHook(0x5FFAC0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    _h->Destroy();
    return result;
}

void LanguageSelectionDlg::Init()
{
    if (DlgStyle::CreateAssets())
    {

        // Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
        //_PI->WriteLoHook(0x5B2F9B, DlgSystemOptions_AtCreate); //System options dlg from Adventure Mgr and Main Menu
        //(by HD mod only - RMB or MRM) _PI->WriteLoHook(0x4EF259, DlgMainMenu_BeforeRun); //MAIN Main Menu Dlg Run

        _PI->WriteHiHook(0x4FB930, THISCALL_, DlgMainMenu_Create);
        _PI->WriteHiHook(0x4D56D0, THISCALL_, DlgMainMenu_NewLoad_Create);
        //	_PI->WriteHiHook(0x456BA0, THISCALL_, DlgMainMenu_Campaign_Create); //goes from new game
        _PI->WriteHiHook(0x4F0799, THISCALL_, DlgMainMenu_Campaign_Run); // goes from new game

        _PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
        _PI->WriteHiHook(0x4D5B50, THISCALL_, DlgMainMenu_Proc); // New/ Load Game Menu Dlg Proc
        _PI->WriteHiHook(0x5B1AA0, THISCALL_,
                         DlgSystemOptions_Create); // Campaign (and some several) Game Menu Dlg Proc

        _PI->WriteHiHook(0x5B3450, THISCALL_, DlgSystemOption_Proc);
    }
}

void LanguageSelectionDlg::CreateDlgItems()
{
    const UINT widgetWidth = m_style->width;
    const UINT widgetHeight = m_style->height;

    const UINT SIZE = localeManager->GetCount();

    const UINT16 MAX_ITEMS_PER_HEIGHT = m_style->maxRows;
    const BOOL createExportButton = m_style->createExportButton;

    const int exportButtonHeight = createExportButton ? widgetHeight : 0; // +2 for export button and frame
    const int exportButtonWidth = createExportButton ? widgetWidth : 0;   // +2 for export button and frame
    // if items more than need
    const int overload = SIZE - MAX_ITEMS_PER_HEIGHT;
    if (overload > 0)
    {
        // create Scroll Bar
        constexpr int SCROLLBAR_WIDTH = 16;
        CreateScrollbar(widgetWidth, 0, SCROLLBAR_WIDTH, heightDlg - exportButtonHeight, 1, overload + 1,
                        DlgSroll_Proc);
    }

    if (localesToDraw = (SIZE < MAX_ITEMS_PER_HEIGHT ? SIZE : MAX_ITEMS_PER_HEIGHT) - createExportButton)
    {
        static_assert(FIRST_SELECTION_WIDGET_ID > 2, "First selection widget ID must be > 2");
        firstLocaleItemId = FIRST_SELECTION_WIDGET_ID;
        lastLocaleItemId = firstLocaleItemId - 1;

        for (size_t i = 0; i < localesToDraw; i++)
        {
            const int y = i * widgetHeight;
            const int itemId = firstLocaleItemId + i;
            auto textPcx =
                H3DlgTextPcxLocale::Create(0, y, widgetWidth, widgetHeight, localeManager->LocaleAt(i),
                                           m_style->fontName.String(), m_style->pcxName, 1, ++lastLocaleItemId);

            AddItem(textPcx);
        }
    }
    if (createExportButton)
    {
        const UINT16 exportButtonItemId =
            ((firstLocaleItemId != -1) ? lastLocaleItemId : FIRST_SELECTION_WIDGET_ID) + 1;
        // create special button to export available text
        exportDlgPcx = H3DlgPcx16::Create(0, localesToDraw * widgetHeight, exportButtonWidth, exportButtonHeight,
                                          exportButtonItemId, nullptr);

        auto pcx = H3LoadedPcx16::Create(exportButtonWidth, exportButtonHeight);

        libc::memset(pcx->buffer, 0, pcx->buffSize);
        pcx->SimpleFrameRegion(0, 0, exportButtonWidth, exportButtonHeight);

        pcx->BackgroundRegion(4, 4, exportButtonWidth - 8, exportButtonHeight - 8, 0); // H3RGB888(0xFF, 0xFF, 0xFF));
        H3FontLoader fnt(m_style->fontName.String());
        fnt->TextDraw(pcx, EraJS::read("era.locale.dlg.export.name"), 2, 2, exportButtonWidth - 4,
                      exportButtonHeight - 4);
        exportDlgPcx->SetPcx(pcx);

        AddItem(exportDlgPcx);
    }

    H3RGB565 color(H3RGB888::Highlight());
    selectionFrame = this->CreateFrame(0, 0, widgetWidth, widgetHeight, FRAME_WIDGET_ID, color);
    selectionFrame->HideDeactivate();

    // auto frame = CreateFrame(0, 0, widthDlg, heightDlg, -1, color);
    // frame->DeActivate();
}
void LanguageSelectionDlg::DlgText::Load() noexcept
{
    questionformat = EraJS::read("era.locale.dlg.question");
    localeHasNoDescriptionFormat = EraJS::read("era.locale.dlg.noDescription");
    sameLocaleFormat = EraJS::read("era.locale.dlg.sameLocale");
}
