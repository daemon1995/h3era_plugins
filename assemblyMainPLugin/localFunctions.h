#pragma once
#include <wininet.h>
#include <malloc.h>
#include <stdio.h>

float getGameFromRegistry()
{
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