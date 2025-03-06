#include "framework.h"
#include <thread>
// #include "webFunctions.cpp"

#include "Shlwapi.h"

#pragma comment(lib, "Shlwapi.lib")

std::string PerformWinHTTPRequest(const wchar_t *api, const wchar_t *host, const wchar_t *path);

// constexpr const char* BASE_JSON_KEY = "gem_plugin.main_menu";

const char *AssemblyInformation::BASE_JSON_KEY = "gem_plugin.main_menu";
const char *AssemblyInformation::ASSEMBLY_INI_FILE = "ERA_Project.ini";

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

        sprintf(h3_TextBuffer, "%s.%s.font", BASE_JSON_KEY, jsonSubKey);
        LPCSTR fName = EraJS::read(h3_TextBuffer, readSuccess);
        fontName = readSuccess ? fName : h3::NH3Dlg::Text::MEDIUM;
        sprintf(h3_TextBuffer, "%s.%s.length_in_px", BASE_JSON_KEY, jsonSubKey);
        UINT lenFromJson = EraJS::readInt(h3_TextBuffer, readSuccess);
        if (readSuccess && lenFromJson > 0)
            characterLength = lenFromJson;

        alwaysDraw = EraJS::readInt("nmmi.menu_info.main.always_draw");

        Era::ReadStrFromIni("ERA", "ShellExecute", ASSEMBLY_INI_FILE, h3_TextBuffer);
        shellExecutePath = h3_TextBuffer;
    }
}

H3DlgText *AssemblyInformation::Version::AddToDlg(H3BaseDlg *dlg) noexcept
{

    H3FontLoader fnt(this->fontName);                          // = H3Font::Load(h3::NH3Dlg::Text::MEDIUM);
    const int itemWidth = fnt->GetMaxLineWidth(text.String()); // text.Length()* characterLength;
    if (800 - x < itemWidth)
        x = 800 - itemWidth;
    if (600 - y < 16)
        y = 600 - 16;
    auto it =
        dlg->CreateText(x, y, itemWidth, fnt->height, text.String(), fnt->GetName(), 7, -1, eTextAlignment::HLEFT);

    // fnt->Dereference();
    // if (versions)
    //	*reinterpret_cast<int*>(*reinterpret_cast<int*>(versions) +0x44) = eTextColor::RED;
    return it;
}

void AssemblyInformation::Version::AdjustItemText() noexcept
{
    if (!customText)
    {
        sprintf(h3_TextBuffer, format, version.String());
        text = h3_TextBuffer;
    }
}
void AssemblyInformation::Version::ClickProcedure() const noexcept
{

    if (shellExecutePath.Empty())
    {
    }
    else
    {
        P_SoundManager->ClickSound();

        // check if we open file or the link
        bool isUrl = PathIsURL(shellExecutePath.String());

        LPCSTR jsonKey =
            isUrl ? "gem_plugin.user_notification.open_web_page" : "gem_plugin.user_notification.open_external_file";
        libc::sprintf(h3_TextBuffer, EraJS::read(jsonKey), shellExecutePath.String());
        BOOL isOk = H3Messagebox::Choice(h3_TextBuffer);
        if (isOk)
        {
            // call ShellExecuteA from exe to open github download page
            INT_PTR intRes =
                STDCALL_6(INT_PTR, PtrAt(0x63A250), NULL, "open", shellExecutePath.String(), NULL, NULL, SW_SHOWNORMAL);

            /** https://learn.microsoft.com/ru-ru/windows/win32/api/shellapi/nf-shellapi-shellexecutea */
            if (intRes <= 32)
            {

                LPSTR messageBuffer = nullptr;

                // Форматирование сообщения об ошибке
                size_t size = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, intRes, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

                std::string message(messageBuffer, size);
                H3Messagebox(message.c_str());

                // Освобождение буфера
                LocalFree(messageBuffer);
            }
        }
    }
}

void AssemblyInformation::LocalVersion::AdjustItemText() noexcept
{
    Version::AdjustItemText();
    if (!customText && remoteVersionIsHigher)
    {
    }
}

void AssemblyInformation::RemoteVersion::GetJsonData(const char *jsonSubKey)
{
    Version::GetJsonData(jsonSubKey);
    Era::ReadStrFromIni("Remote", "ShellExecute", ASSEMBLY_INI_FILE, h3_TextBuffer);
    shellExecutePath = h3_TextBuffer;

    if (show && !customText && !workDone.load())
    {
        workDone = false;
        std::thread th(&AssemblyInformation::RemoteVersion::GetVersion, this);
        th.detach();
    }
}

void AssemblyInformation::RemoteVersion::GetVersion() noexcept
{
    // reset buffer
    sprintf(h3_TextBuffer, "%s", "");

    // create WideStringsPtr
    constexpr LPCSTR sectionName = "GitHub";
    Era::ReadStrFromIni("API", sectionName, ASSEMBLY_INI_FILE, h3_TextBuffer);
    std::string narrowString = h3_TextBuffer;
    std::wstring api(narrowString.begin(), narrowString.end());

    Era::ReadStrFromIni("Host", sectionName, ASSEMBLY_INI_FILE, h3_TextBuffer);
    narrowString = h3_TextBuffer;
    std::wstring host(narrowString.begin(), narrowString.end());

    Era::ReadStrFromIni("Path", sectionName, ASSEMBLY_INI_FILE, h3_TextBuffer);
    narrowString = h3_TextBuffer;
    std::wstring path(narrowString.begin(), narrowString.end());

    version = "";
    if (!api.empty() && !host.empty() && !path.empty())
    {
        // make an HTTP request (thanks to Chat GPT)
        std::string requestResponce = PerformWinHTTPRequest(api.c_str(), host.c_str(), path.c_str());
        if (!requestResponce.empty())
        {
            // parse json (thanks to nlohmann)
            nlohmann::json j = nlohmann::json::parse(requestResponce, nullptr, false);
            // get object and check if versions is correct
            auto &obj = j["tag_name"];
            if (!obj.is_null() && obj.is_string())
                version = obj.get<std::string>().c_str();
        }
    }
    workDone.store(true);
}
// void AssemblyInformation::RemoteVersion::ClickProcedure() noexcept
//{
//
//
// }
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
    sprintf(buffer, AssemblyInformation::GetAssemblyVesrion());
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
    // if there is process run then start to compare locale and remote
    if (m_remoteVersion.workDone.load())
    {

        m_remoteVersion.version.ToDouble();
        m_remoteVersion.workDone = false;

        return m_remoteVersion.version.ToDouble() > m_localVersion.version.ToDouble();
    }

    return false;
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
int __stdcall AssemblyInformation::DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val)
{
    int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, val);
    Get().CreateDlgItems(dlg);
    return result;
}
int __stdcall AssemblyInformation::DlgMainMenu_Campaign_Run(HiHook *h, H3BaseDlg *dlg)
{
    Get().CreateDlgItems(dlg);
    auto _h = _PI->WriteHiHook(0x5FFAC0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    _h->Destroy();
    return result;
}
void AssemblyInformation::CreateDlgItems(H3BaseDlg *dlg)
{
    // hide wnd version hint
    if (auto *it = dlg->GetH3DlgItem(545))
        it->Hide();

    for (const auto version : versions)
    {
        if (version->show)
        {
            version->AdjustItemText();
            version->dlgItem = version->AddToDlg(dlg);
        }
    }
}

int __stdcall AssemblyInformation::DlgMainMenu_Proc(HiHook *h, H3Msg *msg)
{

    H3DlgText *visibleItem = nullptr;
    const AssemblyInformation::Version *activeVersion = nullptr;
    for (auto *ver : Get().versions)
    {
        H3DlgText *it = ver->dlgItem;
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
        P_MouseManager->SetCursor(3, 0);

        if (msg->IsLeftDown())
        {
            activeVersion->ClickProcedure();
        }
    }
    else if (msg->GetX()) // check if mouse isn't reset
    {
        P_MouseManager->DefaultCursor();
    }

    AssemblyInformation::RemoteVersion &remoteVersion = Get().m_remoteVersion;

    H3DlgText *it = remoteVersion.dlgItem;

    if (remoteVersion.workDone.load() && it && it->IsVisible())
    {

        remoteVersion.AdjustItemText();
        it->SetText(remoteVersion.text);
        H3FontLoader fn(remoteVersion.fontName);
        it->SetWidth(fn->GetMaxLineWidth(
            remoteVersion.text.String())); // (remoteVersion.text.Length() + 1)* remoteVersion.characterLength);
        it->Draw();
        it->Refresh();
        remoteVersion.workDone.store(false);
    }

    return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}

void __stdcall AssemblyInformation::OnAfterReloadLanguageData(Era::TEvent *e)
{
    Get().LoadDataFromJson();
    for (const auto it : Get().versions)
    {
        if (it && it->show)
        {
            it->AdjustItemText();
        }
    }
}

void AssemblyInformation::CreatePatches() noexcept
{
    if (!m_isEnabled)
    {
        _PI->WriteHiHook(0x4FB930, THISCALL_, DlgMainMenu_Create);
        _PI->WriteHiHook(0x4D56D0, THISCALL_, DlgMainMenu_NewLoad_Create);
        _PI->WriteHiHook(0x4F0799, THISCALL_, DlgMainMenu_Campaign_Run); // goes from new game
        // move and resize iam00.def (next hero buttn)
        constexpr BYTE defWidth = 32;
        _PI->WriteByte(0x401A85 + 1, defWidth);        // set width
        _PI->WriteDword(0x401A8C + 1, 679 + defWidth); // set y

        _PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
        _PI->WriteHiHook(0x4D5B50, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc

        //	Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");

        m_isEnabled = true;
    }
}

AssemblyInformation &AssemblyInformation::Get()
{
    static AssemblyInformation instance(_PI);
    return instance;

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
