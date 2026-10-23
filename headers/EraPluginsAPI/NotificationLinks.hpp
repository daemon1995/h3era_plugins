#pragma once
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

// Include after header.h. Shared by menu notifications and assembly labels.
namespace mainmenu
{
inline void OpenNotificationLink(const char *path)
{
    if (!path || !path[0])
        return;
    const char *key = PathIsURLA(path) ? "era.notifications.user.open_web_page"
                                     : "era.notifications.user.open_external_file";
    libc::sprintf(h3_TextBuffer, EraJS::read(key), path);
    if (!h3::H3Messagebox::Choice(h3_TextBuffer))
        return;

    const INT_PTR result = STDCALL_6(INT_PTR, PtrAt(0x63A250), NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
    if (result <= 32)
    {
        LPSTR buffer = nullptr;
        const DWORD size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, static_cast<DWORD>(result), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
        if (size && buffer)
            h3::H3Messagebox(buffer);
        LocalFree(buffer);
    }
}
} // namespace mainmenu
