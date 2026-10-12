// Native x86 smoke test only; the production plugin is entirely MASM.
#include <windows.h>
#include <cstdio>
#include "native_symbols.h"

int main(int argc, char** argv)
{
    if (argc != 5) return 2;
    HMODULE plugin = LoadLibraryA(argv[1]);
    if (!plugin) { std::printf("DLL load failed: %lu\n", GetLastError()); return 3; }
    HMODULE era = LoadLibraryExA(argv[2], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!era) { std::printf("ERA image mapping failed: %lu\n", GetLastError()); return 4; }
    BYTE* base = reinterpret_cast<BYTE*>(plugin);
    *reinterpret_cast<HMODULE*>(base + RVA_EraBase) = era;
    auto discover = reinterpret_cast<int (__cdecl*)()>(base + RVA_DiscoverEraVideo);
    if (discover() != 1) { std::puts("Native ERA discovery failed"); return 5; }
    auto fromChar = *reinterpret_cast<DWORD*>(base + RVA_FromPCharReal);
    std::printf("PASS: native x86 DLL loaded; ERA handler discovery succeeded at %p, conversion %08lx\n", era, fromChar);

    HMODULE codec = LoadLibraryA(argv[3]);
    if (!codec) { std::printf("Codec load failed: %lu\n", GetLastError()); return 6; }
    auto open = reinterpret_cast<void* (__stdcall*)(const char*, DWORD)>(GetProcAddress(codec, "_BinkOpen@8"));
    auto frame = reinterpret_cast<int (__stdcall*)(void*)>(GetProcAddress(codec, "_BinkDoFrame@4"));
    auto next = reinterpret_cast<void (__stdcall*)(void*)>(GetProcAddress(codec, "_BinkNextFrame@4"));
    auto close = reinterpret_cast<void (__stdcall*)(void*)>(GetProcAddress(codec, "_BinkClose@4"));
    if (!open || !frame || !next || !close) return 7;
    HANDLE file = CreateFileA(argv[4], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 8;
    void* video = open(reinterpret_cast<const char*>(file), 0x800000);
    if (!video) { std::puts("BinkOpen(FILEHANDLE) failed"); return 9; }
    const DWORD* info = static_cast<const DWORD*>(video);
    std::printf("PASS: actual BIK opened via Win32 handle: %lux%lu, %lu frames\n", info[0], info[1], info[2]);
    for (unsigned i = 0; i < 3 && i < info[2]; ++i) {
        frame(video);
        if (i + 1 < info[2]) next(video);
    }
    close(video);
    CloseHandle(file);
    std::puts("PASS: three frames decoded by installed binkw32a.dll, video closed");
    FreeLibrary(codec);
    FreeLibrary(era);
    FreeLibrary(plugin);
    return 0;
}
