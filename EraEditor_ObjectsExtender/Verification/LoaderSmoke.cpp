#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

int main(int argc, char **argv)
{
    if (argc != 2 || GetModuleHandleA("eramap.dll"))
        return 1;
    const HMODULE plugin = LoadLibraryA(argv[1]);
    if (!plugin)
    {
        std::printf("LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }
    const bool importedEditor = GetModuleHandleA("eramap.dll") != nullptr;
    FreeLibrary(plugin);
    if (importedEditor)
    {
        std::puts("Editor API was imported in a non-editor process");
        return 1;
    }
    std::puts("Editor DLL loaded/unloaded without importing editor runtime in a non-editor process");
    return 0;
}
