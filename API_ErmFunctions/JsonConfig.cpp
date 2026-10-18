#include "framework.h"
#include "JsonConfig.h"

DllExport int __stdcall Erm_ReadJsonInt(const char *key, int defaultValue)
{
    return erm_functions::json_config::ReadInt(key, defaultValue,
        [](const char *name) { return EraJS::read(name); });
}

DllExport int __stdcall Erm_ReadJsonIntArray(const char *key, int *output, int capacity)
{
    try
    {
        return erm_functions::json_config::ReadIntArray(key, output, capacity,
            [](const char *name) { return EraJS::read(name); });
    }
    catch (...) { return 0; }
}
