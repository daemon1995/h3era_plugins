
#include "localFunctions.h"

LONG GetStringRegKey(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue, const std::wstring& strDefaultValue)
{
    strValue = strDefaultValue;
    WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        strValue = szBuffer;
    }
    return nError;
}


float getGameFromRegistry()
{
    LPCWSTR root = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HoMM 3 ERA\\";

    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, root, 0, KEY_READ, &hKey);
    bool bExistsAndSuccess(lRes == ERROR_SUCCESS);
    if (bExistsAndSuccess)
    {
        std::wstring strValueOfBinDir;
        std::wstring strKeyDefaultValue;
        GetStringRegKey(hKey, L"version", strValueOfBinDir, L"0.0");
        return std::stof(strValueOfBinDir);

    }
    else
    {
        return atof(dllText::PLUGIN_VERSION);

    }

    bool bDoesNotExistsSpecifically(lRes == ERROR_FILE_NOT_FOUND);



#define BUFFER 8192
    wchar_t  value[255];
    DWORD BufferSize = BUFFER;

    LPCWSTR mainRoot = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HoMM 3 ERA";
    LPCWSTR KeyName = L"Version";
    //  LPCWSTR* value;

    int result = RegGetValueW(HKEY_LOCAL_MACHINE, mainRoot, KeyName, RRF_RT_REG_SZ, NULL, &value, &BufferSize);

    if (result == 0)
    {
        return std::stof(value);
    }
    else
    {
        return 0;
    }
}




