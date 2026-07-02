#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
constexpr LPCSTR PLUGIN_AUTHOR = "daemon_n";
constexpr LPCSTR PLUGIN_VERSION = "1.1";
constexpr LPCSTR INSTANCE_NAME = "EraPlugin." PROJECT_NAME ".daemon_n";
} // namespace dllText
#define TEXT_HANDLER_DECLATOR(className)                                                                               \
    class className                                                                                                    \
    {                                                                                                                  \
      public:                                                                                                          \
        static void Init();                                                                                            \
    };

TEXT_HANDLER_DECLATOR(ArtifactHandler)
TEXT_HANDLER_DECLATOR(HeroHandler)
TEXT_HANDLER_DECLATOR(MapObjectHandler)
TEXT_HANDLER_DECLATOR(MonsterHandler)
TEXT_HANDLER_DECLATOR(TownHandler)
TEXT_HANDLER_DECLATOR(SpellHandler)

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            // credits: RoseKavalier
            const IMAGE_DOS_HEADER *pDOSHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(0x400000);
            const IMAGE_NT_HEADERS *pNTHeaders =
                reinterpret_cast<IMAGE_NT_HEADERS *>((PBYTE(pDOSHeader) + pDOSHeader->e_lfanew));
            const DWORD entry = pNTHeaders->OptionalHeader.AddressOfEntryPoint;
            if (entry != 0xE84D4 || IntAt(0x4E84D4 + 1) != 0x596ED4)
            {
                return TRUE;
            }
            initialized = true;
            globalPatcher = GetPatcher();
            Eramap::ConnectEra(hModule, dllText::INSTANCE_NAME);
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            ArtifactHandler::Init();
            MapObjectHandler::Init();
            // TownHandler::Init();
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
