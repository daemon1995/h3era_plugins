#include <Windows.h>
#include <array>
#include <string>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
namespace web
{
namespace
{
class WinHttpHandle
{
    HINTERNET handle;

  public:
    explicit WinHttpHandle(HINTERNET value) noexcept : handle(value) {}
    ~WinHttpHandle() noexcept
    {
        if (handle)
            WinHttpCloseHandle(handle);
    }
    WinHttpHandle(const WinHttpHandle &) = delete;
    WinHttpHandle &operator=(const WinHttpHandle &) = delete;
    operator HINTERNET() const noexcept { return handle; }
};
} // namespace

std::string PerformWinHTTPRequest(const wchar_t *api, const wchar_t *host, const wchar_t *path)
{

    const WinHttpHandle hSession(
        WinHttpOpen(api, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));

    if (!hSession)
        return "";

    if (!WinHttpSetTimeouts(hSession, 15000, 15000, 15000, 15000))
        return "";

    const WinHttpHandle hConnect(WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0));

    if (!hConnect)
    {
        return "";
    }

    const WinHttpHandle hRequest(WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER,
                                                   WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));

    if (!hRequest)
    {
        return "";
    }

    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    if (!bResults)
    {
        return "";
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        statusCode < 200 || statusCode >= 300)
        return "";

    // Reuse a fixed buffer and discard the entire response on any read error.
    constexpr size_t MAX_RESPONSE_SIZE = 4 * 1024 * 1024;
    std::array<char, 8192> buffer;
    std::string response;
    for (;;)
    {
        DWORD downloaded = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), static_cast<DWORD>(buffer.size()), &downloaded))
            return "";
        if (!downloaded)
            return response;
        if (downloaded > MAX_RESPONSE_SIZE - response.size())
            return "";
        response.append(buffer.data(), downloaded);
    }
}
} // namespace web
