#include "NotificationPanel.h"

#include <fstream>
#include <shlobj.h>

NotificationPanel *NotificationPanel::instance = nullptr;
H3DlgPcx16 *CreateOpenPanelTextPcxButton(H3BaseDlg *dlg, const size_t notificationAmount);
bool FolderExists(const std::wstring &path);
std::wstring GetAppDataPath();

NotificationPanel::ModInfo::ModInfo(LPCSTR folderName)
{

    bool readSuccess = false;

    libc::sprintf(h3_TextBuffer, jsonFormat::MOD_TEXT, folderName);
    LPSTR notificationText = EraJS::read(h3_TextBuffer, readSuccess);
    const UINT len = libc::strlen(notificationText);
    if (readSuccess && len > 0)
    {
        modFolderName = folderName;
        displayedText = notificationText;

        libc::sprintf(h3_TextBuffer, jsonFormat::MOD_NAME, folderName);
        displayedName = EraJS::read(h3_TextBuffer);

        libc::sprintf(h3_TextBuffer, jsonFormat::MOD_URL, folderName);
        LPSTR link = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess && libc::strlen(link) > 0)
        {
            externalLink = link;
        }

        currentDescriptionHash = Era::Hash32(displayedText, len);

        if (Era::ReadStrFromIni(folderName, INI_SECTION_NAME, INI_FILE_NAME, h3_TextBuffer))
        {
            isHiddenByUser = atoi(h3_TextBuffer) == currentDescriptionHash;
        }
    }

    isVisible = !isHiddenByUser;
}
BOOL NotificationPanel::ModInfo::MarkAsHiddenByUser() noexcept
{
    isVisible = false;

    if (!savedAsHiddenByUser)
    {
        savedAsHiddenByUser = true;
        libc::sprintf(h3_TextBuffer, "%d", currentDescriptionHash);
        Era::WriteStrToIni(modFolderName.c_str(), h3_TextBuffer, INI_SECTION_NAME, INI_FILE_NAME);

        return true;
    }
    return false;
}

BOOL NotificationPanel::ModInfo::ReloadDescription() noexcept
{
    bool readSuccess = false;
    libc::sprintf(h3_TextBuffer, jsonFormat::MOD_TEXT, modFolderName.c_str());
    LPCSTR notificationText = EraJS::read(h3_TextBuffer, readSuccess);
    const UINT len = libc::strlen(notificationText);

    if (readSuccess && len > 0)
    {
        displayedText = notificationText;

        libc::sprintf(h3_TextBuffer, jsonFormat::MOD_NAME, modFolderName.c_str());

        H3FontLoader fn(NH3Dlg::Text::MEDIUM);

        const int oldModNameWidth = fn->GetMaxLineWidth(displayedName) + 2;
        displayedName = EraJS::read(h3_TextBuffer);

        const int modNameWidth = fn->GetMaxLineWidth(displayedName) + 2;
        const int xOffset = (oldModNameWidth - modNameWidth) / 2;

        nameUnderline->SetWidth(modNameWidth);
        nameUnderline->SetX(nameUnderline->GetX() + xOffset);

        libc::sprintf(h3_TextBuffer, jsonFormat::MOD_URL, modFolderName.c_str());
        LPCSTR link = EraJS::read(h3_TextBuffer, readSuccess);
        if (readSuccess && libc::strlen(link) > 0)
        {
            externalLink = link;
        }

        currentDescriptionHash = Era::Hash32(displayedText, len);
        return true;
    }
    return false;
}

bool FolderExists(const std::wstring &path)
{
    DWORD attribs = GetFileAttributesW(path.c_str());
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring GetAppDataPath()
{
    wchar_t *appDataPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath) != S_OK)
    {
        return L"";
    }

    std::wstring result(appDataPath);
    CoTaskMemFree(appDataPath);
    return result;
}

NotificationPanel::NotificationPanel(H3BaseDlg *parent, const int x, const int y, const int width,
                                     const int height) noexcept
    : x(x), y(y), width(width), height(height), isVisible(false), parentDlg(parent)
{

    std::vector<std::string> modList;
    modList::GetEraModList(modList, true);

    // проверка на сущестовавание лаунчера у пользователя
    // Получаем путь к AppData
    bool readSucces = false;
    std::string launcherFolder = EraJS::read(panelText::LAUNCHER_PATH, readSucces);
    if (readSucces && !launcherFolder.empty())
    {
        // Формируем полный путь к папке
        std::wstring appDataPath = GetAppDataPath();
        if (!appDataPath.empty())
        {

            std::wstring targetFolder =
                appDataPath + L"\\" + std::wstring(launcherFolder.begin(), launcherFolder.end());
            // Проверяем существование папки
            if (!FolderExists(targetFolder))
            {
                modList.emplace_back(std::string{"heroes launcher"});
            }
        }
    }

    std::reverse(modList.begin(), modList.end()); // reverse modList
    CreateModInfoList(modList);

    if (notificationsTotal)
    {
        parrentCaller = CreateOpenPanelTextPcxButton(parent, notificationsTotal);

        // must have a backup of the original screen to restore on close
        backupScreen = H3LoadedPcx16::Create(width, height);

        // making hd mod wrong main menu drawing offset fix
        const int _x = x + (H3GameWidth::Get() - 800) / 2;
        const int _y = y + (H3GameHeight::Get() - 600) / 2;
        backupScreen->CopyRegion(P_WindowManager->GetDrawBuffer(), _x, _y);

        H3RGB565 highLightColor(H3RGB888::Highlight());
        H3RGB565 lightBorderColor(H3RGB888(247, 222, 123));
        H3RGB565 darkBorderColor(H3RGB888(165, 140, 66));

        // used for the whole notification panel
        auto &background = runtimes.panelBackground;
        if (background.item = H3DlgPcx16::Create(x, y, nullptr))
        {

            background.pcx = H3LoadedPcx16::Create(width, height);
            // libc::memset(background.pcx->buffer, 0, background.pcx->buffSize);

            constexpr UINT DRAW_OFFSET = 256;
            int xStart = 0, yStart = 0, xEnd = width - 1, yEnd = height - 1;
            do
            {
                do
                {
                    background.pcx->BackgroundRegion(xStart, yStart, DRAW_OFFSET, DRAW_OFFSET, true);

                    xStart = Clamp(0, xStart + DRAW_OFFSET, xEnd);
                } while (xStart < xEnd);
                xStart = 0;
                yStart = Clamp(0, yStart + DRAW_OFFSET, yEnd);

            } while (yStart < yEnd);
            //

            // background.pcx->DarkenArea(45, 45, width - 90, height - 90, 0xff);
            background.item->SetWidth(width);
            background.item->SetHeight(height);
            background.item->SetPcx(background.pcx);

            AddItem(background.item, true);

            auto frame = H3DlgFrame::Create(x + 1, y + 1, width - 2, height - 2, lightBorderColor);
            AddItem(frame, true);

            frame = H3DlgFrame::Create(frame, darkBorderColor, 0, 1);
            AddItem(frame, true);
        }

        libc::sprintf(h3_TextBuffer, "%s", EraJS::read(panelText::TITLE));
        constexpr int TITLE_HEIGHT = 40;
        if (panelTitle = H3DlgText::Create(x + FRAME_OFFSET, y + FRAME_OFFSET, width - FRAME_OFFSET * 2, TITLE_HEIGHT,
                                           h3_TextBuffer,
                                           h3::NH3Dlg::Text::BIG)) // , eTextColor::REGULAR))
        {

            auto frame = H3DlgFrame::Create(panelTitle, highLightColor, 0, true);

            AddItem(frame, true);
            AddItem(panelTitle, true);
        }

        const int switchX = width - FRAME_OFFSET * 2 - 120;
        const int switchY = y + FRAME_OFFSET + 4;
        if (previousModButton = H3DlgDefButton::Create(switchX, switchY, "ntf_left.def", 0, 1))
        {
            auto frame = H3DlgFrame::Create(previousModButton, highLightColor, 0, 1);

            AddItem(frame, true);
            AddItem(previousModButton, true);
        }
        if (nextModButton = H3DlgDefButton::Create(switchX + 100, switchY, "ntf_rght.def", 0, 1))
        {
            auto frame = H3DlgFrame::Create(nextModButton, highLightColor, 0, 1);
            AddItem(frame, true);
            AddItem(nextModButton, true);
        }

        if (notificationsCounter = H3DlgTextPcx::Create(switchX + 37, y + FRAME_OFFSET, 60, 40, h3_NullString,
                                                        h3::NH3Dlg::Text::BIG, PCX_BACK, eTextColor::REGULAR))
        {
            AddItem(notificationsCounter, true);
        }

        constexpr int BTTN_HEIGHT = 40;
        constexpr UINT MOD_AREA_OFFSET = 25;
        CreateModAreaFrame(x + MOD_AREA_OFFSET, y + TITLE_HEIGHT + FRAME_OFFSET * 2 + MOD_AREA_OFFSET / 2,
                           width - MOD_AREA_OFFSET * 2,
                           height + MOD_AREA_OFFSET / 2 - (BTTN_HEIGHT + MOD_AREA_OFFSET) * 2);

        const int bttnWidth = 208; // -FRAME_OFFSET  2;

        const int BTTN_X = x + MOD_AREA_OFFSET;
        const int BTTN_Y = y + height - BTTN_HEIGHT - FRAME_OFFSET * 6;
        if (hideAllButton = H3DlgCaptionButton::Create(BTTN_X + FRAME_OFFSET, BTTN_Y, 0, "ntf_hdal.def",
                                                       EraJS::read(panelText::HIDE_ALL), NH3Dlg::Text::MEDIUM, 0, 0,
                                                       false, 0, eTextColor::REGULAR))
        {
            hideAllButton->SetClickFrame(1);
            auto frame = H3DlgFrame::Create(hideAllButton, highLightColor, 0, 1);
            AddItem(frame, true);
            AddItem(hideAllButton, true);
        }

        if (hideOneButton = H3DlgCaptionButton::Create(x + width - MOD_AREA_OFFSET - bttnWidth - FRAME_OFFSET, BTTN_Y,
                                                       0, "ntf_hide.def", EraJS::read(panelText::HIDE_ONE),
                                                       NH3Dlg::Text::MEDIUM, 0, 0, false, 0, eTextColor::REGULAR))
        {

            hideOneButton->SetClickFrame(1);
            auto frame = H3DlgFrame::Create(hideOneButton, highLightColor, 0, 1);
            AddItem(frame, true);
            AddItem(hideOneButton, true);
        }

        for (auto &i : modInfos)
        {
            if (i.displayedText)
            {
                CreateModDlgItems(parentDlg, i, runtimes.modBackground.item);
            }
        }

        NotificationPanel::instance = this;
        this->UpdateVisibleNotificationsList();
        if (this->notificationsVisible)
        {
            SetVisible(true);
        }
    }
}

void NotificationPanel::CreateModInfoList(std::vector<std::string> &modList) noexcept
{
    notificationsTotal = 0;
    if (const size_t length = modList.size())
    {
        volatile int index = 0;
        modInfos.reserve(length);

        for (size_t i = 0; i < length; i++)
        {
            ModInfo modInfo{modList[i].c_str()};

            if (modInfo.displayedText)
            {
                modInfo.index = ++index;
                modInfos.emplace_back(modInfo);
            }
        }
    }
    notificationsTotal = modInfos.size();
}

NotificationPanel::~NotificationPanel() noexcept
{

    for (auto runtime : runtimes.asArray)
    {
        if (runtime->item && runtime->pcx && runtime->item->GetPcx() == runtime->pcx)
        {
            runtime->item->SetPcx(nullptr);
            runtime->pcx->Destroy();
            runtime->pcx = nullptr;
        }
    }

    if (backupScreen)
    {
        backupScreen->Destroy();
        backupScreen = nullptr;
    }
    if (parrentCaller)
    {
        if (auto pcx = parrentCaller->GetPcx())
        {
            pcx->Destroy();
            parrentCaller->SetPcx(nullptr);
        }
    }
    instance = nullptr;
}

H3DlgPcx16 *CreateOpenPanelTextPcxButton(H3BaseDlg *dlg, const size_t notificationAmount)
{

    constexpr UINT width = 140;
    constexpr UINT height = 20;
    constexpr UINT border_width = 0;

    constexpr UINT16 offset = height + 4 + border_width * 2;

    const int borderHeight = H3GameHeight::Get() - 600;
    constexpr int x = 0;
    const int y = borderHeight > offset << 1 ? -1 * offset : 0;

    auto *captionPcx = H3DlgPcx16::Create(x, y, width, height, 0, nullptr);

    // H3RGB565 highLightColor(H3RGB888::Highlight());
    // H3RGB565 lightBorderColor(H3RGB888(247, 222, 123));
    // H3RGB565 darkBorderColor(H3RGB888(165, 140, 66));

    auto pcx = H3LoadedPcx16::Create(width, height);
    pcx->BackgroundRegion(0, 0, width, height, true);
    pcx->DarkenArea(0, 0, width, height, 50);
    pcx->BevelArea(1, 1, width - 2, height - 2);
    H3FontLoader fnt(NH3Dlg::Text::MEDIUM);
    libc::sprintf(h3_TextBuffer, EraJS::read(panelText::BUTTON_FORMAT), notificationAmount);
    fnt->TextDraw(pcx, h3_TextBuffer, 0, 0, width, height);

    captionPcx->SetPcx(pcx);

    dlg->AddItem(captionPcx);

    return captionPcx;
}

H3DlgItem *NotificationPanel::AddItem(H3DlgItem *item, const bool isCommon) noexcept
{
    if (parentDlg && item)
    {
        parentDlg->AddItem(item);
        item->HideDeactivate();
        if (isCommon)
        {
            items += item;
        }
    }
    return nullptr;
}

void NotificationPanel::UpdateVisibleNotificationsList() noexcept
{
    notificationsVisible = 0;
    for (auto &i : modInfos)
    {
        if (!i.isHiddenByUser && i.isVisible)
        {
            notificationsVisible++;
        }
    }
    if (isVisible && notificationsCounter)
    {
        libc::sprintf(h3_TextBuffer, NOTIFICATION_COUNTER_FORMAT, currentModInfoIndex, notificationsVisible);
        notificationsCounter->SetText(h3_TextBuffer);
    }
}

NotificationPanel::ModInfo *NotificationPanel::GetModInfoFromVisible(const UINT index) noexcept
{
    ModInfo *result = nullptr;
    volatile int i = 0;
    for (auto &it : modInfos)
    {
        if (it.isVisible && it.index == index)
        {
            result = &it;
            break;
        }
    }

    return result;
}

void NotificationPanel::CreateModAreaFrame(const UINT x, const UINT y, const UINT width, const UINT height) noexcept
{
    auto &image = runtimes.modBackground;
    if (image.item = H3DlgPcx16::Create(x, y, nullptr))
    {

        image.pcx = H3LoadedPcx16::Create(width, height);
        libc::memset(image.pcx->buffer, 0, image.pcx->buffSize);

        //   H3PcxLoader back("DlgDBlBk.pcx");

        int w = width;
        int h = height;
        //
        int _y = 0;
        int _h = height;

        while (_h > 0)
        {
            int _x = 0;
            int dh = std::min(256, _h);
            int _w = w;
            while (_w > 0)
            {
                _x += 256;
                _w -= 256;
            }
            _y += 256;
            _h -= 256;
        }

        image.pcx->FrameRegion(0, 0, width, height, false, 4, true);

        image.item->SetWidth(width);
        image.item->SetHeight(height);
        image.item->SetPcx(image.pcx);
        AddItem(image.item, true);
    }
}

void NotificationPanel::CreateModDlgItems(H3BaseDlg *dlg, ModInfo &modInfo, H3DlgPcx16 *modBackground) noexcept
{

    // first create each item w/o adding it into dlg/dl panel in order to not overlap with other items
    constexpr int offset = 10;
    const int x = modBackground->GetX() + offset;
    const int y = modBackground->GetY() + offset - 4;
    const int width = modBackground->GetWidth() - offset * 2;
    const int height = modBackground->GetHeight() - offset + 2;

    eTextColor textColor = eTextColor::REGULAR;
    if (modInfo.externalLink)
    {
        H3RGB565 linkColor(H3RGB888(0, 0xAA, 0xFF));
        H3FontLoader fn(NH3Dlg::Text::MEDIUM);

        const int modNameWidth = fn->GetMaxLineWidth(modInfo.displayedName) + 2;
        const int underX = (width - modNameWidth) / 2 + x;
        auto urlUnderline = H3DlgFrame::Create(underX, y + 24, modNameWidth, 1, linkColor);
        modInfo.items.emplace_back(urlUnderline);
        AddItem(urlUnderline);
        modInfo.nameUnderline = urlUnderline;
        textColor = eTextColor::BLUE2;
    }

    modInfo.modNameDlgText =
        H3DlgText::Create(x, y + 6, width, 20, modInfo.displayedName, NH3Dlg::Text::MEDIUM, textColor);
    modInfo.items.emplace_back(modInfo.modNameDlgText);

    H3RGB565 highLightColor(H3RGB888::Highlight());
    auto delimiterFrame = H3DlgFrame::Create(x + 10, y + 30, width - 16, 1, highLightColor);
    modInfo.items.emplace_back(delimiterFrame);

    modInfo.descriptionTextScrollBar =
        H3DlgScrollableText::Create(modInfo.displayedText, x + 10, y + 40, width - 16, height - 53,
                                    NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, true);

    // modInfo.descriptionTextScrollBar->SetText(modInfo.displayedText);
    modInfo.items.emplace_back(modInfo.descriptionTextScrollBar);

    if (auto scroll = modInfo.descriptionTextScrollBar)
    {
        if (auto items = scroll->GetItems())
        {
            for (auto &i : *items)
            {
                i->HideDeactivate();
            }
        }

        if (auto i = scroll->GetTextScrollBar())
        {
            i->HideDeactivate();
        }
    }

    // add items into dlg as hidden items
    AddItem(delimiterFrame);

    AddItem(modInfo.modNameDlgText);
    AddItem(modInfo.descriptionTextScrollBar);
}

void NotificationPanel::SetVisible(const BOOL visible, const BOOL activateAllNotifications) noexcept
{
    if (this->isVisible != visible)
    {
        this->isVisible = visible;
        auto &background = runtimes.panelBackground.item;

        background->SetPcx(visible ? runtimes.panelBackground.pcx : backupScreen);

        volatile int index = 0;

        if (activateAllNotifications)
        {
            for (auto &i : modInfos)
            {
                i.index = ++index;
                i.isHiddenByUser = false;
                i.isVisible = visible; // (visible && !i.isHiddenByUser) || activateAllNotifications;
            }
        }
        else
        {
            for (auto &i : modInfos)
            {
                if (visible)
                {
                    if (!i.isHiddenByUser)
                        i.index = ++index;
                }
                else
                {
                    i.isHiddenByUser = false;
                }
                i.isVisible = visible && !i.isHiddenByUser;
            }
        }

        if (this->notificationsTotal)
        {
            // if setting panel visible and there are notifications
            if (visible)
            {
                if (currentModInfo == nullptr)
                {
                    currentModInfo = this->GetModInfoFromVisible(1);
                    if (currentModInfo == nullptr)
                    {
                        currentModInfo = &modInfos[0];
                    }
                    currentModInfoIndex = currentModInfo->index;
                }
                this->UpdateVisibleNotificationsList();
            }
            else if (notificationsVisible != this->notificationsTotal)
            {
                // now manage mod items
                if (currentModInfo)
                {
                    SetModVisible(*currentModInfo, visible);
                }

                currentModInfoIndex = 0;
                currentModInfo = nullptr;
            }
        }

        // manage all items in panel
        for (auto &i : items)
        {
            visible ? i->ShowActivate(), i->Draw(), i->Refresh() : i->HideDeactivate();
        }

        // now manage mod items
        if (currentModInfo)
        {
            SetModVisible(*currentModInfo, visible);
        }

        // if panel is not visible draw backup screen
        if (visible == false)
        {
            background->Draw();
            background->Refresh();
        }
    }
}
void NotificationPanel::SetModVisible(ModInfo &modInfo, const BOOL isVisible) noexcept
{

    if (this->isVisible && isVisible == false)
    {
        auto &modBackground = this->runtimes.modBackground.item;
        modBackground->Draw();
        modBackground->Refresh();
    }

    for (auto &i : modInfo.items)
    {
        isVisible ? i->ShowActivate(), i->Draw(), i->Refresh() : i->HideDeactivate();
    }

    if (auto scroll = modInfo.descriptionTextScrollBar)
    {
        if (auto items = scroll->GetItems())
        {
            for (auto &i : *items)
            {
                isVisible ? i->ShowActivate(), i->Draw(), i->Refresh() : i->HideDeactivate();
            }
        }
        if (isVisible)
        {
            if (auto canvas = scroll->GetPcx())
            {
                libc::memset(canvas->buffer, 0, canvas->buffSize);
            }
        }

        // manage scrollbar if it has more than one tick (visible)
        if (auto i = scroll->GetTextScrollBar())
        {
            if (i->GetTicksCount() > 1)
            {
                isVisible ? i->ShowActivate(), i->Draw(), i->Refresh() : i->HideDeactivate();
            }
        }
    }
}

void NotificationPanel::Retranslate(const BOOL redraw) noexcept
{
    bool readSuccess = false;
    LPCSTR str = EraJS::read(panelText::HIDE_ALL, readSuccess);
    if (readSuccess)
    {
        hideAllButton->SetText(str);
        if (redraw)
        {
            hideAllButton->Draw();
            hideAllButton->Refresh();
        }
    }
    str = EraJS::read(panelText::HIDE_ONE, readSuccess);
    if (readSuccess)
    {
        hideOneButton->SetText(str);
        if (redraw)
        {
            hideOneButton->Draw();
            hideOneButton->Refresh();
        }
    }
    str = EraJS::read(panelText::TITLE, readSuccess);
    if (readSuccess)
    {
        panelTitle->SetText(str);
        if (redraw)
        {
            panelTitle->Draw();
            panelTitle->Refresh();
        }
    }
    // str = EraJS::read(panelText::HIDE_ALL, readSuccess);
    // if (readSuccess)
    //{
    //     hideAllButton->SetText(str);
    // }
}
void NotificationPanel::SwitchModInfo(const int step) noexcept
{
    const int infosToShow = notificationsVisible;
    if (step && infosToShow)
    {

        const int newModInfoIndex = Clamp(1, currentModInfoIndex + step, infosToShow);

        if (newModInfoIndex != currentModInfoIndex)
        {
            auto assumedModInfo = GetModInfoFromVisible(newModInfoIndex);
            if (currentModInfo != assumedModInfo)
            {
                if (currentModInfo)
                {
                    SetModVisible(*currentModInfo, false);
                }

                currentModInfo = assumedModInfo;
                currentModInfoIndex = currentModInfo->index;
                SetModVisible(*currentModInfo, true);

                if (notificationsCounter)
                {
                    UpdateVisibleNotificationsList();

                    notificationsCounter->Draw();
                    notificationsCounter->Refresh();
                }
            }
        }
    }
}
void OpenExternalFile(const char *path, const char *msg = nullptr);
BOOL NotificationPanel::ProcessPanel(H3Msg *msg, const BOOL forceRedraw) noexcept
{

    BOOL result = false;
    if (msg)
    {

        const auto processItem = msg->ItemAtPosition(msg->GetDlg());

        result = currentModInfo && currentModInfo->modNameDlgText == processItem && currentModInfo->externalLink;

        if (msg->IsLeftDown())
        {

            if (processItem == this->parrentCaller)
            {
                P_SoundManager->ClickSound();
                this->SetVisible(!this->isVisible, true);
            }
            else if (result)
            {
                P_SoundManager->ClickSound();

                OpenExternalFile(currentModInfo->externalLink);
            }
        }
        else if (msg->IsLeftClick())
        {

            if (processItem == hideAllButton)
            {
                this->SetVisible(false);
                int modsWrittenToIni = 0;
                // if mod has text, mark it as hidden
                for (auto &i : modInfos)
                {
                    modsWrittenToIni += i.MarkAsHiddenByUser();
                }
                if (modsWrittenToIni)
                {
                    Era::SaveIni(INI_FILE_NAME);
                }
            }
            else if (processItem == hideOneButton)
            {

                // first, mark current mod as hidden
                if (currentModInfo->MarkAsHiddenByUser())
                {
                    Era::SaveIni(INI_FILE_NAME);
                }

                // if there are more than one visible notification
                if (notificationsVisible > 1)
                {
                    // hide current mod to make space for the next one

                    SetModVisible(*currentModInfo, false);
                    UpdateVisibleNotificationsList();

                    volatile int i = 0;
                    for (auto &it : modInfos)
                    {
                        it.index = it.isVisible && !it.isHiddenByUser ? ++i : 0;
                    }

                    if (currentModInfoIndex > notificationsVisible)
                    {
                        currentModInfoIndex = currentModInfoIndex - 1;
                    }

                    if (currentModInfo = GetModInfoFromVisible(currentModInfoIndex))
                    {
                        SetModVisible(*currentModInfo, true);
                        UpdateVisibleNotificationsList();
                    }
                    if (notificationsCounter)
                    {
                        notificationsCounter->Draw();
                        notificationsCounter->Refresh();
                    }
                }
                else
                {
                    this->SetVisible(false);
                }
            }
            else if (processItem == nextModButton)
            {
                SwitchModInfo(1);
            }
            else if (processItem == previousModButton)
            {
                SwitchModInfo(-1);
            }
        }

        if (isVisible)
        {
            if (forceRedraw)
            {
                for (auto &i : items)
                {
                    i->Draw();
                }

                if (currentModInfo && currentModInfo->isVisible)
                {

                    for (auto &i : currentModInfo->items)
                    {
                        i->Draw();
                    }

                    if (auto scroll = currentModInfo->descriptionTextScrollBar)
                    {
                        if (auto items = scroll->GetItems())
                        {
                            for (auto &i : *items)
                            {
                                i->Draw();
                            }
                        }

                        // manage scrollbar if it has more than one tick (visible)
                        if (auto i = scroll->GetTextScrollBar())
                        {
                            if (i->GetTicksCount() > 1)
                            {
                                i->Draw();
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

void NotificationPanel::ReloadLanguageData() noexcept
{

    const bool isVisibleBefore = isVisible;

    Retranslate(isVisibleBefore);

    if (isVisibleBefore)
    {
        SetVisible(false);
    }

    for (auto &i : this->modInfos)
    {
        if (i.displayedText && i.ReloadDescription())
        {
            i.descriptionTextScrollBar->SetText(i.displayedText);
            i.modNameDlgText->SetText(i.displayedName);
        }
    }
    if (isVisibleBefore)
    {
        SetVisible(true);
    }
}

NotificationPanel *NotificationPanel::Init(H3BaseDlg *parrent, const int x, const int y, const int width,
                                           const int height) noexcept
{
    if (instance)
    {
        delete instance;
        instance = nullptr;
    }
    instance = new NotificationPanel(parrent, x, y, width, height);
    return instance;
}
