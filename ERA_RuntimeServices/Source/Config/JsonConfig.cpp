#include "../framework.h"
#include "JsonConfig.h"

DllExport int __stdcall Erm_ReadJsonInt(const char *key, int defaultValue) noexcept
{
    return erm_functions::json_config::ReadInt(key, defaultValue,
        [](const char *name) { return EraJS::read(name); });
}

DllExport int __stdcall Erm_ReadJsonIntArray(const char *key, int *output, int capacity) noexcept
{
    try
    {
        return erm_functions::json_config::ReadIntArray(key, output, capacity,
            [](const char *name) { return EraJS::read(name); });
    }
    catch (const std::exception &) { return 0; } // Allocation or container size limit.
}
