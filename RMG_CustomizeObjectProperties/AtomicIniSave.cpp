#include "AtomicIniSave.h"
#include "../headers/Era/era.h"

#include <cstring>
#include <string>

namespace rmgsettings
{
namespace
{
struct TemporaryIni
{
    std::string path;
    ~TemporaryIni()
    {
        if (!path.empty())
        {
            Era::ClearIniCache(path.c_str());
            SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(path.c_str());
        }
    }
};

bool FlushStage(const char *path)
{
    const HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const bool flushed = FlushFileBuffers(file) != FALSE;
    return CloseHandle(file) != FALSE && flushed;
}
} // namespace

bool SaveIniAtomically(const char *path, const std::function<bool(const char *)> &writer, bool preserveExisting)
{
    if (!path || !writer || !Era::ClearIniCache || !Era::EmptyIniCache || !Era::SaveIni)
        return false;

    char absolutePath[MAX_PATH]{};
    const DWORD length = GetFullPathNameA(path, MAX_PATH, absolutePath, nullptr);
    if (!length || length >= MAX_PATH)
        return false;
    const std::string target(absolutePath);
    const auto separator = target.find_last_of("\\/");
    if (separator == std::string::npos)
        return false;

    char stagePath[MAX_PATH]{};
    if (!GetTempFileNameA(target.substr(0, separator + 1).c_str(), "rmg", 0, stagePath))
        return false;
    TemporaryIni stage{stagePath};

    bool copiedExisting = false;
    if (preserveExisting)
    {
        copiedExisting = CopyFileA(absolutePath, stagePath, FALSE) != FALSE;
        if (!copiedExisting)
        {
            const DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND)
                return false;
        }
    }
    if (!copiedExisting)
        Era::EmptyIniCache(stagePath);
    else if (!SetFileAttributesA(stagePath, FILE_ATTRIBUTE_NORMAL))
        return false;

    if (!writer(stagePath) || !Era::SaveIni(stagePath) || !FlushStage(stagePath))
        return false;

    // Staging in the same directory keeps the rename on the same volume.
    // A locked/read-only destination is an error; never truncate it as a fallback.
    if (!MoveFileExA(stagePath, absolutePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return false;

    Era::ClearIniCache(path);
    if (std::strcmp(path, absolutePath) != 0)
        Era::ClearIniCache(absolutePath);
    return true;
}
} // namespace rmgsettings
