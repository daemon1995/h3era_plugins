#pragma once
#include "era.h"

namespace modList
{
static std::vector<std::string> globalModList;

// ������� ��� ��������� �������� ������������ ��������
inline std::string GetExecutableDirectory()
{
    // ����� ��� �������� ������� ����
    char path[MAX_PATH];

    // �������� ������ ���� � ������������ �����
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
    {
        return h3_NullString; // � ������ ������ ���������� ������ ������
    }

    // ����������� ���� � ������ C++
    std::string fullPath(path);

    // ������� ��������� �������� ����� ����� (����������� ���������)
    size_t lastSlashPos = fullPath.find_last_of("\\/");
    if (lastSlashPos == std::string::npos)
    {
        return h3_NullString; // ���� ����������� �� ������, ���������� ������ ������
    }

    // ���������� ��������� �� ��������� �������� ����� �����
    return fullPath.substr(0, lastSlashPos);
}

// ������ int �� ������ (�����������, little endian)
inline int ReadInt(const char *&p)
{
    int value;
    std::memcpy(&value, p, sizeof(int));
    p += sizeof(int);
    return value;
}

// ������ ������ (������� �����, ����� �������)
inline std::string ReadStrWithLenField(const char *&p)
{
    const int len = ReadInt(p);
    std::string result(p, len);
    p += len;
    return result;
}

// �������� ������� �������
inline void ParseSerializedModList(void *SerializedModList, std::vector<std::string> &ResModList)
{
    assert(SerializedModList != nullptr);

    const char *p = static_cast<const char *>(SerializedModList);
    const int NumMods = ReadInt(p);
    assert(NumMods >= 0);

    ResModList.resize(NumMods);
    for (int i = 0; i < NumMods; ++i)
    {
        ResModList[i] = ReadStrWithLenField(p);
    }
}

inline BOOL GetEraMappedModList(std::vector<std::string> &modLsit)
{
    if (HINSTANCE hDll = GetModuleHandleA("vfs.dll"))
    {
        // using GetModList = Era::era_str(__stdcall *)();
        using GetSerializedModListA = void *(__stdcall *)();
        // get exported function
        if (GetSerializedModListA getModList = (GetSerializedModListA)GetProcAddress(hDll, "GetSerializedModListA"))
        {

            void *SerializedModList = getModList();
            ParseSerializedModList(SerializedModList, modLsit);

            // get "Free" function
            using FreeModList = void(__stdcall *)(void *);

            FreeModList freeModList = (FreeModList)GetProcAddress(hDll, "MemFree");
            if (freeModList)
            {
                // clear memory
                freeModList(SerializedModList);
            }

            return true;
        }
    }
    return false;
}

inline std::string ExtractModNameFromPath(const std::string &input)
{
    // ���� ������ ����
    size_t startPos = input.find("$ <= $");
    if (startPos == std::string::npos)
        return h3_NullString;
    startPos += 12; // ���������� "$ <= $"

    // ���� ����� ���� �� ���������� ������
    size_t endPos = input.find("[", startPos);
    if (endPos == std::string::npos)
        return h3_NullString;

    // ��������� ���� � ������� ������� �� �����
    std::string path = input.substr(startPos, endPos - startPos);
    size_t first = path.find_first_not_of(' ');
    size_t last = path.find_last_not_of(' ');

    return path.substr(first, last - first + 1);
}
inline int GetEraModList(std::vector<std::string> &modList, const BOOL toLower = false)
{
    modList.clear();

    // std::istringstream stream(GetEraMappedModList());
    // std::string line;

    if (globalModList.empty())
    {
        GetEraMappedModList(globalModList);
    }
    modList = globalModList;

    if (toLower)
    {
        for (auto &modName : modList)
        {
            std::transform(modName.begin(), modName.end(), modName.begin(), ::tolower);
        }
    }
    return modList.size();
}
inline std::vector<std::string> GetEraModList()
{
    std::vector<std::string> modLsit;
    GetEraModList(modLsit);
    return modLsit;
}

} // namespace modList
