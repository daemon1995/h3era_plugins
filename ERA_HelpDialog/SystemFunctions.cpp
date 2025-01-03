// #include "framework.h"
#include "SystemFunctions.h"
#include "..\headers\Era\era.h"
#include <Windows.h>
namespace external
{

// Функция для получения каталога исполняемого процесса
std::string GetExecutableDirectory()
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

std::string GetLoadedGameMods()
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
} // namespace external
