#pragma once
#include "era.h"
#include <sstream>
namespace modList
{

// Функция для получения каталога исполняемого процесса
inline std::string GetExecutableDirectory()
{
    // Буфер для хранения полного пути
    char path[MAX_PATH];

    // Получаем полный путь к исполняемому файлу
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
    {
        return ""; // В случае ошибки возвращаем пустую строку
    }

    // Преобразуем путь в строку C++
    std::string fullPath(path);

    // Находим последнюю обратную косую черту (разделитель каталогов)
    size_t lastSlashPos = fullPath.find_last_of("\\/");
    if (lastSlashPos == std::string::npos)
    {
        return ""; // Если разделитель не найден, возвращаем пустую строку
    }

    // Возвращаем подстроку до последней обратной косой черты
    return fullPath.substr(0, lastSlashPos);
}

inline std::string GetEraMappedModList()
{
    if (HINSTANCE hDll = GetModuleHandleA("vfs.dll"))
    {
        using GetModList = Era::era_str(__stdcall *)();
        // get exported function
        if (GetModList getModList = (GetModList)GetProcAddress(hDll, "GetMappingsReportA"))
        {
            // call it and store char *
            Era::era_str modList = getModList();

            // create string to save content
            std::string sModList = modList;

            // get "Free" function
            using FreeModList = void(__stdcall *)(void *);

            FreeModList freeModList = (FreeModList)GetProcAddress(hDll, "MemFree");
            if (freeModList)
            {
                // clear memory
                freeModList(modList);
            }

            return sModList;
        }
    }
    return "";
}

inline std::string ExtractModNameFromPath(const std::string &input)
{
    // Ищем начало пути
    size_t startPos = input.find("$ <= $");
    if (startPos == std::string::npos)
        return "";
    startPos += 12; // Пропускаем "$ <= $"

    // Ищем конец пути до квадратной скобки
    size_t endPos = input.find("[", startPos);
    if (endPos == std::string::npos)
        return "";

    // Извлекаем путь и убираем пробелы по краям
    std::string path = input.substr(startPos, endPos - startPos);
    size_t first = path.find_first_not_of(' ');
    size_t last = path.find_last_not_of(' ');

    return path.substr(first, last - first + 1);
}
inline int GetEraModList(std::vector<std::string> &modList, const BOOL toLower = false)
{
    modList.clear();

    std::istringstream stream(GetEraMappedModList());
    std::string line;

    while (std::getline(stream, line))
    {
        std::string parsedLine = ExtractModNameFromPath(line);
        // H3Messagebox(parsedLine.c_str());
        if (!parsedLine.empty())
        {
            modList.emplace_back(parsedLine);
        }
    }
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
