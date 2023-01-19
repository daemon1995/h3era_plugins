#pragma once

#include <string>
#include <iostream>
#include <tchar.h>
#include <atlstr.h>
#include <fstream>
#include <urlmon.h> 
#include <wininet.h>

#pragma comment( lib, "urlmon.lib" )
#pragma comment( lib, "Wininet.lib" )

void GetOnlineVersion(std::string fileLink, std::string & resultRef);

struct ComInit
{
    HRESULT hr;
    ComInit() : hr(::CoInitialize(nullptr)) {}
    ~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
};