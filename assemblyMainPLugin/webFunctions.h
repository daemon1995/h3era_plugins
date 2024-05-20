#pragma once
#include "pch.h"
#include <atlstr.h>
#include <wininet.h>

#pragma comment( lib, "urlmon.lib" )
#pragma comment( lib, "wininet.lib" )


struct ComInit
{
    HRESULT hr;
    ComInit() : hr(::CoInitialize(nullptr)) {}
    ~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
};