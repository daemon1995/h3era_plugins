#include "NotificationPanel.h"
#include "MainMenuConfig.h"
#include "../headers/era/eraModList.hpp"
#include "../headers/EraPluginsAPI/NotificationLinks.hpp"

#include <algorithm>
#include <fstream>
#include <shlobj.h>
#include <windows.h>

NotificationPanel *NotificationPanel::instance = nullptr;
bool FolderExists(const std::wstring &path);
std::wstring GetAppDataPath();

namespace
{
struct PanelColor { int r = 0, g = 0, b = 0; };

PanelColor ReadPanelColor(const H3LoadedPcx16 *image, int x, int y, int bytesPerPixel)
{
    const auto *pixel = image->buffer + y * image->scanlineSize + x * bytesPerPixel;
    if (bytesPerPixel == 4)
        return {pixel[2], pixel[1], pixel[0]};
    H3RGB565 color;
    color = *reinterpret_cast<const unsigned short *>(pixel);
    const auto rgb = color.GetRGB888();
    return {static_cast<int>((rgb >> 16) & 255), static_cast<int>((rgb >> 8) & 255),
            static_cast<int>(rgb & 255)};
}

void WritePanelColor(H3LoadedPcx16 *image, int x, int y, PanelColor color, int bytesPerPixel)
{
    auto *pixel = image->buffer + y * image->scanlineSize + x * bytesPerPixel;
    if (bytesPerPixel == 4)
    {
        pixel[0] = static_cast<unsigned char>(color.b);
        pixel[1] = static_cast<unsigned char>(color.g);
        pixel[2] = static_cast<unsigned char>(color.r);
        pixel[3] = 255;
    }
    else
    {
        const H3RGB565 packed(H3RGB888(color.r, color.g, color.b));
        *reinterpret_cast<unsigned short *>(pixel) = packed;
    }
}

void DrawBlurredPanel(H3LoadedPcx16 *target, const H3LoadedPcx16 *screen, int sourceX, int sourceY)
{
    const int bpp = H3BitMode::Get();
    const int smallWidth = std::max(1, (target->width + 5) / 6);
    const int smallHeight = std::max(1, (target->height + 5) / 6);
    std::vector<PanelColor> samples(smallWidth * smallHeight), horizontal(samples.size()), blurred(samples.size());
    for (int y = 0; y < smallHeight; ++y)
        for (int x = 0; x < smallWidth; ++x)
            samples[y * smallWidth + x] = ReadPanelColor(screen,
                sourceX + std::min(target->width - 1, x * 6 + 3),
                sourceY + std::min(target->height - 1, y * 6 + 3), bpp);
    constexpr int radius = 3;
    for (int y = 0; y < smallHeight; ++y)
        for (int x = 0; x < smallWidth; ++x)
        {
            PanelColor color;
            for (int n = -radius; n <= radius; ++n)
            {
                const auto sample = samples[y * smallWidth + Clamp(0, x + n, smallWidth - 1)];
                color.r += sample.r; color.g += sample.g; color.b += sample.b;
            }
            horizontal[y * smallWidth + x] = {color.r / 7, color.g / 7, color.b / 7};
        }
    for (int y = 0; y < smallHeight; ++y)
        for (int x = 0; x < smallWidth; ++x)
        {
            PanelColor color;
            for (int n = -radius; n <= radius; ++n)
            {
                const auto sample = horizontal[Clamp(0, y + n, smallHeight - 1) * smallWidth + x];
                color.r += sample.r; color.g += sample.g; color.b += sample.b;
            }
            blurred[y * smallWidth + x] = {color.r / 7, color.g / 7, color.b / 7};
        }
    // Bilinear upsampling avoids visible blocks; darkening keeps captions readable.
    for (int y = 0; y < target->height; ++y)
    {
        const int fy = y * std::max(0, smallHeight - 1) * 256 / std::max(1, target->height - 1);
        const int y0 = fy / 256, y1 = std::min(y0 + 1, smallHeight - 1), wy = fy & 255;
        for (int x = 0; x < target->width; ++x)
        {
            const int fx = x * std::max(0, smallWidth - 1) * 256 / std::max(1, target->width - 1);
            const int x0 = fx / 256, x1 = std::min(x0 + 1, smallWidth - 1), wx = fx & 255;
            const auto a = blurred[y0 * smallWidth + x0], b = blurred[y0 * smallWidth + x1];
            const auto c = blurred[y1 * smallWidth + x0], d = blurred[y1 * smallWidth + x1];
            const auto mix = [wx, wy](int av, int bv, int cv, int dv) {
                const int value = ((av * (256 - wx) + bv * wx) * (256 - wy) +
                                   (cv * (256 - wx) + dv * wx) * wy) / 65536;
                return value * 42 / 100 + 6;
            };
            WritePanelColor(target, x, y, {mix(a.r,b.r,c.r,d.r), mix(a.g,b.g,c.g,d.g),
                                          mix(a.b,b.b,c.b,d.b)}, bpp);
        }
    }
}
} // namespace

void NotificationPanel::PrepareAppearance(H3BaseDlg *dlg, H3LoadedPcx16 *cleanScreen) noexcept
{
    if (dlg != parentDlg || !cleanScreen || !runtimes.panelBackground.pcx || !backupScreen)
        return;
    if (!themeReady)
        PlaceAtPictureCorner();
    const int absoluteX = x + dlg->GetX(), absoluteY = y + dlg->GetY();
    if (absoluteX < 0 || absoluteY < 0 || absoluteX + width > cleanScreen->width ||
        absoluteY + height > cleanScreen->height)
        return;
    backupScreen->CopyRegion(cleanScreen, absoluteX, absoluteY);
    auto *asset = mainmenu::background::GetAsset();
    if (!themeReady || themeScreenWidth != cleanScreen->width || themeScreenHeight != cleanScreen->height ||
        themeAsset != asset)
    {
        DrawBlurredPanel(runtimes.panelBackground.pcx, cleanScreen, absoluteX, absoluteY);
        auto &body = runtimes.modBackground;
        if (body.item && body.pcx)
        {
            body.pcx->CopyRegion(runtimes.panelBackground.pcx, body.item->GetX() - x, body.item->GetY() - y);
            body.pcx->DarkenArea(0, 0, body.pcx->width, body.pcx->height, 35);
            body.pcx->DrawFrame(0, 0, body.pcx->width, body.pcx->height);
        }
        for (auto &button : buttonImages)
            button.valid = false;
        themeReady = true;
        themeScreenWidth = cleanScreen->width;
        themeScreenHeight = cleanScreen->height;
        themeAsset = asset;
        UpdateCounterImage();
        for (auto &info : modInfos)
            PrepareTextBackground(info);
    }
    if (openAfterFirstDraw)
    {
        openAfterFirstDraw = false;
        SetVisible(true);
    }
}

bool NotificationPanel::DrawButtonBackground(H3LoadedDef *def, int frame, int sx, int sy, int drawWidth,
                                            int drawHeight, void *buffer, int dx, int dy, int dw, int dh,
                                            int pitch) noexcept
{
    if (!isVisible || !themeReady || !def)
        return false;
    auto *screen = P_WindowManager->GetDrawBuffer();
    if (!screen || buffer != screen->buffer)
        return false;
    for (auto &image : buttonImages)
    {
        auto *button = image.button;
        if (!button || !button->IsVisible() || button->GetDef() != def)
            continue;
        const bool enabled = button == previousModButton ? currentModInfoIndex > 1 :
                             button == nextModButton ? currentModInfoIndex > 0 &&
                                                         currentModInfoIndex < notificationsVisible : true;
        const bool pressed = enabled && frame == 1;
        if (!image.pcx)
            image.pcx = H3LoadedPcx16::Create(button->GetWidth(), button->GetHeight());
        if (!image.pcx)
            return false;
        if (!image.valid || image.pressed != pressed || image.enabled != enabled)
        {
            image.pcx->CopyRegion(runtimes.panelBackground.pcx, button->GetX() - x, button->GetY() - y);
            if (pressed || !enabled)
                image.pcx->DarkenArea(0, 0, image.pcx->width, image.pcx->height, pressed ? 20 : 45);
            image.pcx->BevelArea(1, 1, image.pcx->width - 2, image.pcx->height - 2);
            image.pcx->DrawFrame(0, 0, image.pcx->width, image.pcx->height);
            if (button == previousModButton || button == nextModButton)
            {
                H3FontLoader font(NH3Dlg::Text::BIG);
                font->TextDraw(image.pcx, button == previousModButton ? "<" : ">", 0, pressed ? 1 : 0,
                                image.pcx->width, image.pcx->height - 1,
                                enabled ? eTextColor::HIGHLIGHT : eTextColor::REGULAR,
                                eTextAlignment::MIDDLE_CENTER);
            }
            image.pressed = pressed;
            image.enabled = enabled;
            image.valid = true;
        }
        const int bpp = H3BitMode::Get();
        const int left = std::max(0, std::max(-dx, -sx)), top = std::max(0, std::max(-dy, -sy));
        const int right = std::min(drawWidth, std::min(std::min(dw, screen->width) - dx, image.pcx->width - sx));
        const int bottom = std::min(drawHeight, std::min(std::min(dh, screen->height) - dy, image.pcx->height - sy));
        if (right > left)
            for (int row = top; row < bottom; ++row)
                libc::memcpy(screen->buffer + (dy + row) * pitch + (dx + left) * bpp,
                             image.pcx->buffer + (sy + row) * image.pcx->scanlineSize + (sx + left) * bpp,
                             (right - left) * bpp);
        return true;
    }
    return false;
}

NotificationPanel::ModInfo::ModInfo(LPCSTR description, LPCSTR folderName, const int index)
    : modIndex(index), displayedText(description), modFolderName(folderName)
{

    bool readSuccess = false;

    libc::sprintf(h3_TextBuffer, modIndex == NONE_INDEX ? jsonFormat::UNIQIUE_MOD_TEXT : jsonFormat::ORDERED_MOD_TEXT,
                  folderName, modIndex);

    // LPCSTR notificationText = EraJS::read(h3_TextBuffer, readSuccess);
    //  if (readSuccess && len > 0)

    libc::sprintf(h3_TextBuffer, modIndex == NONE_INDEX ? jsonFormat::UNIQUE_MOD_NAME : jsonFormat::ORDERED_MOD_NAME,
                  folderName, modIndex);
    displayedName = mainmenu::config::Text(h3_TextBuffer, modFolderName.c_str());

    libc::sprintf(h3_TextBuffer, modIndex == NONE_INDEX ? jsonFormat::UNIQUE_MOD_URL : jsonFormat::ORDERED_MOD_URL,
                  folderName, modIndex);
    LPCSTR link = EraJS::read(h3_TextBuffer, readSuccess);
    if (readSuccess && libc::strlen(link) > 0)
    {
        externalLink = link;
    }

    const UINT len = libc::strlen(displayedText);
    currentDescriptionHash = Era::Hash32(displayedText, len);

    libc::sprintf(h3_TextBuffer, "%d", currentDescriptionHash, modFolderName.c_str(), modIndex);

    char buf[16];
    if (Era::ReadStrFromIni(h3_TextBuffer, INI_SECTION_NAME, INI_FILE_NAME, buf))
    {
        isHiddenByUser = atoi(buf); // == currentDescriptionHash;
    }

    isVisible = !isHiddenByUser;
}
BOOL NotificationPanel::ModInfo::MarkAsHiddenByUser() noexcept
{
    isVisible = false;
    isHiddenByUser = true;

    if (!savedAsHiddenByUser)
    {
        savedAsHiddenByUser = true;
        libc::sprintf(h3_TextBuffer, "%d", currentDescriptionHash, modFolderName.c_str(), modIndex);
        Era::WriteStrToIni(h3_TextBuffer, Era::IntToStr(true).c_str(), INI_SECTION_NAME, INI_FILE_NAME);

        return true;
    }
    return false;
}

BOOL NotificationPanel::ModInfo::ReloadDescription() noexcept
{
    bool readSuccess = false;
    libc::sprintf(h3_TextBuffer, modIndex == NONE_INDEX ? jsonFormat::UNIQIUE_MOD_TEXT : jsonFormat::ORDERED_MOD_TEXT,
                  modFolderName.c_str(), modIndex);

    LPCSTR notificationText = EraJS::read(h3_TextBuffer, readSuccess);
    const UINT len = libc::strlen(notificationText);

    if (readSuccess && len > 0)
    {
        displayedText = notificationText;

        libc::sprintf(h3_TextBuffer,
                      modIndex == NONE_INDEX ? jsonFormat::UNIQUE_MOD_NAME : jsonFormat::ORDERED_MOD_NAME,
                      modFolderName.c_str(), modIndex);

        displayedName = mainmenu::config::Text(h3_TextBuffer, modFolderName.c_str());

        const int modNameWidth = Clamp(1, modNameDlgText->GetFont()->GetMaxLineWidth(displayedName) + 2,
                                       nameAreaWidth);
        const int nameX = nameAreaX + (nameAreaWidth - modNameWidth) / 2;
        modNameDlgText->SetWidth(modNameWidth);
        modNameDlgText->SetX(nameX);

        if (nameUnderline)
        {
            nameUnderline->SetWidth(modNameWidth);
            nameUnderline->SetX(nameX);
        }

        libc::sprintf(h3_TextBuffer, modIndex == NONE_INDEX ? jsonFormat::UNIQUE_MOD_URL : jsonFormat::ORDERED_MOD_URL,
                      modFolderName.c_str(), modIndex);

        LPCSTR link = EraJS::read(h3_TextBuffer, readSuccess);

        externalLink = readSuccess && libc::strlen(link) > 0 ? link : nullptr;

        const UINT newHash = Era::Hash32(displayedText, len);
        if (newHash != currentDescriptionHash)
            savedAsHiddenByUser = false;
        currentDescriptionHash = newHash;
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
    wchar_t path[MAX_PATH];

    if (SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path) != S_OK)
    {
        return L"";
    }

    return path;
}

static H3DlgCaptionButton *GetNotificationPanelCallerWidget()
{
    return reinterpret_cast<H3DlgCaptionButton *>(
        MenuWidgetManager::Get().GetWidgetByName(NotificationPanel::PARENT_BUTTON_CALLER_NAME));
}

void NotificationPanel::RegisterWidget()
{
    const auto flags = static_cast<mainmenu::eMenuFlags>(mainmenu::MAIN | mainmenu::AT_BOTTOM);
    const mainmenu::MenuWidgetInfo info{PARENT_BUTTON_CALLER_NAME, h3_NullString, flags, OnPanelCallerClick};
    MenuWidgetManager::Get().RegisterWidget(info);
    Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
}

void __stdcall NotificationPanel::OnAfterReloadLanguageData(Era::TEvent *e)
{
    if (instance)
        instance->ReloadLanguageData();
}

NotificationPanel::NotificationPanel(H3BaseDlg *parent, const int x, const int y, const int width,
                                     const int height) noexcept
    : x(x), y(y), width(width), height(height), isVisible(false), parentDlg(parent)
{

    std::vector<std::string> modList;
    modList::GetEraModList(modList, true);

    // �������� �� �������������� �������� � ������������
    // �������� ���� � AppData
    // AddHeroesLauncherNotification(modList); // Temporarily disabled.

    CreateModInfoList(modList);

    if (notificationsTotal)
    {
        if (parentCaller = GetNotificationPanelCallerWidget())
        {
            libc::sprintf(h3_TextBuffer, mainmenu::config::Text(panelText::BUTTON_FORMAT, "Notifications (%d)"), notificationsTotal);
            parentCaller->SetText(h3_TextBuffer);
        }

        backupScreen = H3LoadedPcx16::Create(width, height);

        H3RGB565 highLightColor(H3RGB888::Highlight());
        H3RGB565 lightBorderColor(H3RGB888(247, 222, 123));
        H3RGB565 darkBorderColor(H3RGB888(165, 140, 66));
        // used for the whole notification panel
        auto &background = runtimes.panelBackground;
        if (background.item = H3DlgPcx16::Create(x, y, nullptr))
        {

            background.pcx = H3LoadedPcx16::Create(width, height);
            libc::memset(background.pcx->buffer, 0, background.pcx->buffSize);
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

        libc::sprintf(h3_TextBuffer, "%s", mainmenu::config::Text(panelText::TITLE, "Notifications"));
        constexpr int TITLE_HEIGHT = 40;
        if (panelTitle = H3DlgText::Create(x + FRAME_OFFSET, y + FRAME_OFFSET, width - 172, TITLE_HEIGHT,
                                           h3_TextBuffer,
                                           h3::NH3Dlg::Text::BIG)) // , eTextColor::REGULAR))
        {

            auto frame = H3DlgFrame::Create(panelTitle, highLightColor, 0, true);

            AddItem(frame, true);
            AddItem(panelTitle, true);
        }

        const int switchX = width - FRAME_OFFSET * 2 - 157 + x;
        const int switchY = y + FRAME_OFFSET + 4;
        if (previousModButton =
                H3DlgDefButton::Create(switchX, switchY, PREVIOUS_BUTTON_ID, "ntf_left.def", 0, 1, false, 0))
        {
            auto frame = H3DlgFrame::Create(previousModButton, highLightColor, 0, 1);

            AddItem(frame, true);
            AddItem(previousModButton, true);
        }
        if (nextModButton =
                H3DlgDefButton::Create(switchX + 100, switchY, NEXT_BUTTON_ID, "ntf_rght.def", 0, 1, false, 0))
        {
            auto frame = H3DlgFrame::Create(nextModButton, highLightColor, 0, 1);
            AddItem(frame, true);
            AddItem(nextModButton, true);
        }

        auto &counter = runtimes.counter;
        counter.pcx = H3LoadedPcx16::Create(60, 40);
        if (counter.pcx && (counter.item = H3DlgPcx16::Create(switchX + 37, y + FRAME_OFFSET, nullptr)))
        {
            counter.item->SetPcx(counter.pcx);
            counter.item->SetWidth(counter.pcx->width);
            counter.item->SetHeight(counter.pcx->height);
            notificationsCounter = counter.item;
            AddItem(notificationsCounter, true);
        }

        constexpr UINT BTTN_HEIGHT = 40;
        constexpr UINT MOD_AREA_OFFSET = 25;
        CreateModAreaFrame(x + MOD_AREA_OFFSET, y + TITLE_HEIGHT + FRAME_OFFSET * 2 + MOD_AREA_OFFSET / 2,
                           width - MOD_AREA_OFFSET * 2,
                           height + MOD_AREA_OFFSET / 2 - (BTTN_HEIGHT + MOD_AREA_OFFSET) * 2);

        const int bttnWidth = std::min(208, (width - static_cast<int>(MOD_AREA_OFFSET * 2) - FRAME_OFFSET * 2 - 12) / 2);

        const int BTTN_X = x + MOD_AREA_OFFSET;
        const int BTTN_Y = y + height - BTTN_HEIGHT - FRAME_OFFSET * 6;

        if (hideOneButton = H3DlgCaptionButton::Create(
                x + width - MOD_AREA_OFFSET - bttnWidth - FRAME_OFFSET, BTTN_Y, HIDE_ONE_BUTTON_ID, "ntf_hide.def",
                mainmenu::config::Text(panelText::HIDE_ONE, "Hide"), NH3Dlg::Text::MEDIUM, 0, 0, false, 0, eTextColor::REGULAR))
        {

            hideOneButton->SetClickFrame(1);
            hideOneButton->SetWidth(bttnWidth);
            auto frame = H3DlgFrame::Create(hideOneButton, highLightColor, 0, 1);
            AddItem(frame, true);
            AddItem(hideOneButton, true);
        }

        if (hideAllButton = H3DlgCaptionButton::Create(BTTN_X + FRAME_OFFSET, BTTN_Y, HIDE_ALL_BUTTON_ID,
                                                       "ntf_hdal.def", mainmenu::config::Text(panelText::HIDE_ALL, "Hide all"),
                                                       NH3Dlg::Text::MEDIUM, 0, 0, false, 0, eTextColor::REGULAR))
        {
            hideAllButton->SetClickFrame(1);
            hideAllButton->SetWidth(bttnWidth);
            auto frame = H3DlgFrame::Create(hideAllButton, highLightColor, 0, 1);
            AddItem(frame, true);
            AddItem(hideAllButton, true);
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
        buttonImages[0].button = previousModButton;
        buttonImages[1].button = nextModButton;
        buttonImages[2].button = hideOneButton;
        buttonImages[3].button = hideAllButton;
        openAfterFirstDraw = this->notificationsVisible != 0;
    }
}

void NotificationPanel::AddHeroesLauncherNotification(std::vector<std::string> &modList)
{
    bool readSucces = false;
    std::string launcherFolder = EraJS::read(panelText::LAUNCHER_PATH, readSucces);
    if (readSucces && !launcherFolder.empty())
    {
        // ��������� ������ ���� � �����
        std::wstring appDataPath = GetAppDataPath();
        if (!appDataPath.empty())
        {

            std::wstring targetFolder =
                appDataPath + L"\\" + std::wstring(launcherFolder.begin(), launcherFolder.end());
            // ��������� ������������� �����
            if (!FolderExists(targetFolder))
            {
                modList.insert(modList.begin(), "heroes launcher");
            }
        }
    }
}

void NotificationPanel::CreateModInfoList(std::vector<std::string> &modList) noexcept
{
    notificationsTotal = 0;
    if (const size_t length = modList.size())
    {
        UINT infoIndex = 0;
        modInfos.reserve(length);
        for (size_t i = 0; i < length; i++)
        {

            LPCSTR modFolderNamePtr = modList[i].c_str();

            bool readSuccess = false;

            libc::sprintf(h3_TextBuffer, jsonFormat::UNIQIUE_MOD_TEXT, modFolderNamePtr);

            LPCSTR notificationText = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess && strlen(notificationText))
            {
                modInfos.emplace_back(notificationText, modFolderNamePtr, ModInfo::NONE_INDEX);
                modInfos.back().displayedIndex = ++infoIndex;
            }
            else
            {
                int modIndex = 0;

                do
                {
                    libc::sprintf(h3_TextBuffer, jsonFormat::ORDERED_MOD_TEXT, modFolderNamePtr, modIndex);

                    LPCSTR notificationText = EraJS::read(h3_TextBuffer, readSuccess);
                    if (readSuccess && strlen(notificationText))
                    {
                        modInfos.emplace_back(notificationText, modFolderNamePtr, modIndex);
                        modInfos.back().displayedIndex = ++infoIndex;
                    }

                } while (readSuccess && ++modIndex < ModInfo::NOTIFICATIONS_PER_MOD);
            }
        }
        modInfos.shrink_to_fit();
    }
    notificationsTotal = modInfos.size();
}

NotificationPanel::~NotificationPanel() noexcept
{
    for (auto &button : buttonImages)
        if (button.pcx)
            button.pcx->Destroy();

    for (auto runtime : runtimes.asArray)
    {
        // An item may currently show backupScreen rather than its owned image.
        if (runtime->item)
            runtime->item->SetPcx(nullptr);
        if (runtime->pcx)
        {
            runtime->pcx->Destroy();
            runtime->pcx = nullptr;
        }
    }

    if (backupScreen)
    {
        backupScreen->Destroy();
        backupScreen = nullptr;
    }

    instance = nullptr;
}

void RedrawPcxText(H3LoadedPcx16 *pcx, const int notificationAmount)
{
    if (pcx)
    {
        const UINT width = pcx->width;
        const UINT height = pcx->height;

        pcx->BackgroundRegion(0, 0, width, height, true);
        pcx->DarkenArea(0, 0, width, height, 50);
        pcx->BevelArea(1, 1, width - 2, height - 2);
        H3FontLoader fnt(NH3Dlg::Text::MEDIUM);
        libc::sprintf(h3_TextBuffer, mainmenu::config::Text(panelText::BUTTON_FORMAT, "Notifications (%d)"), notificationAmount);
        fnt->TextDraw(pcx, h3_TextBuffer, 0, 0, width, height);
    }
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

void NotificationPanel::UpdateCounterImage() noexcept
{
    auto &counter = runtimes.counter;
    if (!counter.item || !counter.pcx || !themeReady)
        return;
    counter.pcx->CopyRegion(runtimes.panelBackground.pcx, counter.item->GetX() - x, counter.item->GetY() - y);
    char text[32];
    libc::sprintf(text, NOTIFICATION_COUNTER_FORMAT, currentModInfoIndex, notificationsVisible);
    H3FontLoader font(NH3Dlg::Text::BIG);
    font->TextDraw(counter.pcx, text, 0, 0, counter.pcx->width, counter.pcx->height,
                   eTextColor::REGULAR, eTextAlignment::MIDDLE_CENTER);
}

void NotificationPanel::PrepareTextBackground(ModInfo &info) noexcept
{
    auto *scroll = info.descriptionTextScrollBar;
    auto &body = runtimes.modBackground;
    if (!themeReady || !scroll || !body.item || !body.pcx)
        return;
    // Native scroll callback 0x5BA9B0 restores this canvas before drawing the
    // new lines. Keep a clean themed backing here, never the rendered text.
    if (auto *canvas = scroll->GetPcx())
        canvas->CopyRegion(body.pcx, scroll->GetX() - body.item->GetX(),
                           scroll->GetY() - body.item->GetY());
}

void NotificationPanel::PlaceAtPictureCorner() noexcept
{
    // Resolve against the fitted picture on the first draw. Constructor
    // coordinates can still describe the original 800x600 menu.
    const auto layout = mainmenu::background::GetLayout(parentDlg, mainmenu::background::Enabled());
    const bool custom = layout.custom && layout.assetWidth >= 480 && layout.assetHeight >= 440;
    const int left = custom ? layout.assetX : layout.x;
    const int top = custom ? layout.assetY : layout.y;
    const int pictureWidth = custom ? layout.assetWidth : layout.width;
    const int pictureHeight = custom ? layout.assetHeight : layout.height;
    const int bottom = top + pictureHeight - 32;
    const int newX = left + std::min(20, std::max(0, pictureWidth - width)) - parentDlg->GetX();
    const int newY = std::max(top, bottom - height) - parentDlg->GetY();
    const int dx = newX - x, dy = newY - y;
    std::vector<H3DlgItem *> moved;
    const auto move = [&](H3DlgItem *item) {
        if (item && std::find(moved.begin(), moved.end(), item) == moved.end())
        {
            item->SetX(item->GetX() + dx);
            item->SetY(item->GetY() + dy);
            moved.push_back(item);
        }
    };
    for (auto *item : items)
        move(item);
    for (auto &info : modInfos)
    {
        for (auto *item : info.items)
            move(item);
        info.nameAreaX += dx;
        if (auto *scroll = info.descriptionTextScrollBar)
        {
            move(scroll->GetTextScrollBar());
            for (auto *item : *scroll->GetItems())
                move(item);
        }
    }
    x = newX;
    y = newY;
}

void NotificationPanel::UpdateVisibleNotificationsList() noexcept
{
    notificationsVisible = 0;
    for (auto &i : modInfos)
    {
        i.displayedIndex = !i.isHiddenByUser && i.isVisible ? ++notificationsVisible : 0;
    }
    if (isVisible && notificationsCounter)
    {
        UpdateCounterImage();
    }
    const auto updateButton = [this](H3DlgDefButton *button, const bool isEnable) {
        if (!button)
            return;
        button->SendCommand(5 + isEnable, 4096);
        if (isVisible)
        {
            button->Draw();
            button->Refresh();
        }
    };
    updateButton(previousModButton, isVisible && currentModInfoIndex > 1 &&
                                       currentModInfoIndex <= notificationsVisible);
    updateButton(nextModButton, isVisible && currentModInfoIndex > 0 &&
                                   currentModInfoIndex < notificationsVisible);
}

NotificationPanel::ModInfo *NotificationPanel::GetModInfoFromVisible(const UINT index) noexcept
{
    ModInfo *result = nullptr;
    for (auto &it : modInfos)
    {
        if (!it.isHiddenByUser && it.isVisible && it.displayedIndex == index)
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

        image.pcx->DrawFrame(0, 0, width, height);

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
    modInfo.nameAreaX = x;
    modInfo.nameAreaWidth = width;
    H3FontLoader fn(NH3Dlg::Text::MEDIUM);
    const int modNameWidth = Clamp(1, fn->GetMaxLineWidth(modInfo.displayedName) + 2, width);
    const int nameX = x + (width - modNameWidth) / 2;

    //  if (modInfo.externalLink)
    {
        const H3RGB565 linkColor(H3RGB888(0, 0xAA, 0xFF));
        modInfo.nameUnderline = H3DlgFrame::Create(nameX, y + 24, modNameWidth, 1, linkColor);
        modInfo.items.emplace_back(modInfo.nameUnderline);
    }

    const eTextColor textColor = modInfo.externalLink ? eTextColor::REGULAR : eTextColor::BLUE2;

    modInfo.modNameDlgText =
        H3DlgText::Create(nameX, y + 6, modNameWidth, 20, modInfo.displayedName, NH3Dlg::Text::MEDIUM, textColor);
    modInfo.items.emplace_back(modInfo.modNameDlgText);

    H3RGB565 highLightColor(H3RGB888::Highlight());
    modInfo.delimiterFrame = H3DlgFrame::Create(x + 10, y + 30, width - 16, 1, highLightColor);
    modInfo.items.emplace_back(modInfo.delimiterFrame);

    // Keep both text controls to avoid the game's scrollbar truncating a
    // description after a language change.
    modInfo.descriptionTextScrollBar =
        H3DlgScrollableText::Create(modInfo.displayedText, x + 10, y + 40, width - 16, height - 53,
                                    NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, false);

    // modInfo.descriptionTextScrollBar->SetText(modInfo.displayedText);
    modInfo.items.emplace_back(modInfo.descriptionTextScrollBar);

    modInfo.descriptionText =
        H3DlgText::Create(x + 10, y + 40, width - 16, height - 53, modInfo.displayedText, // dummy text
                          NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, 0, eTextAlignment::VTOP);
    modInfo.descriptionText->HideDeactivate();
    // modInfo.descriptionTextScrollBar->SetText(modInfo.displayedText);
    modInfo.items.emplace_back(modInfo.descriptionText);

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

    for (auto &i : modInfo.items)
    {
        AddItem(i);
    }
}

void NotificationPanel::SetVisible(const BOOL visible, const BOOL activateAllNotifications) noexcept
{
    auto *background = runtimes.panelBackground.item;
    if (!notificationsTotal || !background || !runtimes.panelBackground.pcx || !backupScreen ||
        this->isVisible == visible)
        return;

    if (visible)
    {
        for (auto &info : modInfos)
        {
            if (activateAllNotifications)
                info.isHiddenByUser = false;
            info.isVisible = !info.isHiddenByUser;
        }
        UpdateVisibleNotificationsList();
        if (!notificationsVisible)
            return;
        if (!currentModInfo || currentModInfo->isHiddenByUser || !currentModInfo->isVisible)
            currentModInfo = GetModInfoFromVisible(1);
        if (!currentModInfo)
            return;
        currentModInfoIndex = currentModInfo->displayedIndex;

        // Account for HD Mod's main menu drawing offset.
        const int _x = background->GetAbsoluteX();
        const int _y = background->GetAbsoluteY();
        backupScreen->CopyRegion(P_WindowManager->GetDrawBuffer(), _x, _y);
    }

    this->isVisible = visible;
    background->SetPcx(visible ? runtimes.panelBackground.pcx : backupScreen);
    UpdateVisibleNotificationsList();
    for (auto *item : items)
    {
        if (visible)
        {
            item->ShowActivate();
            item->Draw();
            item->Refresh();
        }
        else
            item->HideDeactivate();
    }
    if (currentModInfo)
        SetModVisible(*currentModInfo, visible);
    if (!visible)
    {
        background->Draw();
        background->Refresh();
    }
}
void NotificationPanel::SetModVisible(ModInfo &modInfo, const BOOL visible) noexcept
{

    if (this->isVisible && visible == false)
    {
        auto &modBackground = this->runtimes.modBackground.item;
        if (modBackground)
        {
            modBackground->Draw();
            modBackground->Refresh();
        }
    }

    if (visible)
    {
        if (auto it = modInfo.modNameDlgText)
        {
            it->ShowActivate();
            it->Draw();
            it->Refresh();
        }

        if (auto it = modInfo.nameUnderline)
        {
            it->ShowActivate();
            it->Draw();
            it->Refresh();
        }

        if (auto it = modInfo.delimiterFrame)
        {
            it->ShowActivate();
            it->Draw();
            it->Refresh();
        }

        BOOL drawSlider = false;

        if (auto scrollableText = modInfo.descriptionTextScrollBar)
        {
            auto slider = scrollableText->GetTextScrollBar();
            drawSlider = slider && slider->GetTicksCount() > 1;

            PrepareTextBackground(modInfo);
        }

        if (drawSlider)
        {
            if (auto scrollableText = modInfo.descriptionTextScrollBar)
            {

                scrollableText->ShowActivate(), scrollableText->Draw(), scrollableText->Refresh();
                if (auto it = scrollableText->GetTextScrollBar())
                {
                    it->ShowActivate(), it->Draw(), it->Refresh();
                }
                for (auto &it : *scrollableText->GetItems())
                {
                    it->ShowActivate(), it->Draw(), it->Refresh();
                }
            }
        }
        else
        {
            if (auto it = modInfo.descriptionText)
            {
                it->ShowActivate();
                it->Draw();
                it->Refresh();
            }
        }
    }
    else
    {

        // hide all items
        if (auto it = modInfo.modNameDlgText)
        {
            it->HideDeactivate();
        }

        if (auto it = modInfo.nameUnderline)
        {
            it->HideDeactivate();
        }

        if (auto it = modInfo.delimiterFrame)
        {
            it->HideDeactivate();
        }

        if (auto it = modInfo.descriptionText)
        {
            it->HideDeactivate();
        }

        if (auto scrollableText = modInfo.descriptionTextScrollBar)
        {

            scrollableText->HideDeactivate();
            if (auto it = scrollableText->GetTextScrollBar())
            {
                it->HideDeactivate();
            }
            for (auto &it : *scrollableText->GetItems())
            {
                it->HideDeactivate();
            }
        }
    }
}

void NotificationPanel::Retranslate(const BOOL redraw) noexcept
{
    bool readSuccess = false;
    LPCSTR str = mainmenu::config::Text(panelText::HIDE_ALL, "Hide all");
    if (hideAllButton)
    {
        hideAllButton->SetText(str);
        if (redraw)
        {
            hideAllButton->Draw();
            hideAllButton->Refresh();
        }
    }
    str = mainmenu::config::Text(panelText::HIDE_ONE, "Hide");
    if (hideOneButton)
    {
        hideOneButton->SetText(str);
        if (redraw)
        {
            hideOneButton->Draw();
            hideOneButton->Refresh();
        }
    }
    str = mainmenu::config::Text(panelText::TITLE, "Notifications");
    if (panelTitle)
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
            if (assumedModInfo && currentModInfo != assumedModInfo)
            {
                if (currentModInfo)
                {
                    SetModVisible(*currentModInfo, false);
                }

                currentModInfo = assumedModInfo;
                currentModInfoIndex = currentModInfo->displayedIndex;
                SetModVisible(*currentModInfo, true);

                UpdateVisibleNotificationsList();
                if (notificationsCounter)
                {
                    notificationsCounter->Draw();
                    notificationsCounter->Refresh();
                }
            }
        }
    }
}

int __fastcall NotificationPanel::OnPanelCallerClick(void *msg) noexcept
{
    auto *h3msg = reinterpret_cast<H3Msg *>(msg);
    if (h3msg && h3msg->IsLeftClick() && instance && instance->notificationsTotal)
        instance->SetVisible(!instance->isVisible, true);
    return true;
}

bool NotificationPanel::IsLinkHovered(H3Msg *msg) const noexcept
{
    return msg && msg->GetDlg() == parentDlg && isVisible && currentModInfo &&
           currentModInfo->isVisible && currentModInfo->externalLink && currentModInfo->modNameDlgText &&
           currentModInfo->modNameDlgText->IsVisible() && currentModInfo->modNameDlgText->IsActive() &&
           currentModInfo->modNameDlgText == msg->ItemAtPosition(parentDlg);
}

BOOL NotificationPanel::ProcessPanel(H3Msg *msg, const BOOL forceRedraw) noexcept
{
    if (!isVisible || !notificationsTotal)
        return false;

    BOOL result = false;
    if (msg)
    {

        if (msg->IsLeftClick())
        {
            const int itemId = msg->itemId;
            int modsWrittenToIni = 0;

            switch (msg->itemId)
            {
            case PREVIOUS_BUTTON_ID:
                SwitchModInfo(-1);
                break;
            case NEXT_BUTTON_ID:
                SwitchModInfo(1);
                break;
            case HIDE_ONE_BUTTON_ID:
                if (!currentModInfo)
                    break;
                // first, mark current mod as hidden
                if (currentModInfo->MarkAsHiddenByUser())
                {
                    Era::SaveIni(INI_FILE_NAME);
                }

                SetModVisible(*currentModInfo, false);
                UpdateVisibleNotificationsList();
                if (notificationsVisible)
                {
                    currentModInfoIndex = std::min(currentModInfoIndex, notificationsVisible);
                    currentModInfo = GetModInfoFromVisible(currentModInfoIndex);
                    if (currentModInfo)
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
                    SetVisible(false);
                }
                break;
            case HIDE_ALL_BUTTON_ID:
                SetVisible(false);
                // if mod has text, mark it as hidden
                for (auto &i : modInfos)
                {
                    modsWrittenToIni += i.MarkAsHiddenByUser();
                }
                if (modsWrittenToIni)
                {
                    Era::SaveIni(INI_FILE_NAME);
                }
                UpdateVisibleNotificationsList();
                currentModInfo = nullptr;
                currentModInfoIndex = 0;
                break;

            default:
                break;
            }
        }
        else
        {

            // set cursor to hand
            result = IsLinkHovered(msg);

            if (result && msg->IsLeftDown())
            {
                P_SoundManager->ClickSound();
                mainmenu::OpenNotificationLink(currentModInfo->externalLink);
            }
        }

        if (isVisible && forceRedraw)
        {

            for (auto &i : items)
            {
                i->Draw();
            }

            if (currentModInfo && currentModInfo->isVisible)
            {
                for (auto &i : currentModInfo->items)
                {
                    if (i->IsVisible())
                        i->Draw();
                }

                if (auto scroll = currentModInfo->descriptionTextScrollBar)
                {
                    if (scroll->IsVisible())
                    {
                        if (auto _items = scroll->GetItems())
                        {
                            for (auto &i : *_items)
                            {
                                i->Draw();
                            }
                        }

                        // manage scrollbar if it has more than one tick (visible)
                        if (auto _i = scroll->GetTextScrollBar())
                        {
                            if (_i->GetTicksCount() > 1)
                            {
                                _i->Draw();
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
    themeReady = false;
    if (!notificationsTotal)
        return;

    if (parentCaller)
    {
        libc::sprintf(h3_TextBuffer, mainmenu::config::Text(panelText::BUTTON_FORMAT, "Notifications (%d)"), notificationsTotal);
        parentCaller->SetText(h3_TextBuffer);
        MenuWidgetManager::Get().SetWidgetText(NotificationPanel::PARENT_BUTTON_CALLER_NAME, h3_TextBuffer);
    }

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
            if (i.descriptionTextScrollBar)
                i.descriptionTextScrollBar->SetText(i.displayedText);
            if (i.descriptionText)
                i.descriptionText->SetText(i.displayedText);
            if (i.modNameDlgText)
                i.modNameDlgText->SetText(i.displayedName);
            SetModVisible(i, false);
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
    auto layout = mainmenu::background::GetLayout(parrent, mainmenu::background::Enabled());
    if (layout.custom && (layout.assetWidth < 480 || layout.assetHeight < 440))
        layout = mainmenu::background::GetLayout(parrent, false);
    const int pictureX = layout.custom ? layout.assetX : layout.x;
    const int pictureY = layout.custom ? layout.assetY : layout.y;
    const int pictureWidth = layout.custom ? layout.assetWidth : layout.width;
    const int pictureHeight = layout.custom ? layout.assetHeight : layout.height;
    const int panelWidth = std::min(width, std::max(200, pictureWidth - 40));
    const int panelHeight = std::min(height, std::max(160, pictureHeight - 64));
    const int panelX = pictureX + std::min(20, std::max(0, pictureWidth - panelWidth)) - parrent->GetX();
    const int panelY = pictureY + pictureHeight - 32 - panelHeight - parrent->GetY();
    instance = new NotificationPanel(parrent, panelX, panelY, panelWidth, panelHeight);
    return instance;
}
