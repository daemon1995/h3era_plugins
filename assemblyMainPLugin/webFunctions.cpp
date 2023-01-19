#include "webFunctions.h"
using namespace std;

ULONG divider = 100;




void GetOnlineVersion(string fileLink, string& result)
{
    // USES_CONVERSION_EX;
    BOOL clearCa = DeleteUrlCacheEntryA(fileLink.c_str());
    // return 1.1f;
     // LPCWSTR siteForCheckingVersion = A2W_EX(fileLink.c_str(), fileLink.length());

   // result = "N/A";

    ComInit init;
    // use CComPtr so you don't have to manually call Release()
    CComPtr<IStream> pStream;
    // Open the HTTP request.
    HRESULT hr = URLOpenBlockingStreamA(nullptr, fileLink.c_str(), &pStream, 0, nullptr);
    if (FAILED(hr))
    {
        //  std::cout << "ERROR: Could not connect. HRESULT: 0x" << std::hex << hr << std::dec << "\n";
        return;
    }
    else
    {
        char buffer[64] = "";
        do
        {
            DWORD bytesRead = 0;
            hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);

            if (bytesRead > 0)
            {
                std::string aa(buffer);

                result = aa;
                return;// .erase(answer.find(""), 0);
            }

        } while (SUCCEEDED(hr) && hr != S_FALSE);

        if (FAILED(hr))
        {
            //std::cout << "ERROR: Download failed. HRESULT: 0x" << std::hex << hr << std::dec << "\n";
            return;
        }
    }
}