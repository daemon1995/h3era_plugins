#define NOMINMAX
#include <windows.h>
#include "WoGProfileStore.h"
#include <vector>

namespace era_options
{
namespace
{
struct TemporaryFile
{
    std::string path;
    bool keep = false;
    explicit TemporaryFile(const std::string &directory)
    {
        char buffer[MAX_PATH] = {};
        if (GetTempFileNameA(directory.c_str(), "eom", 0, buffer))
            path = buffer;
    }
    ~TemporaryFile() { if (!keep && !path.empty()) DeleteFileA(path.c_str()); }
};

struct FileHandle
{
    HANDLE value;
    explicit FileHandle(HANDLE value) : value(value) {}
    ~FileHandle()
    {
        const DWORD error = GetLastError();
        if (value != INVALID_HANDLE_VALUE) CloseHandle(value);
        SetLastError(error);
    }
    FileHandle(const FileHandle &) = delete;
    FileHandle &operator=(const FileHandle &) = delete;
};

bool WriteProfile(const std::string &path, const std::int32_t *values)
{
    const HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    constexpr DWORD bytes = WogOptionCount * sizeof(std::int32_t);
    const bool saved = WriteFile(file, values, bytes, &written, nullptr) && written == bytes && FlushFileBuffers(file);
    return CloseHandle(file) != FALSE && saved;
}

bool ReadContents(HANDLE file, std::vector<char> &bytes)
{
    const DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE) return false;
    bytes.resize(size);
    DWORD read = 0;
    return SetFilePointer(file, 0, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&
        ReadFile(file, bytes.data(), size, &read, nullptr) && read == size;
}

bool WriteContents(HANDLE file, const std::vector<char> &bytes)
{
    DWORD written = 0;
    return SetFilePointer(file, 0, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&
        WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
        written == bytes.size() && SetEndOfFile(file) && FlushFileBuffers(file);
}

// ERA/INI readers can share writes while denying deletion. A rename then
// fails even though the same file can be saved normally by the game's CRT.
bool ReplaceSharedFile(const std::string &stage, const std::string &target, bool *restoreFailed = nullptr)
{
    if (restoreFailed) *restoreFailed = false;
    if (MoveFileExA(stage.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return true;
    const DWORD moveError = GetLastError();
    if (moveError != ERROR_SHARING_VIOLATION && moveError != ERROR_ACCESS_DENIED) return false;
    const FileHandle source(CreateFileA(stage.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
    if (source.value == INVALID_HANDLE_VALUE) return false;
    std::vector<char> next;
    if (!ReadContents(source.value, next)) return false;
    const FileHandle file(CreateFileA(target.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    if (file.value == INVALID_HANDLE_VALUE) return false;
    std::vector<char> previous;
    bool success = ReadContents(file.value, previous);
    bool restored = true;
    DWORD error = GetLastError();
    if (success)
    {
        success = WriteContents(file.value, next);
        error = GetLastError();
        if (!success) restored = WriteContents(file.value, previous);
    }
    if (!restored)
    {
        if (restoreFailed) *restoreFailed = true;
        OutputDebugStringA(("ERA_OptionsMenu: failed to restore shared file " + target + "\n").c_str());
    }
    if (!success) SetLastError(error);
    return success;
}

bool ProfileAlreadySelected(const std::string &ini, const std::string &directory)
{
    char folder[MAX_PATH] = {}, name[MAX_PATH] = {};
    GetPrivateProfileStringA("WoGification", "Options_File_Name", "", name, MAX_PATH, ini.c_str());
    GetPrivateProfileStringA("WoGification", "Options_File_Path", "", folder, MAX_PATH, ini.c_str());
    return _stricmp(name, WogProfileName) == 0 && _stricmp(folder, directory.c_str()) == 0;
}
}

bool SaveWogProfileFiles(const std::string &directory, const std::int32_t *values, std::string *error)
{
    if (error) error->clear();
    const auto failed = [&](const char *operation) {
        const DWORD code = GetLastError();
        if (error) *error = std::string(operation) + " (Win32 error " + std::to_string(code) + ")";
        return false;
    };
    if (!values || directory.empty())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return failed("invalid WoG profile data");
    }
    const std::string target = directory + WogProfileName;
    const std::string ini = directory + "WoG.ini";
    const std::string backup = ini + ".before-era-options-menu";
    const bool selectProfile = !ProfileAlreadySelected(ini, directory);
    TemporaryFile profileStage(directory), iniStage(directory), rollback(directory);
    if (profileStage.path.empty() || iniStage.path.empty() || rollback.path.empty())
        return failed("cannot create temporary files for WoG profile");

    // Prepare both files before committing either one. Flush native INI caches
    // before cloning, so unrelated game settings are included in the copy.
    WritePrivateProfileStringA(nullptr, nullptr, nullptr, ini.c_str());
    const DWORD iniAttributes = GetFileAttributesA(ini.c_str());
    if (iniAttributes == INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_FILE_NOT_FOUND)
        return failed("cannot inspect WoG.ini");
    if (selectProfile && iniAttributes != INVALID_FILE_ATTRIBUTES)
    {
        if (!CopyFileA(ini.c_str(), iniStage.path.c_str(), FALSE))
            return failed("cannot stage WoG.ini");
        if (!CopyFileA(ini.c_str(), backup.c_str(), TRUE) && GetLastError() != ERROR_FILE_EXISTS)
            return failed("cannot back up WoG.ini");
    }
    if (!WriteProfile(profileStage.path, values)) return failed("cannot write staged WoG profile");
    if (selectProfile && (!WritePrivateProfileStringA("WoGification", "Options_File_Path", directory.c_str(), iniStage.path.c_str()) ||
        !WritePrivateProfileStringA("WoGification", "Options_File_Name", WogProfileName, iniStage.path.c_str())))
        return failed("cannot update staged WoG.ini");
    WritePrivateProfileStringA(nullptr, nullptr, nullptr, iniStage.path.c_str());

    const DWORD targetAttributes = GetFileAttributesA(target.c_str());
    const bool existed = targetAttributes != INVALID_FILE_ATTRIBUTES;
    if (!existed && GetLastError() != ERROR_FILE_NOT_FOUND)
        return failed("cannot inspect managed WoG profile");
    if (existed && !CopyFileA(target.c_str(), rollback.path.c_str(), FALSE))
        return failed("cannot back up managed WoG profile");
    bool restoreFailed = false;
    if (!ReplaceSharedFile(profileStage.path, target, &restoreFailed))
    {
        failed("cannot save managed WoG profile");
        if (restoreFailed && existed)
        {
            rollback.keep = true;
            if (error) *error += "; original profile preserved at " + rollback.path;
        }
        return false;
    }
    if (selectProfile && !ReplaceSharedFile(iniStage.path, ini))
    {
        failed("cannot select profile in WoG.ini");
        const bool restored = existed ? ReplaceSharedFile(rollback.path, target) :
                                        DeleteFileA(target.c_str()) != FALSE;
        if (!restored && existed)
        {
            rollback.keep = true;
            if (error) *error += "; original profile preserved at " + rollback.path;
            OutputDebugStringA(("ERA_OptionsMenu: profile rollback preserved at " + rollback.path + "\n").c_str());
        }
        return false;
    }
    // Invalidate the original-path cache after the atomic INI replacement.
    WritePrivateProfileStringA(nullptr, nullptr, nullptr, ini.c_str());
    return true;
}
} // namespace era_options
