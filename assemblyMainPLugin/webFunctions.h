#pragma once
//bool url();

#include <string>

#include <tchar.h>
#include <atlstr.h>
#include <fstream>
#include <urlmon.h> 
#include <wininet.h>
#pragma comment( lib, "urlmon.lib" )
#pragma comment( lib, "Wininet.lib" )

using namespace std;

ULONG divider = 100;


struct ComInit
{
    HRESULT hr;
    ComInit() : hr(::CoInitialize(nullptr)) {}
    ~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
};

float checkOnlineVersion(string fileLink)
{
   // USES_CONVERSION_EX;
   bool clearCa = DeleteUrlCacheEntryA(fileLink.c_str());
  // return 1.1f;
   // LPCWSTR siteForCheckingVersion = A2W_EX(fileLink.c_str(), fileLink.length());


    ComInit init;
    // use CComPtr so you don't have to manually call Release()
    CComPtr<IStream> pStream;
    // Open the HTTP request.
    HRESULT hr = URLOpenBlockingStreamA(nullptr, fileLink.c_str(), &pStream, 0, nullptr);
    if (FAILED(hr))
    {
        //  std::cout << "ERROR: Could not connect. HRESULT: 0x" << std::hex << hr << std::dec << "\n";
        return 0;
    }
    // Download the response and write it to stdout.
    else
    {
        char buffer[64] = "";
        string answer(buffer);

        do
        {
            DWORD bytesRead = 0;
            hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);

            if (bytesRead > 0)
            {
                //std::cout.write(buffer, bytesRead)<<"\n";
               // std::stod(buffer);
                return std::stod(buffer);// .erase(answer.find(""), 0);

            }

        } while (SUCCEEDED(hr) && hr != S_FALSE);


        if (FAILED(hr))
        {
            //std::cout << "ERROR: Download failed. HRESULT: 0x" << std::hex << hr << std::dec << "\n";
            return 0;
        }

    }

}

