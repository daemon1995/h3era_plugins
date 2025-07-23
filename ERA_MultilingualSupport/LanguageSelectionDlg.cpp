#include "pch.h"

constexpr UINT16 FRAME_WIDGET_ID = 2;
constexpr UINT16 FIRST_SELECTION_WIDGET_ID = 3;
constexpr UINT16 LOCALEDLG_BUTTON_ID = 4445;

std::vector<DlgStyle> LanguageSelectionDlg::styles;

bool LanguageSelectionDlg::CreateAssets(const BOOL forceRecreate)
{
    if (forceRecreate)
        styles.clear();

    if (styles.empty())
    {
        styles.emplace_back(DlgStyle(200, 23, 7, false, DlgStyle::BROWN_BACK, h3::NH3Dlg::Text::MEDIUM));
        styles.emplace_back(DlgStyle(150, 20, 25, true, DlgStyle::BLUE_BACK, h3::NH3Dlg::Text::MEDIUM));
    }
    return !styles.empty();
}

/// function to get last locale from vector
// This function retrieves the last locale from the available locales
// and returns it for further processing or display.
void CurrentDlg_HandleLocaleDlgStart(void *_msg)
{
    if (const auto msg = static_cast<H3Msg *>(_msg))
    {
        const auto callerItem = msg->GetDlg()->GetCaptionButton(msg->itemId);
        if (callerItem && msg->IsLeftClick())
        {
            auto &style = LanguageSelectionDlg::styles[DlgStyle::BLUE_BACK]; // use first style by default

            LocaleManager localeManager;

            const UINT localesCount = localeManager.GetCount() + (style.createExportButton);

            const UINT16 maxLocalesToDisplay = style.maxRows;
            const bool createSlider = localesCount > maxLocalesToDisplay;

            // if there are locales
            if (const INT32 itemsToDisplay = createSlider ? maxLocalesToDisplay : localesCount)
            {
                // calc width based on the dlg asset width + assumed scroll width
                const UINT16 dlgWidth = style.width + 16 * createSlider;

                const UINT16 dlgHeight = style.height * itemsToDisplay;

                const int callerX = callerItem->GetAbsoluteX();
                const int callerY = callerItem->GetAbsoluteY();
                const int callerWidth = callerItem->GetWidth();
                const int callerHeight = callerItem->GetHeight();
                const int gameWidth = H3GameWidth::Get();
                const int gameHeight = H3GameHeight::Get();
                constexpr int backgroundWidth = 800;
                constexpr int backgroundHeight = 600;
                const int backgroundX = gameWidth - backgroundWidth >> 1;
                const int backgroundY = gameHeight - backgroundHeight >> 1;

                INT16 dlgX = callerX + callerWidth;

                // calculate dialog position
                if (dlgX + dlgWidth > backgroundX + backgroundWidth)
                {
                    dlgX = backgroundX + backgroundWidth - dlgWidth;
                }

                // dlgX = callerItem->GetAbsoluteX();

                // const INT16 dlgY = bttn->GetAbsoluteY();// +yOffset;
                //   dlgX = callerItem->GetAbsoluteX() - style.width;
                INT16 dlgY = backgroundY;

                LanguageSelectionDlg langDlg(dlgX, dlgY, dlgWidth, dlgHeight, style, &localeManager);

                const std::string currentLocale = LocaleManager::ReadLocaleFromIni();
                langDlg.Start();

                const std::string selectedLocale = LocaleManager::ReadLocaleFromIni();
                if (currentLocale != selectedLocale)
                {
                    callerItem->SetText(LocaleManager::GetDisplayedName());
                    callerItem->Draw();
                    callerItem->ParentRedraw();
                }
            }
            else
            {
                H3Messagebox("ERROR:\nNo locales");
            }
        }
    }
}

LanguageSelectionDlg::LanguageSelectionDlg(const int x, const int y, const int width, const int height,
                                           const DlgStyle &style, LocaleManager *handler)
    : H3Dlg(width, height, x, y, false, false), style(style), localeManager(handler)
{

    flags ^= 0x10; // disable dlg shadow
    fontLoader = style.fontName.String();

    CreateDlgItems();
    dlgText.Load();
}

void LanguageSelectionDlg::CreateDlgItems()
{
    const UINT widgetWidth = style.width;
    const UINT widgetHeight = style.height;

    const UINT SIZE = localeManager->GetCount();
    const BOOL createExportButton = style.createExportButton;

    const UINT16 MAX_ITEMS_PER_HEIGHT =
        Clamp(createExportButton, style.maxRows, heightDlg / widgetHeight) - createExportButton;

    const int exportButtonHeight = createExportButton ? widgetHeight : 0; // +2 for export button and frame
    const int exportButtonWidth = createExportButton ? widgetWidth : 0;   // +2 for export button and frame
    // if items more than need
    const int overload = SIZE - MAX_ITEMS_PER_HEIGHT;
    if (overload > 0)
    {
        // create Scroll Bar
        constexpr int SCROLLBAR_WIDTH = 16;
        CreateScrollbar(widgetWidth, 0, SCROLLBAR_WIDTH, heightDlg - exportButtonHeight, 1, overload + 1, DlgSroll_Proc,
                        style.isBlueBack);
    }

    if (localesToDraw = SIZE < MAX_ITEMS_PER_HEIGHT ? SIZE : MAX_ITEMS_PER_HEIGHT)
    {
        static_assert(FIRST_SELECTION_WIDGET_ID > 2, "First selection widget ID must be > 2");
        firstLocaleItemId = FIRST_SELECTION_WIDGET_ID;
        lastLocaleItemId = firstLocaleItemId - 1;

        for (size_t i = 0; i < localesToDraw; i++)
        {
            const int y = i * widgetHeight;
            const int itemId = firstLocaleItemId + i;

            auto textPcx = H3DlgPcx16Locale::Create(0, y, style, &localeManager->LocaleAt(i), fontLoader.Get(),
                                                    ++lastLocaleItemId);
            displayedLocales.push_back(textPcx);
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

        fontLoader->TextDraw(pcx, EraJS::read(ExportDlg::BUTTON_NAME), 2, 2, exportButtonWidth - 4,
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

void __fastcall LanguageSelectionDlg::DlgSroll_Proc(INT32 tickId, H3BaseDlg *dlg)
{
    // try to cast dlg to our dlg
    if (auto localeDlg = dynamic_cast<LanguageSelectionDlg *>(dlg))
    {
        localeDlg->RedrawLocales(tickId);
        localeDlg->Redraw();
    }
}

BOOL LanguageSelectionDlg::OnMouseHover(H3DlgItem *it)
{
    if (it->GetID() >= firstLocaleItemId && it->GetID() <= lastLocaleItemId)
    {
        if (auto pcxLocale = it->Cast<H3DlgPcx16Locale>())
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
        if (auto selectionWidget = Get<H3DlgPcx16Locale>(i + firstLocaleItemId))
        { // set new locale according on the scroll offset
            auto &locale = localeManager->LocaleAt(i + firstItemId);
            selectionWidget->SetLocale(&locale);
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
void LanguageSelectionDlg::PlaceFrameAtWidget(const H3DlgPcx16Locale *it) const noexcept
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

            const BOOL samelocale = locale == localeManager->GetCurrent();

            LPCSTR formatPtr = samelocale ? dlgText.sameLocaleFormat : dlgText.questionformat;

            LPCSTR comment = h3_NullString;
            // check if locale description is broken
            if (locale->broken)
            {
                comment = H3String::Format(EraJS::read(LocaleManager::error::name), locale->name.c_str()).String();
            }

            libc::sprintf(h3_TextBuffer, formatPtr, locale->name.c_str(), locale->displayedName.c_str(), comment);

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
            P_SoundManager->ClickSound();
            // create export dialog
            ExportDlg exportDlg(630, 300, -1, -1, style);
            exportDlg.Start();
        }
    }
    if (msg.IsKeyDown())
    {
        const eVKey keyCode = msg.GetKey();
        if (keyCode == eVKey::H3VK_ESCAPE || keyCode == eVKey::H3VK_SPACEBAR || keyCode == eVKey::H3VK_ENTER)
        {
            Stop();
        }
    }
    if (msg.ClickOutside())
        Stop();

    return 0;
}

LanguageSelectionDlg::~LanguageSelectionDlg()
{
    fontLoader.Release();
    if (exportDlgPcx)
    {
        if (auto pcx = exportDlgPcx->GetPcx())
        {
            pcx->Destroy();
            exportDlgPcx->SetPcx(nullptr);
        }
    }
    // destroy created pictures
    for (auto &i : displayedLocales)
    {
        if (auto pcx = i->GetPcx())
        {
            pcx->Destroy();
            i->SetPcx(nullptr);
        }
    }
}

void LanguageSelectionDlg::DlgText::Load() noexcept
{
    questionformat = EraJS::read("era.locale.dlg.question");
    localeHasNoDescriptionFormat = EraJS::read("era.locale.dlg.noDescription");
    sameLocaleFormat = EraJS::read("era.locale.dlg.sameLocale");
}
void LanguageSelectionDlg::Init()
{
    if (CreateAssets())
    {
        using namespace mainmenu;

        eMenuList menuList = eMenuList(eMenuList::Main | eMenuList::NewGame | eMenuList::LoadGame);
        MenuWidgetInfo langInfo{UNIQUE_BUTTON_NAME, LocaleManager::GetDisplayedName(), menuList,
                            &CurrentDlg_HandleLocaleDlgStart};

        RegisterMainMenuWidget(langInfo);
        for (size_t i = 0; i < 6; i++)
        {
            libc::sprintf(h3_TextBuffer, "era.locale.dlg.style.%d", i);
            mainmenu::MenuWidgetInfo info{h3_TextBuffer, LocaleManager::GetDisplayedName(), mainmenu::eMenuList::Main,
                                          &CurrentDlg_HandleLocaleDlgStart};

            mainmenu::RegisterMainMenuWidget(info);
        }
    }
}

DlgStyle::DlgStyle(const UINT width, const UINT height, const UINT maxRows, const BOOL createExportButton,
                   const BOOL isBlueBack, LPCSTR fontName)
    : width(width), height(height), maxRows(maxRows), createExportButton(createExportButton), isBlueBack(isBlueBack),
      fontName(fontName)
{
    this->maxRows = Clamp(0, maxRows + createExportButton, (600 / height));
    pcxName = isBlueBack ? NH3Dlg::HDassets::DLGBLUEBACK : NH3Dlg::Assets::DIBOXBACK;
}
