#include "ExportManager.h"
#ifdef CREATE_TEXT_JSON_EXPORTS
#include "../headers/EraPluginsAPI/RuntimeServices.h"
#include "../headers/EraPluginsAPI/ConfigExportAPI.h"
#include <stdexcept>
BOOL ExportManager::WriteJsonFile(const std::string &filePath, const std::string &utf8Json)
{
    if (utf8Json.empty()) return FALSE;
    for (size_t pos = filePath.find_first_of("/\\"); pos != std::string::npos; pos = filePath.find_first_of("/\\", pos + 1))
    {
        const std::string directory = filePath.substr(0, pos);
        if (!directory.empty()) CreateDirectoryA(directory.c_str(), nullptr);
    }
    std::string content;
    for (char character : utf8Json)
    {
        if (character == '\n') content += '\r';
        content += character;
    }
    content += "\r\n";
    const std::string temporary = filePath + "." + std::to_string(GetCurrentProcessId()) + ".tmp";
    const HANDLE file = CreateFileA(temporary.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Could not create " + filePath);
    bool success = true;
    size_t offset = 0;
    while (offset < content.size())
    {
        const DWORD count = static_cast<DWORD>((std::min)(content.size() - offset, size_t(MAXDWORD)));
        DWORD written = 0;
        if (!WriteFile(file, content.data() + offset, count, &written, nullptr) || !written) { success = false; break; }
        offset += written;
    }
    if (!CloseHandle(file)) success = false;
    if (success) success = MoveFileExA(temporary.c_str(), filePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!success)
    {
        DeleteFileA(temporary.c_str());
        throw std::runtime_error("Could not write " + filePath);
    }
    return TRUE;
}

std::vector<configExport::Category> ExportManager::Collect()
{
    return configExport::Collect();
}
#endif
