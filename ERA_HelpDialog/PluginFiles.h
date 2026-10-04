#pragma once
#include "HelpLogic.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace helpdlg
{
inline std::vector<std::string> EnumeratePluginFiles(const std::string &modRoot)
{
    std::vector<std::string> files;
    for (const char *directory : {"EraPlugins", "EraPlugins/AfterWog", "EraPlugins/BeforeWog"})
    {
        WIN32_FIND_DATAA data{};
        const auto pattern = modRoot + "/" + directory + "/*";
        const HANDLE handle = FindFirstFileA(pattern.c_str(), &data);
        if (handle == INVALID_HANDLE_VALUE)
            continue;
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && IsPluginFile(data.cFileName))
                files.push_back(std::string(directory) + "/" + data.cFileName);
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
    }
    std::sort(files.begin(), files.end(), [](const std::string &a, const std::string &b) {
        return NormalizePluginPath(a) < NormalizePluginPath(b);
    });
    return files;
}
} // namespace helpdlg
