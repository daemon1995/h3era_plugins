#include <chrono>

#include "framework.h"
#include "VersionUtils.h"
#include "../headers/EraPluginsAPI/NotificationLinks.hpp"
#include "../Interface_MainMenuAPI/MainMenuConfig.h"
// #include "webFunctions.cpp"

#include "Shlwapi.h"

#pragma comment(lib, "Shlwapi.lib")

namespace web
{
std::string PerformWinHTTPRequest(const wchar_t *api, const wchar_t *host, const wchar_t *path);
}
// constexpr const char* BASE_JSON_KEY = "gem_plugin.main_menu";

AssemblyInformation *AssemblyInformation::instance = nullptr;
// @todo // add red color for different versions
AssemblyInformation::AssemblyInformation(PatcherInstance *_pi)
    : IGamePatch(_pi), versions{&m_eraVersion, &m_localVersion, &m_remoteVersion}
{
    LoadDataFromJson();
}
void AssemblyInformation::Version::GetVersion() noexcept
{
    //	version = Era::GetEraVersion();
}

void AssemblyInformation::Version::GetJsonData(const char *jsonSubKey)
{
    bool readSuccess = false;

    sprintf(h3_TextBuffer, "%s.%s.display_version", BASE_JSON_KEY, jsonSubKey);
    show = EraJS::readInt(h3_TextBuffer, readSuccess);

    if (show && readSuccess)
    {
        sprintf(h3_TextBuffer, "%s.%s.x", BASE_JSON_KEY, jsonSubKey);
        x = EraJS::readInt(h3_TextBuffer);

        sprintf(h3_TextBuffer, "%s.%s.y", BASE_JSON_KEY, jsonSubKey);
        y = EraJS::readInt(h3_TextBuffer);

        sprintf(h3_TextBuffer, "%s.%s.custom_text", BASE_JSON_KEY, jsonSubKey);
        text = EraJS::read(h3_TextBuffer, readSuccess);

        customText = readSuccess && !text.Empty();

        sprintf(h3_TextBuffer, "%s.%s.format", BASE_JSON_KEY, jsonSubKey);
        format = EraJS::read(h3_TextBuffer);

        alwaysDraw = mainmenu::config::Enabled("era.main_menu.rendering.redraw_always", false);

        Era::ReadStrFromIni("ERA", "ShellExecute", ASSEMBLY_INI_FILE, h3_TextBuffer);
        shellExecutePath = h3_TextBuffer;
    }
}

H3DlgPcx16 *AssemblyInformation::Version::AddToDlg(H3BaseDlg *dlg) noexcept
{
    parentDlg = dlg;
    dlgItem = H3DlgPcx16::Create(0, 0, 1, 1, -1, nullptr);
    if (!dlgItem || !UpdateImage())
    {
        if (dlgItem)
            dlgItem->Destroy(TRUE);
        dlgItem = nullptr;
        parentDlg = nullptr;
        return nullptr;
    }
    dlg->AddItem(dlgItem);
    return dlgItem;
}

bool AssemblyInformation::Version::UpdateImage(const bool drawText) noexcept
{
    if (!parentDlg || !dlgItem)
        return false;
    H3FontLoader font(NH3Dlg::Text::SMALL);
    constexpr int horizontalPadding = 5;
    constexpr int verticalPadding = 3;
    const int maxWidth = parentDlg->GetWidth();
    const int maxHeight = parentDlg->GetHeight();
    if (!font.Get() || maxWidth <= horizontalPadding * 2 || maxHeight <= verticalPadding * 2)
        return false;

    // Keep the right edge and never shrink the backing image: an asynchronous
    // version update must paint over the entire previous label.
    const int textWidth = Clamp(1, font->GetMaxLineWidth(text.String()), maxWidth - horizontalPadding * 2);
    const int width = std::max(textWidth + horizontalPadding * 2, labelPcx ? labelPcx->width : 0);
    const int height = std::min(font->height + verticalPadding * 2, maxHeight);
    const int right = labelPcx ? dlgItem->GetX() + dlgItem->GetWidth()
                              : Clamp(0, x, maxWidth - width) + width;
    const int top = labelPcx ? dlgItem->GetY() : y;
    if (!labelPcx || labelPcx->width != width || labelPcx->height != height)
    {
        auto *replacement = H3LoadedPcx16::Create(width, height);
        if (!replacement)
            return false;
        dlgItem->SetPcx(nullptr);
        if (labelPcx)
            labelPcx->Destroy();
        labelPcx = replacement;
        dlgItem->SetPcx(labelPcx);
    }
    dlgItem->SetWidth(width);
    dlgItem->SetHeight(height);
    dlgItem->SetX(Clamp(0, right - width, maxWidth - width));
    dlgItem->SetY(Clamp(0, top, maxHeight - height));

    // Same runtime text-on-PCX approach as the language selection dialog.
    libc::memset(labelPcx->buffer, 0, labelPcx->buffSize);
    labelPcx->DrawFrame(0, 0, width, height); // Thin, one-pixel frame.
    if (drawText)
        font->TextDraw(labelPcx, text.String(), horizontalPadding, verticalPadding,
                       width - horizontalPadding * 2, height - verticalPadding * 2,
                       eTextColor::REGULAR, eTextAlignment::MIDDLE_CENTER);
    return true;
}

void AssemblyInformation::Version::ReleaseImage() noexcept
{
    // Dialog items belong to the game; detach our image before their destructor.
    if (dlgItem)
        dlgItem->SetPcx(nullptr);
    if (labelPcx)
        labelPcx->Destroy();
    labelPcx = nullptr;
    dlgItem = nullptr;
    parentDlg = nullptr;
}

void AssemblyInformation::Version::AdjustItemText() noexcept
{
    if (!customText)
    {
        sprintf(h3_TextBuffer, format, version.String());
        text = h3_TextBuffer;
    }
}

std::string GetAppForExtension(const std::string &extension)
{
    // Проверяем, что расширение начинается с точки
    std::string extWithDot = (extension[0] == '.') ? extension : "." + extension;

    DWORD bufferSize = 0;
    // Первый вызов нужен, чтобы узнать необходимый размер буфера
    HRESULT hr =
        AssocQueryStringA(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_EXECUTABLE, extWithDot.c_str(), NULL, NULL, &bufferSize);

    if (hr == S_FALSE && bufferSize > 0)
    {
        std::string appPath(bufferSize, '\0');
        // Второй вызов для получения самого пути
        hr = AssocQueryStringA(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_EXECUTABLE, extWithDot.c_str(), NULL, &appPath[0],
                               &bufferSize);
        if (hr == S_OK)
        {
            appPath.resize(bufferSize - 1);
            return appPath;
        }
    }
    // Если программа не найдена, возвращаем пустую строку
    return h3_NullString;
}

std::string LPCSTR_to_wstring(LPCSTR ansi_str)
{
    // 1. Получаем длину без нуль-терминатора
    const int src_len = libc::strlen(ansi_str);

    // 2. ANSI → UTF-16
    const int wlen = libc::MultiByteToWideChar(CP_ACP, 0, ansi_str, src_len, NULL, 0);
    std::wstring wstr(wlen, 0);
    libc::MultiByteToWideChar(CP_ACP, 0, ansi_str, src_len, &wstr[0], wlen);

    // 3. UTF-16 → UTF-8
    const int ulen = libc::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    std::string utf8_str(ulen, 0);
    libc::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8_str[0], ulen, NULL, NULL);

    return utf8_str;
}

std::wstring GetFriendlyAppNameForExtension(const std::wstring &extension)
{
    std::wstring extWithDot = (extension[0] == L'.') ? extension : L"." + extension;
    DWORD bufferSize = 0;

    // Узнаём необходимый размер буфера
    HRESULT hr = AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_FRIENDLYAPPNAME, extWithDot.c_str(), NULL, NULL,
                                   &bufferSize);
    if (hr == S_FALSE && bufferSize > 0)
    {
        std::wstring appName(bufferSize, L'\0');
        // Получаем имя
        hr = AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_FRIENDLYAPPNAME, extWithDot.c_str(), NULL,
                               &appName[0], &bufferSize);
        if (hr == S_OK)
        {
            appName.resize(bufferSize - 1);
            return appName;
        }
    }
    return L"";
}
std::string GetFriendlyAppNameForExtensionA(const std::string &extension)
{
    std::string extWithDot = (extension[0] == '.') ? extension : "." + extension;
    DWORD bufferSize = 0;

    // Узнаём необходимый размер буфера
    HRESULT hr = AssocQueryStringA(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_FRIENDLYAPPNAME, extWithDot.c_str(), NULL, NULL,
                                   &bufferSize);
    if (hr == S_FALSE && bufferSize > 0)
    {
        std::string appName(bufferSize, '\0');
        // Получаем имя
        hr = AssocQueryStringA(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_FRIENDLYAPPNAME, extWithDot.c_str(), NULL,
                               &appName[0], &bufferSize);
        if (hr == S_OK)
        {
            appName.resize(bufferSize - 1);
            return appName;
        }
    }
    return h3_NullString;
}
void AssemblyInformation::Version::ClickProcedure() const noexcept
{
    if (!shellExecutePath.Empty())
    {
        P_SoundManager->ClickSound();
        mainmenu::OpenNotificationLink(shellExecutePath.String());
    }
}

void AssemblyInformation::LocalVersion::AdjustItemText() noexcept
{
    Version::AdjustItemText();
}

void AssemblyInformation::RemoteVersion::GetJsonData(const char *jsonSubKey)
{
    ApplyCompletedRequest();

    Version::GetJsonData(jsonSubKey);
    Era::ReadStrFromIni("Remote", "ShellExecute", ASSEMBLY_INI_FILE, h3_TextBuffer);
    shellExecutePath = h3_TextBuffer;

    if (show && !customText && !request.valid())
    {
        constexpr DWORD FIFTEEN_MINUTES_MS = 15 * 60 * 1000;

        // ��� ������ ������� ������ ����� �� INI

        if (Era::ReadStrFromIni(LAST_TIME_CHECKED_INI_KEY, ASSEMBLY_SETTINGS_SECTION, ASSEMBLY_SETTINGS_INI,
                                h3_TextBuffer))
        {
            DWORD lastCheckTime = strtoul(h3_TextBuffer, nullptr, 10);
            if (Era::ReadStrFromIni(LAST_VERSION_INI_KEY, ASSEMBLY_SETTINGS_SECTION, ASSEMBLY_SETTINGS_INI,
                                    h3_TextBuffer))
            {
                H3String cachedVersion = h3_TextBuffer;
                if (lastCheckTime != 0 && (h3::GetTime() - lastCheckTime) < FIFTEEN_MINUTES_MS &&
                    !cachedVersion.Empty())
                {
                    version = cachedVersion;
                    workDone = true;
                    return;
                }
            }
        }

        GetVersion();
    }
}

void AssemblyInformation::RemoteVersion::GetVersion() noexcept
{
    if (request.valid())
        return;
    try
    {
        // ERA and H3 data stay on the game thread. The worker owns its input and result.
        const auto readSetting = [](const char *key) {
            h3_TextBuffer[0] = '\0';
            if (!Era::ReadStrFromIni(key, "GitHub", ASSEMBLY_INI_FILE, h3_TextBuffer))
                return std::wstring{};
            const std::string value = h3_TextBuffer;
            return std::wstring(value.begin(), value.end());
        };
        const std::wstring api = readSetting("API");
        const std::wstring host = readSetting("Host");
        const std::wstring path = readSetting("Path");
        if (api.empty() || host.empty() || path.empty())
            return;
        request = std::async(std::launch::async, [api, host, path]() -> std::string {
            try
            {
                const auto response = web::PerformWinHTTPRequest(api.c_str(), host.c_str(), path.c_str());
                const auto json = nlohmann::json::parse(response, nullptr, false);
                if (!json.is_object())
                    return {};
                const auto tag = json.find("tag_name");
                if (tag == json.end() || !tag->is_string())
                    return {};
                const auto value = tag->get<std::string>();
                assemblyVersion::ParsedVersion parsed;
                return assemblyVersion::Parse(value, parsed) ? value : std::string{};
            }
            catch (...)
            {
                return {};
            }
        });
    }
    catch (...)
    {
        // A failed optional update check must not stop the game.
    }
}

void AssemblyInformation::RemoteVersion::ApplyCompletedRequest()
{
    if (!request.valid() || request.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        return;
    std::string receivedVersion;
    try
    {
        receivedVersion = request.get();
    }
    catch (...)
    {
        return;
    }
    // Keep the last successful version and its cache when the network check fails.
    if (!receivedVersion.empty())
    {
        version = receivedVersion.c_str();
        workDone = true;
        sprintf(h3_TextBuffer, "%lu", h3::GetTime());
        Era::WriteStrToIni(LAST_TIME_CHECKED_INI_KEY, h3_TextBuffer, ASSEMBLY_SETTINGS_SECTION,
                           ASSEMBLY_SETTINGS_INI);
        Era::WriteStrToIni(LAST_VERSION_INI_KEY, version.String(), ASSEMBLY_SETTINGS_SECTION,
                           ASSEMBLY_SETTINGS_INI);
        Era::SaveIni(ASSEMBLY_SETTINGS_INI);
    }
}

void AssemblyInformation::LocalVersion::GetJsonData(const char *jsonSubKey)
{
    Version::GetJsonData(jsonSubKey);
    bool readSuccess = false;

    sprintf(h3_TextBuffer, "%s.%s.custom_version", BASE_JSON_KEY, jsonSubKey);
    version = EraJS::read(h3_TextBuffer, readSuccess);
    if (readSuccess && !version.Empty())
        customVersion = true;

    sprintf(h3_TextBuffer, "%s.%s.read_registry", BASE_JSON_KEY, jsonSubKey);
    readRegistry = EraJS::readInt(h3_TextBuffer);
    if (!customText)
        GetVersion();

    Era::ReadStrFromIni("Assembly", "ShellExecute", ASSEMBLY_INI_FILE, h3_TextBuffer);
    shellExecutePath = h3_TextBuffer;
}

EXTERN_C __declspec(dllexport) void GetAssemblyGameVersion(char *buffer)
{
    sprintf(buffer, "%s", AssemblyInformation::GetAssemblyVesrion());
}

const char *AssemblyInformation::GetAssemblyVesrion()
{
    return AssemblyInformation::Get().m_localVersion.version.String();
}

void AssemblyInformation::CheckOnlineVersion()
{
    m_remoteVersion.GetJsonData("online_version");
}

const BOOL AssemblyInformation::CompareVersions()
{
    m_remoteVersion.ApplyCompletedRequest();
    const BOOL wasHigher = m_localVersion.remoteVersionIsHigher;
    m_localVersion.remoteVersionIsHigher =
        assemblyVersion::IsNewer(m_remoteVersion.version.String(), m_localVersion.version.String());

    if (m_localVersion.remoteVersionIsHigher && !wasHigher)
    {
        versionBlinkLastUpdate = h3::GetTime();
        versionBlinkVisible = true;
        if (isVisible && m_localVersion.dlgItem)
        {
            m_localVersion.UpdateImage();
            m_localVersion.dlgItem->Show();
            m_localVersion.dlgItem->Draw();
            m_localVersion.dlgItem->Refresh();
        }
    }
    else if (wasHigher && !m_localVersion.remoteVersionIsHigher && isVisible && m_localVersion.dlgItem)
    {
        // Do not leave the installed version hidden when blinking stops.
        versionBlinkVisible = true;
        m_localVersion.UpdateImage();
        m_localVersion.dlgItem->Show();
        m_localVersion.dlgItem->Draw();
        m_localVersion.dlgItem->Refresh();
    }

    return m_localVersion.remoteVersionIsHigher;
}

void AssemblyInformation::LocalVersion::GetVersion() noexcept
{
    if (!customVersion)
    {
        Era::ReadStrFromIni("key", "Registry", ASSEMBLY_INI_FILE, h3_TextBuffer);
        if (ReadRegistry(h3_TextBuffer))
        {
        }

        if (Era::ReadStrFromIni("Version", "Assembly", "ERA_Project.ini", h3_TextBuffer))
            version = h3_TextBuffer;
    }
}

BOOL AssemblyInformation::LocalVersion::ReadRegistry(const char *registryKey)
{
    HKEY hKey;

    LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, registryKey, 0, KEY_READ, &hKey);
    bool bExistsAndSuccess(lRes == ERROR_SUCCESS);

    if (bExistsAndSuccess)
    {
        CHAR szBuffer[64];
        DWORD dwBufferSize = sizeof(szBuffer);
        ULONG nError;
        nError = RegQueryValueExA(hKey, "version", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
        if (ERROR_SUCCESS == nError)
            version = szBuffer;
        RegCloseKey(hKey);
        bExistsAndSuccess = !version.Empty();
    }

    return bExistsAndSuccess;
}

int __stdcall AssemblyInformation::DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg)
{
    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    Get().CreateDlgItems(dlg);
    return result;
}
int __stdcall AssemblyInformation::DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg)

{
    Get().isVisible = false;
    for (auto *version : Get().versions)
        version->ReleaseImage();
    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}
// int __stdcall AssemblyInformation::DlgMainMenu_NewLoad_Create(HiHook *h,
// H3BaseDlg *dlg, const int val)
//{
//     int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, val);
//     Get().CreateDlgItems(dlg);
//     return result;
// }
// int __stdcall AssemblyInformation::DlgMainMenu_Campaign_Run(HiHook *h,
// H3BaseDlg *dlg)
//{
//     Get().CreateDlgItems(dlg);
//     auto _h = _PI->WriteHiHook(0x5FFAC0, THISCALL_, DlgMainMenu_Proc); //
//     Main Main Menu Dlg Proc int result = THISCALL_1(int, h->GetDefaultFunc(),
//     dlg); _h->Destroy(); return result;
// }
void AssemblyInformation::CreateDlgItems(H3BaseDlg *dlg)
{
    CompareVersions();
    // hide wnd version hint
    if (auto *it = dlg->GetH3DlgItem(545))
        it->Hide();

    for (const auto version : versions)
    {
        if (version->show)
        {
            version->AdjustItemText();
            version->dlgItem = version->AddToDlg(dlg);
            isVisible |= version->dlgItem != nullptr;
        }
    }
    AlignVersionLabels();
}

void AssemblyInformation::AlignVersionLabels() noexcept
{
    auto *installed = m_localVersion.dlgItem;
    auto *online = m_remoteVersion.dlgItem;
    if (!installed || !online)
        return;

    // Keep the configured placement, with a common right edge and a gap
    // between the installed/online labels, including their frames.
    const int right = std::max(installed->GetX() + installed->GetWidth(),
                               online->GetX() + online->GetWidth());
    installed->SetX(right - installed->GetWidth());
    online->SetX(right - online->GetWidth());
    constexpr int gap = 2;
    if (installed->GetY() + installed->GetHeight() + gap > online->GetY())
    {
        const int top = std::max(0, online->GetY() - installed->GetHeight() - gap);
        installed->SetY(top);
        online->SetY(std::min(top + installed->GetHeight() + gap,
                             m_remoteVersion.parentDlg->GetHeight() - online->GetHeight()));
    }
}

int __stdcall AssemblyInformation::DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{
    auto &instance = Get();
    BOOL setCustomCursor = false;

    if (instance.isVisible)
    {
        const AssemblyInformation::Version *activeVersion = nullptr;
        for (auto *ver : instance.versions)
        {
            H3DlgPcx16 *it = ver->dlgItem;
            if (it && it->IsVisible())
            {
                if (it == msg->ItemAtPosition(msg->GetDlg()))
                {
                    activeVersion = ver;
                }

                if (ver->alwaysDraw)
                {
                    it->Draw();
                }
            }
        }

        if (activeVersion)
        {
            setCustomCursor = true;

            if (msg->IsLeftDown())
            {
                activeVersion->ClickProcedure();
            }
        }

        AssemblyInformation::RemoteVersion &remoteVersion = instance.m_remoteVersion;

        H3DlgPcx16 *it = remoteVersion.dlgItem;

        remoteVersion.ApplyCompletedRequest();
        if (remoteVersion.workDone && it)
        {
            instance.CompareVersions();
            if (it->IsVisible())
            {
                remoteVersion.AdjustItemText();
                remoteVersion.UpdateImage();
                instance.AlignVersionLabels();
                it->Draw();
                it->Refresh();
            }

            remoteVersion.workDone = false;
        }

        instance.UpdateVersionBlink();
    }

    if (setCustomCursor)
    {
        P_MouseManager->SetCursor(3, 0);
    }
    else if (msg->GetX()) // check if mouse isn't reset
    {
        P_MouseManager->DefaultCursor();
    }

    return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}

void AssemblyInformation::UpdateVersionBlink() noexcept
{
    if (!m_localVersion.remoteVersionIsHigher || !m_localVersion.dlgItem)
        return;

    const DWORD currentTime = h3::GetTime();
    constexpr DWORD BLINK_INTERVAL_MS = 500;
    if (currentTime - versionBlinkLastUpdate < BLINK_INTERVAL_MS)
        return;

    versionBlinkLastUpdate = currentTime;
    versionBlinkVisible = !versionBlinkVisible;
    // Blink only the text. Drawing the black backing image erases the old
    // glyphs while the frame, click target and item visibility stay stable.
    m_localVersion.UpdateImage(versionBlinkVisible != FALSE);
    m_localVersion.dlgItem->Draw();
    m_localVersion.dlgItem->Refresh();
}

H3DlgText *__stdcall H3DlgText__Ctor(HiHook *h, H3DlgText *_this, int xpos, int ypos, int xsize, int ysize,
                                     const char *text, const char *font, int color, int itemId, int align, int bkcolor,
                                     int unused)
{
    // bkcolor = 1; // rand() % 255;
    // text =
    // libc::sprintf(h3_TextBuffer, "%d", bkcolor);
    // text = h3_TextBuffer;
    return THISCALL_12(H3DlgText *, h->GetDefaultFunc(), _this, xpos, ypos, xsize, ysize, text, font, color, itemId,
                       align, bkcolor, unused);
}

_LHF_(gem_Text)
{
    if (auto dlgText = reinterpret_cast<H3DlgText *>(c->edi))
    {
        if (!libc::strcmpi(reinterpret_cast<LPCSTR>(c->eax), h3_NullString))
        {
            dlgText->bkColor = 1;
        }
        // const int lineId = c->ebx >> 2;
        // if (c->ebx ==128)
        //{
        //     dlgText->bkColor = 1;

        //}
        else
        {
            dlgText->bkColor = 0;
        }
    }
    // c->eax = 123;
    return EXEC_DEFAULT;
}

void AssemblyInformation::CreatePatches() noexcept
{
    if (!m_isEnabled)
    {
        _PI->WriteHiHook(0x04EF00C, THISCALL_, DlgMainMenu_Create);
        _PI->WriteHiHook(0x04EF247, THISCALL_, DlgMainMenu_Create);

        _PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc);

        _PI->WriteHiHook(0x4EF02B, THISCALL_, DlgMainMenu_Dtor); // MAIN menu
        _PI->WriteHiHook(0x04EF267, THISCALL_, DlgMainMenu_Dtor); // MAIN menu

        if (0)
        {
        }
        // _PI->WriteHiHook(0x05BA547, THISCALL_, H3DlgText__Ctor);
        // _PI->WriteHiHook(0x044E190, THISCALL_, H3DlgText__Draw);
        // _PI->WriteLoHook(0x05BAA35, gem_Text);
        //  _PI->WriteHiHook(0x4D56D0, THISCALL_, DlgMainMenu_NewLoad_Create);
        // _PI->WriteHiHook(0x4F0799, THISCALL_, DlgMainMenu_Campaign_Run); // goes
        // from new game

        // _PI->WriteHiHook(0x04D5AA0, THISCALL_, DlgMainMenu_Dtor); // New/Load
        // Game

        // _PI->WriteHiHook(0x4D5B50, THISCALL_, DlgMainMenu_Proc); // Main Main
        // Menu Dlg Proc


        m_isEnabled = true;
    }
}

AssemblyInformation &AssemblyInformation::Get()
{
    if (!instance)
    {
        instance = new AssemblyInformation(_PI);
    }
    return *instance;

    // TODO: insert return statement here
}

void AssemblyInformation::LoadDataFromJson()
{
    if (!m_isInited)
    {
        m_eraVersion.GetJsonData("era_version");
        m_localVersion.GetJsonData("current_version");
        m_remoteVersion.GetJsonData("online_version");
        m_eraVersion.version = Era::GetEraVersion();
        if (m_eraVersion.show || m_localVersion.show || m_remoteVersion.show)
            CreatePatches();
        m_isInited = true;
    }
}

void AssemblyInformation::PluginText::Load()
{
}
