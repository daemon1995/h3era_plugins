// dllmain.cpp : Defines the entry point for the DLL application.

#include "pch.h"

using namespace h3;
namespace dllText
{
constexpr const char *PLUGIN_AUTHOR = "daemon_n";
constexpr const char *PLUGIN_VERSION = "1.07";
constexpr const char *PLUGIN_DATA = __DATE__;
constexpr const char *INSTANCE_NAME = "EraPlugin.LanguageSelectionDlg.daemon_n";
} // namespace dllText

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

_LHF_(HooksInit)
{

    MapObjectHandler::Init();
    TownHandler::Init();

    // ArtifactHandler::Init();
    LanguageSelectionDlg::Init();

    return EXEC_DEFAULT;
}
#include <set>

std::set<std::string> shortLocales;

BOOL CALLBACK EnumLocalesCallback(LPWSTR lpLocaleString, DWORD dwFlags, LPARAM lParam)
{
    WCHAR language[LOCALE_NAME_MAX_LENGTH];

    // �������� ���� �� ������� �������������� (��������, "en-US")
    if (GetLocaleInfoEx(lpLocaleString, LOCALE_SISO639LANGNAME, language, LOCALE_NAME_MAX_LENGTH))
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, language, -1, nullptr, 0, nullptr, nullptr);
        std::string shortLang(len - 1, 0); // -1 ��� ���������� null-�����������
        WideCharToMultiByte(CP_UTF8, 0, language, -1, const_cast<LPSTR>(shortLang.data()), len, nullptr, nullptr);
        //  if (shortLang.length() == 2)
        {
            shortLocales.insert(shortLang);
        }
    }
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
    {
        // ������: "ru-RU"
    }
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static bool pluginIsOn = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;

            // Era::ConnectEra();
            if (Era::GetVersionNum() >= 3906)
            {
                EnumSystemLocalesEx(EnumLocalesCallback, LOCALE_ALL, 0, nullptr);

                std::string locales;
                volatile int id = 0;
                for (auto &i : shortLocales)
                {
                    // locales.append(i);
                    // locales.append("\n");
                    Era::WriteStrToIni(std::to_string(id++).c_str(), i.c_str(), "Era", "Runtime/locale_names.ini");
                }
                Era::SaveIni("Runtime/locale_names.ini");

                Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
                globalPatcher = GetPatcher();
                _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
                // must have hooks before other
                MonsterHandler::Init();

                ArtifactHandler::Init();
                //   _PI->WriteLoHook(0x04EE053, gem_TestHook);

                _PI->WriteLoHook(0x4EEAF2, HooksInit);
            }
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
