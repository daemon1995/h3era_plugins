#include "ModListScanner.h"
#include "framework.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <vector>

namespace
{
constexpr LPCSTR RENAME_MODS_KEY = "assembly.update.rename_mods";

struct ListTxtData
{
    std::vector<std::string> lines;
    std::string lineEnding = "\r\n";
    bool hasFinalLineEnding = false;
};

struct ActiveModCount
{
    std::string modName;
    size_t count = 0;
};

char ToLower(const char value) noexcept
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](const char character) { return ToLower(character); });
    return value;
}

std::string Trim(std::string value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};

    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

bool IsReservedDirectoryName(const std::string &name) noexcept
{
    std::string baseName = name;
    const size_t dot = baseName.find('.');
    if (dot != std::string::npos)
        baseName.resize(dot);

    baseName = ToLower(baseName);
    if (baseName == "con" || baseName == "prn" || baseName == "aux" || baseName == "nul")
        return true;

    if (baseName.size() == 4 && (baseName.compare(0, 3, "com") == 0 || baseName.compare(0, 3, "lpt") == 0))
        return baseName[3] >= '1' && baseName[3] <= '9';

    return false;
}

bool IsValidDirectoryName(const std::string &name) noexcept
{
    if (name.empty() || name == "." || name == ".." || IsReservedDirectoryName(name))
        return false;

    if (name.back() == ' ' || name.back() == '.')
        return false;

    if (name.find_first_of("<>:\"/\\|?*") != std::string::npos)
        return false;

    return std::find_if(name.begin(), name.end(),
                        [](const char character) { return static_cast<unsigned char>(character) < 32; }) == name.end();
}

bool HasUtf8Bom(const std::string &line) noexcept
{
    return line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
           static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF;
}

std::string GetListModName(const std::string &rawLine, const size_t lineIndex)
{
    const size_t bomSize = lineIndex == 0 && HasUtf8Bom(rawLine) ? 3 : 0;
    return Trim(rawLine.substr(bomSize));
}

bool GetValidModName(const std::string &line, const size_t lineIndex, std::string &modName)
{
    modName = ToLower(GetListModName(line, lineIndex));
    return !modName.empty() && modName.front() != '*' && IsValidDirectoryName(modName);
}

void ReplaceListModName(std::string &rawLine, const size_t lineIndex, const std::string &newName)
{
    const size_t bomSize = lineIndex == 0 && HasUtf8Bom(rawLine) ? 3 : 0;
    const size_t nameStart = rawLine.find_first_not_of(" \t\r\n", bomSize);
    const size_t nameEnd = rawLine.find_last_not_of(" \t\r\n");

    if (nameStart != std::string::npos && nameEnd != std::string::npos)
        rawLine.replace(nameStart, nameEnd - nameStart + 1, newName);
}

bool GetDirectoryNameInMods(const std::string &modsPath, const std::string &modName, std::string &actualName) noexcept
{
    WIN32_FIND_DATAA fileData;
    const HANDLE findHandle = FindFirstFileA((modsPath + "\\*").c_str(), &fileData);
    if (findHandle == INVALID_HANDLE_VALUE)
        return false;

    do
    {
        if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            lstrcmpiA(fileData.cFileName, modName.c_str()) == 0)
        {
            actualName = fileData.cFileName;
            FindClose(findHandle);
            return true;
        }
    } while (FindNextFileA(findHandle, &fileData));

    FindClose(findHandle);
    return false;
}

bool ReadListTxt(const std::string &listPath, ListTxtData &result)
{
    const HANDLE listHandle = CreateFileA(listPath.c_str(), GENERIC_READ,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL, nullptr);
    if (listHandle == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(listHandle, &fileSize) || fileSize.QuadPart < 0 ||
        static_cast<unsigned long long>(fileSize.QuadPart) > std::numeric_limits<size_t>::max())
    {
        CloseHandle(listHandle);
        return false;
    }

    std::string fileContent(static_cast<size_t>(fileSize.QuadPart), '\0');
    size_t bytesToRead = fileContent.size();
    size_t bytesReadTotal = 0;
    while (bytesToRead != 0)
    {
        const DWORD readSize = static_cast<DWORD>(std::min<size_t>(bytesToRead, std::numeric_limits<DWORD>::max()));
        DWORD bytesRead = 0;
        if (!ReadFile(listHandle, &fileContent[bytesReadTotal], readSize, &bytesRead, nullptr) || bytesRead == 0)
        {
            CloseHandle(listHandle);
            return false;
        }
        bytesReadTotal += bytesRead;
        bytesToRead -= bytesRead;
    }
    CloseHandle(listHandle);

    size_t lineStart = 0;
    while (lineStart < fileContent.size())
    {
        const size_t lineEnd = fileContent.find('\n', lineStart);
        const bool hasLineFeed = lineEnd != std::string::npos;
        std::string line = fileContent.substr(lineStart, hasLineFeed ? lineEnd - lineStart : std::string::npos);
        const bool isWindowsLineEnding = !line.empty() && line.back() == '\r';
        if (isWindowsLineEnding)
            line.pop_back();
        if (result.lines.empty())
            result.lineEnding = isWindowsLineEnding ? "\r\n" : "\n";
        result.lines.push_back(std::move(line));

        if (!hasLineFeed)
            break;
        lineStart = lineEnd + 1;
    }

    result.hasFinalLineEnding = !fileContent.empty() &&
                                (fileContent.back() == '\r' || fileContent.back() == '\n');
    return true;
}

bool TryGetRenamedMod(const std::string &oldName, std::string &newNameInListTxt)
{
    const std::string renameKey = std::string(RENAME_MODS_KEY) + "." + oldName;
    bool readSuccess = false;
    const LPCSTR renamedMod = EraJS::read(renameKey, readSuccess);
    if (!readSuccess || !renamedMod || !renamedMod[0])
        return false;

    newNameInListTxt = Trim(renamedMod);
    if (!IsValidDirectoryName(newNameInListTxt))
        return false;

    return true;
}

size_t FindActiveModCount(const std::vector<ActiveModCount> &counts, const std::string &modName)
{
    for (size_t index = 0; index < counts.size(); ++index)
    {
        if (counts[index].modName == modName)
            return index;
    }
    return counts.size();
}

void IncrementActiveModCount(std::vector<ActiveModCount> &counts, const std::string &modName)
{
    const size_t index = FindActiveModCount(counts, modName);
    if (index != counts.size())
    {
        ++counts[index].count;
        return;
    }

    ActiveModCount newCount;
    newCount.modName = modName;
    newCount.count = 1;
    counts.push_back(std::move(newCount));
}

void DecrementActiveModCount(std::vector<ActiveModCount> &counts, const std::string &modName)
{
    const size_t index = FindActiveModCount(counts, modName);
    if (index != counts.size() && counts[index].count != 0)
        --counts[index].count;
}

bool ApplyRenames(ListTxtData &listTxt, const std::string &modsPath)
{
    bool wasChanged = false;
    const bool hasUtf8Bom = !listTxt.lines.empty() && HasUtf8Bom(listTxt.lines.front());
    std::vector<ActiveModCount> enabledModCounts;
    std::vector<bool> removeLines(listTxt.lines.size(), false);

    for (size_t lineIndex = 0; lineIndex < listTxt.lines.size(); ++lineIndex)
    {
        std::string modName;
        if (GetValidModName(listTxt.lines[lineIndex], lineIndex, modName))
            IncrementActiveModCount(enabledModCounts, modName);
    }

    for (size_t lineIndex = 0; lineIndex < listTxt.lines.size(); ++lineIndex)
    {
        std::string modName;
        if (!GetValidModName(listTxt.lines[lineIndex], lineIndex, modName))
            continue;

        std::string newNameInListTxt;
        const std::string newName = TryGetRenamedMod(modName, newNameInListTxt) ? ToLower(newNameInListTxt) : "";
        std::string actualNewName;
        if (!newName.empty() && newName != modName && GetDirectoryNameInMods(modsPath, newName, actualNewName))
        {
            DecrementActiveModCount(enabledModCounts, modName);
            const size_t newNameIndex = FindActiveModCount(enabledModCounts, newName);
            if (newNameIndex != enabledModCounts.size() && enabledModCounts[newNameIndex].count != 0)
            {
                removeLines[lineIndex] = true;
            }
            else
            {
                ReplaceListModName(listTxt.lines[lineIndex], lineIndex, actualNewName);
                IncrementActiveModCount(enabledModCounts, newName);
            }
            wasChanged = true;
        }
    }

    for (size_t lineIndex = listTxt.lines.size(); lineIndex-- > 0;)
    {
        if (removeLines[lineIndex])
            listTxt.lines.erase(listTxt.lines.begin() + lineIndex);
    }

    if (hasUtf8Bom && !listTxt.lines.empty() && !HasUtf8Bom(listTxt.lines.front()))
        listTxt.lines.front().insert(0, "\xEF\xBB\xBF");

    return wasChanged;
}

bool WriteListTxt(const std::string &listPath, const ListTxtData &listTxt)
{
    std::string fileContent;
    for (size_t lineIndex = 0; lineIndex < listTxt.lines.size(); ++lineIndex)
    {
        fileContent += listTxt.lines[lineIndex];
        if (lineIndex + 1 < listTxt.lines.size() || listTxt.hasFinalLineEnding)
            fileContent += listTxt.lineEnding;
    }

    const HANDLE listHandle = CreateFileA(listPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (listHandle == INVALID_HANDLE_VALUE)
        return false;

    size_t bytesToWrite = fileContent.size();
    size_t bytesWrittenTotal = 0;
    while (bytesToWrite != 0)
    {
        const DWORD writeSize = static_cast<DWORD>(std::min<size_t>(bytesToWrite, std::numeric_limits<DWORD>::max()));
        DWORD bytesWritten = 0;
        if (!WriteFile(listHandle, &fileContent[bytesWrittenTotal], writeSize, &bytesWritten, nullptr) || bytesWritten == 0)
        {
            CloseHandle(listHandle);
            return false;
        }
        bytesWrittenTotal += bytesWritten;
        bytesToWrite -= bytesWritten;
    }

    return CloseHandle(listHandle) != FALSE;
}

BOOL UpdateListTxt()
{
    std::string gamePath = h3_GamePath ? h3_GamePath : "";
    while (!gamePath.empty() && (gamePath.back() == '\\' || gamePath.back() == '/'))
        gamePath.pop_back();
    const std::string modsPath = gamePath + "\\mods";
    const std::string listPath = modsPath + "\\list.txt";
    ListTxtData listTxt;
    const bool hasListTxt = ReadListTxt(listPath, listTxt);

    if (hasListTxt)
    {
        if (ApplyRenames(listTxt, modsPath))
        {
            return WriteListTxt(listPath, listTxt) ? TRUE : FALSE;
        }
    }

    return FALSE;
}
} // namespace

namespace assemblyModList
{
BOOL Get()
{
    return UpdateListTxt();
}
} // namespace assemblyModList
